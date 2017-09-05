/*
 * Copyright (c) 1997, 98, 2000  Motoyuki Kasahara
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "appendix.h"

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * The maximum length of path name.
 */
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX	MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX	1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

/*
 * Unexported functions.
 */
static EB_Error_Code eb_initialize_appendix_catalog EB_P((EB_Appendix *));


/*
 * Clear alternation text cache in `appendix'.
 */
void
eb_initialize_alt_cache(appendix)
    EB_Appendix *appendix;
{
    EB_Alternation_Cache *p;
    int i;

    for (i = 0, p = appendix->narrow_cache;
	 i < EB_MAX_ALTERNATION_CACHE; i++, p++)
	p->character_number = -1;
    for (i = 0, p = appendix->wide_cache;
	 i < EB_MAX_ALTERNATION_CACHE; i++, p++)
	p->character_number = -1;
}


/*
 * Initialize `appendix'.
 */
void
eb_initialize_appendix(appendix)
    EB_Appendix *appendix;
{
    eb_initialize_lock(&appendix->lock);
    appendix->path = NULL;
    appendix->subbook_current = NULL;
    appendix->subbooks = NULL;
    eb_initialize_alt_cache(appendix);
}


/*
 * Finalize `appendix'.
 */
void
eb_finalize_appendix(appendix)
    EB_Appendix *appendix;
{
    /*
     * Dispose memories and unset struct members.
     */
    eb_unset_appendix_subbook(appendix);
    if (appendix->subbooks != NULL)
	free(appendix->subbooks);

    if (appendix->path != NULL)
	free(appendix->path);

    appendix->path = NULL;
    appendix->subbook_current = NULL;
    appendix->subbooks = NULL;

    eb_initialize_alt_cache(appendix);
    eb_finalize_lock(&appendix->lock);
}


/*
 * Suspend `appendix'.
 */
void
eb_suspend_appendix(appendix)
    EB_Appendix *appendix;
{
    eb_lock(&appendix->lock);
    eb_unset_appendix_subbook(appendix);
    eb_unlock(&appendix->lock);
}


/*
 * Bind `appendix' to `path'.
 */
