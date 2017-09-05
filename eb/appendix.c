/*
 * Copyright (c) 1997, 98, 2000, 01
      Motoyuki Kasahara
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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "appendix.h"
#include "internal.h"

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
    int i;

    /*
     * Dispose memories and unset struct members.
     */
    eb_unset_appendix_subbook(appendix);
    if (appendix->subbooks != NULL) {
	for (i = 0; i < appendix->subbook_count; i++)
	    zio_finalize(&(appendix->subbooks + i)->zio);

	free(appendix->subbooks);
    }

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
     * be PATH_MAX maximum.
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
     * Read information from the catalog file.
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
 * Hints of catalog file name in appendix package.
 */
#define EB_HINT_INDEX_CATALOG		0
#define EB_HINT_INDEX_CATALOGS		1

static const char *catalog_hint_list[] = {
    "catalog", "catalogs", NULL
};

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
    char catalog_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char catalog_path_name[PATH_MAX + 1];
    char *space;
    EB_Appendix_Subbook *subbook;
    size_t catalog_size;
    size_t title_size;
    int hint_index;
    Zio zio;
    int i;

    zio_initialize(&zio);

    /*
     * Find a catalog file.
     */
    eb_find_file_name(appendix->path, catalog_hint_list, catalog_file_name,
	&hint_index);

    switch (hint_index) {
    case EB_HINT_INDEX_CATALOG:
	appendix->disc_code = EB_DISC_EB;
	catalog_size = EB_SIZE_EB_CATALOG;
	title_size = EB_MAX_EB_TITLE_LENGTH;
	break;

    case EB_HINT_INDEX_CATALOGS:
	appendix->disc_code = EB_DISC_EPWING;
	catalog_size = EB_SIZE_EPWING_CATALOG;
	title_size = EB_MAX_EPWING_TITLE_LENGTH;
	break;

    default:
	error_code = EB_ERR_FAIL_OPEN_CATAPP;
	goto failed;
    }

    eb_compose_path_name(appendix->path, catalog_file_name, catalog_path_name);

    /*
     * Open the catalog file.
     */
    if (zio_open(&zio, catalog_path_name, ZIO_NONE) < 0) {
	error_code = EB_ERR_FAIL_OPEN_CATAPP;
	goto failed;
    }

    /*
     * Get the number of subbooks in the appendix.
     */
    if (zio_read(&zio, buffer, 16) != 16) {
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
	if (zio_read(&zio, buffer, catalog_size) != catalog_size) {
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
	zio_initialize(&subbook->zio);
    }

    /*
     * Close the catalog file.
     */
    zio_close(&zio);
    zio_finalize(&zio);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    zio_close(&zio);
    zio_finalize(&zio);
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


