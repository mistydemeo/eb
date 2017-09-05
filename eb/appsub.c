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
 * Initialize all subbooks in `appendix'.
 */
EB_Error_Code
eb_initialize_appendix_subbook(appendix)
    EB_Appendix *appendix;
{
    EB_Error_Code error_code;
    EB_Appendix_Subbook *subbook = appendix->subbook_current;
    char buffer[16];
    int stop_code_page;
    int character_count;

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
     * Adjust a directory name.
     */
    if (appendix->disc_code == EB_DISC_EPWING) {
	strcpy(subbook->data_directory_name, EB_DIRECTORY_NAME_DATA);
	eb_fix_directory_name2(appendix->path, subbook->directory_name,
	    subbook->data_directory_name);
    }

    /*
     * Rewind the APPENDIX file.
     */
    if (eb_zlseek(&subbook->appendix_zip, subbook->appendix_file, 0, SEEK_SET)
	< 0) {
	error_code = EB_ERR_FAIL_SEEK_APP;
	goto failed;
    }

    /*
     * Set character code used in the appendix.
     */
    if (eb_zread(&subbook->appendix_zip, subbook->appendix_file, buffer, 16)
	!= 16) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }
    subbook->character_code = eb_uint2(buffer + 2);

    /*
     * Set information about alternation text of wide font.
     */
    if (eb_zread(&subbook->appendix_zip, subbook->appendix_file, buffer, 16)
	!= 16) {
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
    if (eb_zread(&subbook->appendix_zip, subbook->appendix_file, buffer, 16)
	!= 16) {
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
    if (eb_zread(&subbook->appendix_zip, subbook->appendix_file, buffer, 16)
	!= 16) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }
    stop_code_page = eb_uint4(buffer);
    if (eb_zlseek(&subbook->appendix_zip, subbook->appendix_file,
	(stop_code_page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_APP;
	goto failed;
    }
    if (eb_zread(&subbook->appendix_zip, subbook->appendix_file, buffer, 16)
	!= 16) {
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
    if (eb_zlseek(&subbook->appendix_zip, subbook->appendix_file, 0, SEEK_SET)
	< 0) {
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
    EB_Appendix_Subbook *subbook;
    char appendix_path_name[PATH_MAX + 1];

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
     * Set the current subbook.
     */
    appendix->subbook_current = appendix->subbooks + subbook_code;
    subbook = appendix->subbook_current;

    /*
     * Open an appendix file.
     */
    subbook->appendix_file = -1;
    if (appendix->disc_code == EB_DISC_EB) {
	if (eb_compose_path_name2(appendix->path, subbook->directory_name, 
	    EB_FILE_NAME_APPENDIX, EB_SUFFIX_NONE, appendix_path_name) == 0) {
	    subbook->appendix_file = eb_zopen_none(&subbook->appendix_zip,
		appendix_path_name);
	} else if (eb_compose_path_name2(appendix->path,
	    subbook->directory_name, EB_FILE_NAME_APPENDIX, EB_SUFFIX_EBZ,
	    appendix_path_name) == 0) {
	    subbook->appendix_file = eb_zopen_ebzip(&subbook->appendix_zip,
		appendix_path_name);
	}
    } else {
	if (eb_compose_path_name3(appendix->path, subbook->directory_name,
	    subbook->data_directory_name, EB_FILE_NAME_FUROKU, EB_SUFFIX_NONE,
	    appendix_path_name) == 0) {
	    subbook->appendix_file = eb_zopen_none(&subbook->appendix_zip, 
		appendix_path_name);
	} else if (eb_compose_path_name3(appendix->path,
	    subbook->directory_name, subbook->data_directory_name,
	    EB_FILE_NAME_FUROKU, EB_SUFFIX_EBZ, appendix_path_name) == 0) {
	    subbook->appendix_file = eb_zopen_ebzip(&subbook->appendix_zip,
		appendix_path_name);
	}
    }

    if (subbook->appendix_file < 0) {
	subbook = NULL;
	error_code = EB_ERR_FAIL_OPEN_APP;
	goto failed;
    }

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
	eb_zclose(&appendix->subbook_current->appendix_zip,
	    appendix->subbook_current->appendix_file);
	appendix->subbook_current = NULL;
    }

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);
}