EB_Error_Code
eb_bind_appendix(appendix, path)
    EB_Appendix *appendix;
    const char *path;
{
    EB_Error_Code error_code;
    char temporary_path[PATH_MAX + 1];

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Reset structure members in the appendix.
     */
    eb_finalize_appendix(appendix);
    eb_initialize_appendix(appendix);

    /*
     * Set path of the appendix.
     * The length of the file name "path/subdir/subsubdir/file.;1" must
     * not exceed PATH_MAX.
     */
    if (PATH_MAX < strlen(path)) {
	error_code = EB_ERR_TOO_LONG_FILE_NAME;
	goto failed;
    }
    strcpy(temporary_path, path);
    error_code = eb_canonicalize_path_name(temporary_path);
    if (error_code != EB_SUCCESS)
	goto failed;
    appendix->path_length = strlen(temporary_path);

    if (PATH_MAX < appendix->path_length + 1 + EB_MAX_DIRECTORY_NAME_LENGTH
	+ 1 + EB_MAX_DIRECTORY_NAME_LENGTH + 1 + EB_MAX_FILE_NAME_LENGTH) {
	error_code = EB_ERR_TOO_LONG_FILE_NAME;
	goto failed;
    }

    appendix->path = (char *)malloc(appendix->path_length + 1);
    if (appendix->path == NULL) {
	error_code = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }
    strcpy(appendix->path, temporary_path);

    /*
     * Read information from the `CATALOG(S)' file.
     */
    error_code = eb_initialize_appendix_catalog(appendix);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_finalize_appendix(appendix);
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Read information from the `CATALOG(S)' file in `appendix'.
 * Return EB_SUCCESS, if it succeeds, error-code ohtherwise.
 */
static EB_Error_Code
eb_initialize_appendix_catalog(appendix)
    EB_Appendix *appendix;
{
    EB_Error_Code error_code;
    char buffer[EB_SIZE_PAGE];
    char catalog_path_name[PATH_MAX + 1];
    char *space;
    EB_Appendix_Subbook *subbook;
    size_t catalog_size;
    size_t title_size;
    int file = -1;
    int i;

    /*
     * Find a catalog file.
     */
    if (eb_compose_path_name(appendix->path, EB_FILE_NAME_CATALOG,
	EB_SUFFIX_NONE, catalog_path_name) == 0) {
	appendix->disc_code = EB_DISC_EB;
	catalog_size = EB_SIZE_EB_CATALOG;
	title_size = EB_MAX_EB_TITLE_LENGTH;
    } else if (eb_compose_path_name(appendix->path, EB_FILE_NAME_CATALOGS,
	EB_SUFFIX_NONE, catalog_path_name) == 0) {
	appendix->disc_code = EB_DISC_EPWING;
	catalog_size = EB_SIZE_EPWING_CATALOG;
	title_size = EB_MAX_EPWING_TITLE_LENGTH;
    } else {
	error_code = EB_ERR_FAIL_OPEN_CATAPP;
	goto failed;
    }

    /*
     * Open the catalog file.
     */
    file = open(catalog_path_name, O_RDONLY | O_BINARY);
    if (file < 0) {
	error_code = EB_ERR_FAIL_OPEN_CATAPP;
	goto failed;
    }

    /*
     * Get the number of subbooks in the appendix.
     */
    if (eb_read_all(file, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_CATAPP;
	goto failed;
    }
    appendix->subbook_count = eb_uint2(buffer);
    if (EB_MAX_SUBBOOKS < appendix->subbook_count)
	appendix->subbook_count = EB_MAX_SUBBOOKS;
    if (EB_MAX_SUBBOOKS == 0) {
	appendix->subbook_count = EB_ERR_UNEXP_CATAPP;
	goto failed;
    }

    /*
     * Allocate memories for subbook entries.
     */
    appendix->subbooks = (EB_Appendix_Subbook *)
	malloc(sizeof(EB_Appendix_Subbook) * appendix->subbook_count);
    if (appendix->subbooks == NULL) {
	error_code = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }

    /*
     * Read subbook information.
     */
    for (i = 0, subbook = appendix->subbooks; i < appendix->subbook_count;
	 i++, subbook++) {
	/*
	 * Read data from the catalog file.
	 */
	if (eb_read_all(file, buffer, catalog_size) != catalog_size) {
	    error_code = EB_ERR_FAIL_READ_CAT;
	    goto failed;
	}

	/*
	 * Set a directory name of the subbook.
	 */
	strncpy(subbook->directory_name, buffer + 2 + title_size,
	    EB_MAX_DIRECTORY_NAME_LENGTH);
	subbook->directory_name[EB_MAX_DIRECTORY_NAME_LENGTH] = '\0';
	space = strchr(subbook->directory_name, ' ');
	if (space != NULL)
	    *space = '\0';
	eb_fix_directory_name(appendix->path, subbook->directory_name);

	subbook->initialized = 0;
	subbook->code = i;
    }

    /*
     * Close the catalog file.
     */
    close(file);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);
    if (appendix->subbooks != NULL) {
	free(appendix->subbooks);
	appendix->subbooks = NULL;
    }
    return error_code;
}


/*
 * Examine whether `appendix' is bound or not.
 */
int
eb_is_appendix_bound(appendix)
    EB_Appendix *appendix;
{
    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Examine whether the appendix is bound.
     */
    if (appendix->path == NULL)
	goto failed;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&appendix->lock);
    return 0;
}


/*
 * Get the bound path of `appendix'.
 */
EB_Error_Code
eb_appendix_path(appendix, path)
    EB_Appendix *appendix;
    char *path;
{
    EB_Error_Code error_code;

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Check for the current status.
     */
    if (appendix->path == NULL) {
	error_code = EB_ERR_UNBOUND_APP;
	goto failed;
    }

    /*
     * Copy the path to `path'.
     */
    strcpy(path, appendix->path);

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *path = '\0';
    eb_unlock(&appendix->lock);
    return error_code;
}


