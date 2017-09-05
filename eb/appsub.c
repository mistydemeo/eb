/*
 * Copyright (c) 1997, 98, 2000, 01
 *    Motoyuki Kasahara
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
#include "internal.h"
#include "appendix.h"

/*
 * Unexported functions.
 */
static EB_Error_Code eb_initialize_appendix_subbook EB_P((EB_Appendix *));
static EB_Error_Code eb_set_appendix_subbook_eb EB_P((EB_Appendix *,
    EB_Subbook_Code));
static EB_Error_Code eb_set_appendix_subbook_epwing EB_P((EB_Appendix *,
    EB_Subbook_Code));

/*
 * Initialize all subbooks in `appendix'.
 */
static EB_Error_Code
eb_initialize_appendix_subbook(appendix)
    EB_Appendix *appendix;
{
    EB_Error_Code error_code;
    EB_Appendix_Subbook *subbook;
    char buffer[16];
    int stop_code_page;
    int character_count;

    subbook = appendix->subbook_current;

    /*
     * Check for the current status.
     */
    if (subbook == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    /*
     * If the subbook has already initialized, return immediately.
     */
    if (subbook->initialized != 0)
	goto succeeded;

    /*
     * Rewind the APPENDIX file.
     */
    if (zio_lseek(&subbook->zio, (off_t)0, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_APP;
	goto failed;
    }

    /*
     * Set character code used in the appendix.
     */
    if (zio_read(&subbook->zio, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }
    subbook->character_code = eb_uint2(buffer + 2);

    /*
     * Set information about alternation text of wide font.
     */
    if (zio_read(&subbook->zio, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }
    character_count = eb_uint2(buffer + 12);
    subbook->narrow_page = eb_uint4(buffer);
    subbook->narrow_start = eb_uint2(buffer + 10);
    subbook->narrow_end = subbook->narrow_start
	+ ((character_count / 0x5e) << 8) + (character_count % 0x5e) - 1;
    if (0x7e < (subbook->narrow_end & 0xff))
	subbook->narrow_end += 0xa3;

    /*
     * Set information about alternation text of wide font.
     */
    if (zio_read(&subbook->zio, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }
    character_count = eb_uint2(buffer + 12);
    subbook->wide_page = eb_uint4(buffer);
    subbook->wide_start = eb_uint2(buffer + 10);
    subbook->wide_end = subbook->wide_start
	+ ((character_count / 0x5e) << 8) + (character_count % 0x5e) - 1;
    if (0x7e < (subbook->wide_end & 0xff))
	subbook->wide_end += 0xa3;
    
    /*
     * Set stop-code.
     */
    if (zio_read(&subbook->zio, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }
    stop_code_page = eb_uint4(buffer);
    if (zio_lseek(&subbook->zio, (off_t)(stop_code_page - 1) * EB_SIZE_PAGE,
	SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_APP;
	goto failed;
    }
    if (zio_read(&subbook->zio, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }
    if (eb_uint2(buffer) != 0) {
	subbook->stop0 = eb_uint2(buffer + 2);
	subbook->stop1 = eb_uint2(buffer + 4);
    } else {
	subbook->stop0 = 0;
	subbook->stop1 = 0;
    }

    /*
     * Rewind the file descriptor, again.
     */
    if (zio_lseek(&subbook->zio, (off_t)0, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_APP;
	goto failed;
    }

    /*
     * Initialize the alternation text cache.
     */
    eb_initialize_alt_cache(appendix);

  succeeded:
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
}


/*
 * Initialize all subbooks in the book.
 */
EB_Error_Code
eb_initialize_all_appendix_subbooks(appendix)
    EB_Appendix *appendix;
{
    EB_Error_Code error_code;
    EB_Subbook_Code current_subbook_code;
    EB_Appendix_Subbook *subbook;
    int i;

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	error_code = EB_ERR_UNBOUND_APP;
	goto failed;
    }

    /*
     * Get the current subbook.
     */
    if (appendix->subbook_current != NULL)
	current_subbook_code = appendix->subbook_current->code;
    else
	current_subbook_code = -1;

    /*
     * Initialize each subbook.
     */
    for (i = 0, subbook = appendix->subbooks;
	 i < appendix->subbook_count; i++, subbook++) {
	error_code = eb_set_appendix_subbook(appendix, subbook->code);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }

    /*
     * Restore the current subbook.
     */
    if (current_subbook_code < 0)
	eb_unset_appendix_subbook(appendix);
    else {
	error_code = eb_set_appendix_subbook(appendix, current_subbook_code);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Get a subbook list in `appendix'.
 */
EB_Error_Code
eb_appendix_subbook_list(appendix, subbook_list, subbook_count)
    EB_Appendix *appendix;
    EB_Subbook_Code *subbook_list;
    int *subbook_count;
{
    EB_Error_Code error_code;
    EB_Subbook_Code *list_p;
    int i;

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
     * Make a subbook list.
     */
    for (i = 0, list_p = subbook_list; i < appendix->subbook_count;
	 i++, list_p++)
	*list_p = i;
    *subbook_count = appendix->subbook_count;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *subbook_count = 0;
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Get the subbook-code of the current subbook in `appendix'.
 */
EB_Error_Code
eb_appendix_subbook(appendix, subbook_code)
    EB_Appendix *appendix;
    EB_Subbook_Code *subbook_code;
{
    EB_Error_Code error_code;

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Check for the current status.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    /*
     * Copy the current subbook code to `subbook_code'.
     */
    *subbook_code = appendix->subbook_current->code;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *subbook_code = EB_SUBBOOK_INVALID;
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Get the directory name of the current subbook in `appendix'.
 */
EB_Error_Code
eb_appendix_subbook_directory(appendix, directory)
    EB_Appendix *appendix;
    char *directory;
{
    EB_Error_Code error_code;

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Check for the current status.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    /*
     * Copy the directory name to `directory'.
     */
    strcpy(directory, appendix->subbook_current->directory_name);

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *directory = '\0';
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Get the directory name of the subbook `subbook_code' in `appendix'.
 */
EB_Error_Code
eb_appendix_subbook_directory2(appendix, subbook_code, directory)
    EB_Appendix *appendix;
    EB_Subbook_Code subbook_code;
    char *directory;
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
     * Check for `subbook_code'.
     */
    if (subbook_code < 0 || appendix->subbook_count <= subbook_code) {
	error_code = EB_ERR_NO_SUCH_APPSUB;
	goto failed;
    }

    /*
     * Copy the directory name to `directory'.
     */
    strcpy(directory, (appendix->subbooks + subbook_code)->directory_name);

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *directory = '\0';
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Set the subbook `subbook_code' as the current subbook.
 */
EB_Error_Code
eb_set_appendix_subbook(appendix, subbook_code)
    EB_Appendix *appendix;
    EB_Subbook_Code subbook_code;
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
     * Check for `subbook_code'.
     */
    if (subbook_code < 0 || appendix->subbook_count <= subbook_code) {
	error_code = EB_ERR_NO_SUCH_APPSUB;
	goto failed;
    }

    /*
     * If the current subbook is `subbook_code', return immediately.
     * Otherwise close the current subbook and continue.
     */
    if (appendix->subbook_current != NULL) {
	if (appendix->subbook_current->code == subbook_code)
	    goto succeeded;
	eb_unset_appendix_subbook(appendix);
    }

    /*
     * Disc type specific section.
     */
    if (appendix->disc_code == EB_DISC_EB)
	error_code = eb_set_appendix_subbook_eb(appendix, subbook_code);
    else
	error_code = eb_set_appendix_subbook_epwing(appendix, subbook_code);

    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Initialize the subbook.
     */
    error_code = eb_initialize_appendix_subbook(appendix);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Unlock the appendix.
     */
  succeeded:
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_appendix_subbook(appendix);
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * EB* specific section of eb_set_appendix_subbook().
 */
static EB_Error_Code
eb_set_appendix_subbook_eb(appendix, subbook_code)
    EB_Appendix *appendix;
    EB_Subbook_Code subbook_code;
{
    EB_Error_Code error_code;
    EB_Appendix_Subbook *subbook;
    char appendix_path_name[PATH_MAX + 1];
    Zio_Code zio_code;

    /*
     * Set the current subbook.
     */
    appendix->subbook_current = appendix->subbooks + subbook_code;
    subbook = appendix->subbook_current;

    /*
     * Open an appendix file.
     */
    if (eb_find_file_name2(appendix->path, subbook->directory_name,
	"appendix", subbook->file_name) != EB_SUCCESS) {
	error_code = EB_ERR_FAIL_OPEN_APP;
	goto failed;
    }

    eb_compose_path_name2(appendix->path, subbook->directory_name,
	subbook->file_name, appendix_path_name);
    eb_path_name_zio_code(appendix_path_name, ZIO_PLAIN, &zio_code);

    if (zio_open(&subbook->zio, appendix_path_name, zio_code) < 0) {
	error_code = EB_ERR_FAIL_OPEN_APP;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_appendix_subbook(appendix);
    return error_code;
}


/*
 * EPWING specific section of eb_set_appendix_subbook().
 */
static EB_Error_Code
eb_set_appendix_subbook_epwing(appendix, subbook_code)
    EB_Appendix *appendix;
    EB_Subbook_Code subbook_code;
{
    EB_Error_Code error_code;
    EB_Appendix_Subbook *subbook;
    char appendix_path_name[PATH_MAX + 1];
    Zio_Code zio_code;

    /*
     * Set the current subbook.
     */
    appendix->subbook_current = appendix->subbooks + subbook_code;
    subbook = appendix->subbook_current;

    zio_initialize(&subbook->zio);

    /*
     * Adjust a directory name.
     */
    strcpy(subbook->data_directory_name, EB_DIRECTORY_NAME_DATA);
    eb_fix_directory_name2(appendix->path, subbook->directory_name,
	subbook->data_directory_name);

    /*
     * Open an appendix file.
     */
    if (eb_find_file_name3(appendix->path, subbook->directory_name,
	subbook->data_directory_name, "furoku", subbook->file_name)
	!= EB_SUCCESS) {
	error_code = EB_ERR_FAIL_OPEN_APP;
	goto failed;
    }

    eb_compose_path_name3(appendix->path, subbook->directory_name,
	subbook->data_directory_name, subbook->file_name, 
	appendix_path_name);
    eb_path_name_zio_code(appendix_path_name, ZIO_PLAIN, &zio_code);

    if (zio_open(&subbook->zio, appendix_path_name, zio_code) < 0) {
	subbook = NULL;
	error_code = EB_ERR_FAIL_OPEN_APP;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_appendix_subbook(appendix);
    return error_code;
}


/*
 * Unset the current subbook.
 */
void
eb_unset_appendix_subbook(appendix)
    EB_Appendix *appendix;
{
    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Close a file for the current subbook.
     */
    if (appendix->subbook_current != NULL) {
	zio_close(&appendix->subbook_current->zio);
	appendix->subbook_current = NULL;
    }

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);
}


