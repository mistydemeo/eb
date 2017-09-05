/*
 * Copyright (c) 1997, 1998  Motoyuki Kasahara
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
static int eb_initialize_appendix_catalog EB_P((EB_Appendix *));


/*
 * Clear cache for alternation text in `appendix'.
 */
void
eb_initialize_alt_cache(appendix)
    EB_Appendix *appendix;
{
    EB_Alternation_Cache *p;
    int i;

    for (i = 0, p = appendix->narw_cache;
	 i < EB_MAX_ALTERNATION_CACHE; i++, p++)
	p->charno = -1;
    for (i = 0, p = appendix->wide_cache;
	 i < EB_MAX_ALTERNATION_CACHE; i++, p++)
	p->charno = -1;
}


/*
 * Initialize `appendix'.
 */
void
eb_initialize_appendix(appendix)
    EB_Appendix *appendix;
{
    appendix->path = NULL;
    appendix->sub_current = NULL;
    appendix->subbooks = NULL;

    eb_initialize_alt_cache(appendix);
}


/*
 * Finish using `appendix'.
 */
void
eb_clear_appendix(appendix)
    EB_Appendix *appendix;
{
    eb_unset_appendix_subbook(appendix);
    if (appendix->subbooks != NULL)
	free(appendix->subbooks);

    if (appendix->path != NULL)
	free(appendix->path);

    eb_initialize_appendix(appendix);
    eb_zclear();
}


/*
 * Suspend using `appendix'.
 */
void
eb_suspend_appendix(appendix)
    EB_Appendix *appendix;
{
    eb_unset_appendix_subbook(appendix);
}


/*
 * Bind `appendix' to `path'.
 */
int
eb_bind_appendix(appendix, path)
    EB_Appendix *appendix;
    const char *path;
{
    char tmppath[PATH_MAX + 1];
    int count;

    /*
     * Reset structure members in the appendix.
     */
    eb_initialize_appendix(appendix);

    /*
     * Set a path of the appendix.
     * The length of the filename "path/subdir/subsubdir/file.;1" must
     * not exceed PATH_MAX.
     */
    if (PATH_MAX < strlen(path)) {
	eb_error = EB_ERR_TOO_LONG_FILENAME;
	goto failed;
    }

    strcpy(tmppath, path);
    if (eb_canonicalize_filename(tmppath) < 0)
	goto failed;

    appendix->path_length = strlen(tmppath);
    if (PATH_MAX < appendix->path_length + (1 + EB_MAXLEN_BASENAME + 1
	+ EB_MAXLEN_BASENAME + 1 + EB_MAXLEN_BASENAME + 3)) {
	eb_error = EB_ERR_TOO_LONG_FILENAME;
	goto failed;
    }

    appendix->path = (char *)malloc(appendix->path_length + 1);
    if (appendix->path == NULL) {
	eb_error = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }
    strcpy(appendix->path, tmppath);

    /*
     * Get disc type and filename mode.
     */
    if (eb_appendix_catalog_filename(appendix) < 0)
	goto failed;

    /*
     * Read information from the `CATALOG(S)' file.
     */
    count = eb_initialize_appendix_catalog(appendix);
    if (count < 0)
	goto failed;
    
    return count;

    /*
     * An error occurs...
     */
  failed:
    eb_clear_appendix(appendix);
    return -1;
}


/*
 * Read information from the `CATALOG(S)' file in `appendix'.
 *
 * If succeeded, the number of subbooks in `appendix' is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
static int
eb_initialize_appendix_catalog(appendix)
    EB_Appendix *appendix;
{
    char buf[EB_SIZE_PAGE];
    char catalog[PATH_MAX + 1];
    char *space;
    EB_Appendix_Subbook *sub;
    size_t catalogsize;
    size_t titlesize;
    int file = -1;
    int i;

    if (appendix->disc_code == EB_DISC_EB) {
	catalogsize = EB_SIZE_EB_CATALOG;
	titlesize = EB_MAXLEN_EB_TITLE;
	sprintf(catalog, "%s/%s", appendix->path, EB_FILENAME_CATALOG);
    } else {
	catalogsize = EB_SIZE_EPWING_CATALOG;
	titlesize = EB_MAXLEN_EPWING_TITLE;
	sprintf(catalog, "%s/%s", appendix->path, EB_FILENAME_CATALOGS);
    }
	
    /*
     * Open the catalog file.
     */
    eb_fix_appendix_filename(appendix, catalog);
    file = open(catalog, O_RDONLY | O_BINARY);
    if (file < 0)
	goto failed;

    /*
     * Get the number of subbooks in the appendix.
     */
    if (eb_read_all(file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_CATAPP;
	goto failed;
    }
    appendix->sub_count = eb_uint2(buf);
    if (EB_MAX_SUBBOOKS < appendix->sub_count)
	appendix->sub_count = EB_MAX_SUBBOOKS;
    if (EB_MAX_SUBBOOKS == 0) {
	appendix->sub_count = EB_ERR_UNEXP_CATAPP;
	goto failed;
    }

    /*
     * Allocate memories for subbook entries.
     */
    appendix->subbooks = (EB_Appendix_Subbook *)
	malloc(sizeof(EB_Appendix_Subbook) * appendix->sub_count);
    if (appendix->subbooks == NULL) {
	eb_error = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }

    /*
     * Read subbook information.
     */
    for (i = 0, sub = appendix->subbooks; i < appendix->sub_count; i++, sub++) {
	/*
	 * Read data from the catalog file.
	 */
	if (eb_read_all(file, buf, catalogsize) != catalogsize) {
	    eb_error = EB_ERR_FAIL_READ_CAT;
	    goto failed;
	}

	/*
	 * Set a directory name of the subbook.
	 */
	strncpy(sub->directory, buf + 2 + titlesize, EB_MAXLEN_BASENAME);
	sub->directory[EB_MAXLEN_BASENAME] = '\0';
	space = strchr(sub->directory, ' ');
	if (space != NULL)
	    *space = '\0';

	sub->initialized = 0;
	sub->code = i;
    }

    /*
     * Close the catalog file.
     */
    close(file);
    return appendix->sub_count;

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

    return -1;
}


/*
 * Examine whether `appendix' is bound to a path or not.
 */
int
eb_is_appendix_bound(appendix)
    EB_Appendix *appendix;
{
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	return 0;
    }

    return 1;
}


/*
 * Return the bound path of `appendix'.
 */
const char *
eb_appendix_path(appendix)
    EB_Appendix *appendix;
{
    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	return NULL;
    }

    return appendix->path;
}


