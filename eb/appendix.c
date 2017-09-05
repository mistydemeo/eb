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

#include "build-pre.h"
#include "eb.h"
#include "error.h"
#include "appendix.h"
#include "build-post.h"

/*
 * Appendix ID counter.
 */
static EB_Book_Code appendix_counter = 0;

/*
 * Mutex for `appendix_counter'.
 */
#ifdef ENABLE_PTHREAD
static pthread_mutex_t appendix_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Unexported functions.
 */
static EB_Error_Code eb_load_appendix_catalog EB_P((EB_Appendix *));


/*
 * Initialize alternation text cache in `appendix'.
 */
void
eb_initialize_alt_caches(appendix)
    EB_Appendix *appendix;
{
    EB_Alternation_Cache *p;
    int i;

    LOG(("in: eb_initialize_alt_caches(appendix=%d)", (int)appendix->code));

    for (i = 0, p = appendix->narrow_cache;
	 i < EB_MAX_ALTERNATION_CACHE; i++, p++)
	p->character_number = -1;
    for (i = 0, p = appendix->wide_cache;
	 i < EB_MAX_ALTERNATION_CACHE; i++, p++)
	p->character_number = -1;

    LOG(("out: eb_initialize_alt_caches()"));
}


/*
 * Finalize alternation text cache in `appendix'.
 */
void
eb_finalize_alt_caches(appendix)
    EB_Appendix *appendix;
{
    LOG(("in+out: eb_finalize_alt_caches(appendix=%d)", (int)appendix->code));

    /* nothing to be done */
}


/*
 * Initialize `appendix'.
 */
void
eb_initialize_appendix(appendix)
    EB_Appendix *appendix;
{
    LOG(("in: eb_initialize_appendix()"));

    appendix->code = EB_BOOK_NONE;
    appendix->path = NULL;
    appendix->path_length = 0;
    appendix->disc_code = EB_DISC_INVALID;
    appendix->subbook_count = 0;
    appendix->subbooks = NULL;
    appendix->subbook_current = NULL;
    eb_initialize_lock(&appendix->lock);
    eb_initialize_alt_caches(appendix);

    LOG(("out: eb_initialize_appendix()"));
}


/*
 * Finalize `appendix'.
 */
void
eb_finalize_appendix(appendix)
    EB_Appendix *appendix;
{
    LOG(("in: eb_finalize_appendix(appendix=%d)", (int)appendix->code));

    appendix->code = EB_BOOK_NONE;

    if (appendix->path != NULL) {
	free(appendix->path);
	appendix->path = NULL;
    }
    appendix->path_length = 0;

    appendix->disc_code = EB_DISC_INVALID;

    if (appendix->subbooks != NULL) {
	eb_finalize_appendix_subbooks(appendix);
	free(appendix->subbooks);
	appendix->subbooks = NULL;
	appendix->subbook_count = 0;
    }
    appendix->subbook_current = NULL;
    eb_finalize_lock(&appendix->lock);
    eb_finalize_alt_caches(appendix);

    LOG(("out: eb_finalize_appendix()"));
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
    char temporary_path[EB_MAX_PATH_LENGTH + 1];

    eb_lock(&appendix->lock);
    LOG(("in: eb_bind_appendix(path=%s)", path));

    /*
     * Reset structure members in the appendix.
     */
    if (appendix->path != NULL) {
	eb_finalize_appendix(appendix);
	eb_initialize_appendix(appendix);
    }

    /*
     * Assign a book code.
     */
    pthread_mutex_lock(&appendix_counter_mutex);
    appendix->code = appendix_counter++;
    pthread_mutex_unlock(&appendix_counter_mutex);

    /*
     * Set path of the appendix.
     * The length of the file name "path/subdir/subsubdir/file.;1" must
     * be EB_MAX_PATH_LENGTH maximum.
     */
    if (EB_MAX_PATH_LENGTH < strlen(path)) {
	error_code = EB_ERR_TOO_LONG_FILE_NAME;
	goto failed;
    }
    strcpy(temporary_path, path);
    error_code = eb_canonicalize_path_name(temporary_path);
    if (error_code != EB_SUCCESS)
	goto failed;
    appendix->path_length = strlen(temporary_path);

    if (EB_MAX_PATH_LENGTH
	< appendix->path_length + 1 + EB_MAX_DIRECTORY_NAME_LENGTH
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
    error_code = eb_load_appendix_catalog(appendix);
    if (error_code != EB_SUCCESS)
	goto failed;

    LOG(("out: eb_bind_appendix(appendix=%d) = %s", (int)appendix->code,
	eb_error_string(EB_SUCCESS)));
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_finalize_appendix(appendix);
    LOG(("out: eb_bind_appendix() = %s", eb_error_string(error_code)));
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Read information from the `CATALOG(S)' file in `appendix'.
 * Return EB_SUCCESS, if it succeeds, error-code ohtherwise.
 */
static EB_Error_Code
eb_load_appendix_catalog(appendix)
    EB_Appendix *appendix;
{
    EB_Error_Code error_code;
    char buffer[EB_SIZE_PAGE];
    char catalog_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char catalog_path_name[EB_MAX_PATH_LENGTH + 1];
    char *space;
    EB_Appendix_Subbook *subbook;
    size_t catalog_size;
    size_t title_size;
    Zio zio;
    Zio_Code zio_code;
    int i;

    LOG(("in: eb_load_appendix_catalog(appendix=%d)", (int)appendix->code));

    zio_initialize(&zio);

    /*
     * Find a catalog file.
     */
    if (eb_find_file_name(appendix->path, "catalog", catalog_file_name)
	== EB_SUCCESS) {
	appendix->disc_code = EB_DISC_EB;
	catalog_size = EB_SIZE_EB_CATALOG;
	title_size = EB_MAX_EB_TITLE_LENGTH;
    } else if (eb_find_file_name(appendix->path, "catalogs", catalog_file_name)
	== EB_SUCCESS) {
	appendix->disc_code = EB_DISC_EPWING;
	catalog_size = EB_SIZE_EPWING_CATALOG;
	title_size = EB_MAX_EPWING_TITLE_LENGTH;
    } else {
	error_code = EB_ERR_FAIL_OPEN_CATAPP;
	goto failed;
    }

    eb_compose_path_name(appendix->path, catalog_file_name, catalog_path_name);
    eb_path_name_zio_code(catalog_path_name, ZIO_PLAIN, &zio_code);

    /*
     * Open the catalog file.
     */
    if (zio_open(&zio, catalog_path_name, zio_code) < 0) {
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
    if (appendix->subbook_count == 0) {
	error_code = EB_ERR_UNEXP_CATAPP;
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
    eb_initialize_appendix_subbooks(appendix);

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
    }

    /*
     * Close the catalog file.
     */
    zio_close(&zio);
    zio_finalize(&zio);
    LOG(("out: eb_load_appendix_catalog() = %s", eb_error_string(EB_SUCCESS)));

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
    LOG(("out: eb_load_appendix_catalog() = %s", eb_error_string(error_code)));
    return error_code;
}


/*
 * Examine whether `appendix' is bound or not.
 */
int
eb_is_appendix_bound(appendix)
    EB_Appendix *appendix;
{
    int is_bound;

    eb_lock(&appendix->lock);
    LOG(("in: eb_is_appendix_bound(appendix=%d)", (int)appendix->code));

    is_bound = (appendix->path != NULL);

    LOG(("out: eb_is_appendix_bound() = %d", is_bound));
    eb_unlock(&appendix->lock);

    return is_bound;
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

    eb_lock(&appendix->lock);
    LOG(("in: eb_appendix_path(appendix=%d)", (int)appendix->code));

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

    LOG(("out: eb_appendix_path(path=%s) = %s",
	path, eb_error_string(EB_SUCCESS)));
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *path = '\0';
    LOG(("out: eb_appendix_path() = %s", eb_error_string(error_code)));
    eb_unlock(&appendix->lock);
    return error_code;
}


