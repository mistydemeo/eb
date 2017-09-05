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

#ifdef HAVE_LIMITS_H
#include <limits.h>
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
 * Read information from the subbooks in `appendix'.
 */
int
eb_initialize_appendix_subbook(appendix)
    EB_Appendix *appendix;
{
    char buf[16];
    int stoppage;
    int len;

    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return -1;
    }

    /*
     * If the subbook has already initialized, return immediately.
     */
    if (appendix->sub_current->initialized != 0)
	return 0;

    /*
     * Rewind the APPENDIX file.
     */
    if (eb_zlseek(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, 0, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_APP;
	return -1;
    }

    /*
     * Set general information about appendix the subbook.
     */
    if (eb_zread(&(appendix->sub_current->zip),
	appendix->sub_current->sub_file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_APP;
	return -1;
    }
    appendix->sub_current->char_code = eb_uint2(buf + 2);

    /*
     * Set information about alternation text for a narrow font.
     */
    if (eb_zread(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_APP;
	return -1;
    }
    len = eb_uint2(buf + 12);
    appendix->sub_current->narw_page = eb_uint4(buf);
    appendix->sub_current->narw_start = eb_uint2(buf + 10);
    appendix->sub_current->narw_end = appendix->sub_current->narw_start
	+ ((len / 0x5e) << 8) + (len % 0x5e) - 1;
    if (0x7e < (appendix->sub_current->narw_end & 0xff))
	appendix->sub_current->narw_end += 0xa3;

    /*
     * Set information about alternation text for a wide font.
     */
    if (eb_zread(&(appendix->sub_current->zip),
	appendix->sub_current->sub_file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_APP;
	return -1;
    }
    len = eb_uint2(buf + 12);
    appendix->sub_current->wide_page = eb_uint4(buf);
    appendix->sub_current->wide_start = eb_uint2(buf + 10);
    appendix->sub_current->wide_end = appendix->sub_current->wide_start
	+ ((len / 0x5e) << 8) + (len % 0x5e) - 1;
    if (0x7e < (appendix->sub_current->wide_end & 0xff))
	appendix->sub_current->wide_end += 0xa3;
    
    /*
     * Set a stop-code;
     */
    if (eb_zread(&(appendix->sub_current->zip),
	appendix->sub_current->sub_file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_APP;
	return -1;
    }
    stoppage = eb_uint4(buf);
    if (eb_zlseek(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, (stoppage - 1) * EB_SIZE_PAGE,
	SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_APP;
	return -1;
    }
    if (eb_zread(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_APP;
	return -1;
    }
    if (eb_uint2(buf) != 0) {
	appendix->sub_current->stop0 = eb_uint2(buf + 2);
	appendix->sub_current->stop1 = eb_uint2(buf + 4);
    } else {
	appendix->sub_current->stop0 = 0;
	appendix->sub_current->stop1 = 0;
    }

    /*
     * Rewind the file descriptor of the APPENDIX file.
     */
    if (eb_zlseek(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, 0, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_APP;
	return -1;
    }

    eb_initialize_alt_cache(appendix);

    return 0;
}


/*
 * Get information about all subbooks in the book.
 */
int
eb_initialize_all_appendix_subbooks(appendix)
    EB_Appendix *appendix;
{
    EB_Subbook_Code cur;
    EB_Appendix_Subbook *sub;
    int i;

    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	return -1;
    }

    /*
     * Get the current subbook.
     */
    if (appendix->sub_current != NULL)
	cur = appendix->sub_current->code;
    else
	cur = -1;

    /*
     * Initialize each subbook.
     */
    for (i = 0, sub = appendix->subbooks;
	 i < appendix->sub_count; i++, sub++) {
	if (eb_set_appendix_subbook(appendix, sub->code) < 0)
	    return -1;
    }

    /*
     * Restore the current subbook.
     */
    if (cur < 0)
	eb_unset_appendix_subbook(appendix);
    else if (eb_set_appendix_subbook(appendix, cur) < 0)
	return -1;

    return 0;
}


/*
 * Return the number of subbooks in `appendix'.
 */
int
eb_appendix_subbook_count(appendix)
    EB_Appendix *appendix;
{
    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	return -1;
    }

    return appendix->sub_count;
}


/*
 * Make a subbook list in `appendix'.
 */
int
eb_appendix_subbook_list(appendix, list)
    EB_Appendix *appendix;
    EB_Subbook_Code *list;
{
    EB_Subbook_Code *lp;
    int i;

    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	return -1;
    }

    for (i = 0, lp = list; i < appendix->sub_count; i++, lp++)
	*lp = i;

    return appendix->sub_count;
}


/*
 * Return the subbook-code of the current subbook in `appendix'.
 */
EB_Subbook_Code
eb_appendix_subbook(appendix)
    EB_Appendix *appendix;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return -1;
    }

    return appendix->sub_current->code;
}


/*
 * Return a subbook-code of the subbook which contains the directory
 * name `dirname'.
 */
EB_Subbook_Code
eb_appendix_subbook2(appendix, dirname)
    EB_Appendix *appendix;
    const char *dirname;
{
    EB_Appendix_Subbook *sub;
    int i;

    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	return -1;
    }

    for (i = 0, sub = appendix->subbooks;
	 i < appendix->sub_count; i++, sub++) {
	if (strcmp(sub->directory, dirname) == 0)
	    return sub->code;
    }

    eb_error = EB_ERR_NO_SUCH_APPSUB;
    return -1;
}


/*
 * Return the directory name of the current subbook in `appendix'.
 */
const char *
eb_appendix_subbook_directory(appendix)
    EB_Appendix *appendix;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return NULL;
    }

    return appendix->sub_current->directory;
}


/*
 * Return the directory name of the subbook `code' in `appendix'.
 */
const char *
eb_appendix_subbook_directory2(appendix, code)
    EB_Appendix *appendix;
    EB_Subbook_Code code;
{
    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	return NULL;
    }

    /*
     * Check for the subbook-code.
     */
    if (code < 0 || appendix->sub_count <= code) {
	eb_error = EB_ERR_NO_SUCH_APPSUB;
	return NULL;
    }

    return (appendix->subbooks + code)->directory;
}


/*
 * Set the subbook `code' as the current subbook.
 */
int
eb_set_appendix_subbook(appendix, code)
    EB_Appendix *appendix;
    EB_Subbook_Code code;
{
    char filename[PATH_MAX + 1];

    /*
     * The appendix must have been bound.
     */
    if (appendix->path == NULL) {
	eb_error = EB_ERR_UNBOUND_APP;
	goto failed;
    }

    /*
     * Check for the subbook-code.
     */
    if (code < 0 || appendix->sub_count <= code) {
	eb_error = EB_ERR_NO_SUCH_APPSUB;
	goto failed;
    }

    /*
     * If the current subbook is the subbook with `code', return
     * immediately.
     * Otherwise close the current subbook and continue.
     */
    if (appendix->sub_current != NULL) {
	if (appendix->sub_current->code == code)
	    return 0;
	eb_unset_appendix_subbook(appendix);
    }

    /*
     * Open the subbook.
     */
    appendix->sub_current = appendix->subbooks + code;
    if (appendix->disc_code == EB_DISC_EB) {
	sprintf(filename, "%s/%s/%s", appendix->path,
	    appendix->sub_current->directory, EB_FILENAME_APPENDIX);
    } else {
	sprintf(filename, "%s/%s/%s/%s", appendix->path,
	    appendix->sub_current->directory, EB_DIRNAME_DATA,
	    EB_FILENAME_FUROKU);
    }
    eb_fix_appendix_filename(appendix, filename);
    appendix->sub_current->sub_file =
	eb_zopen(&(appendix->sub_current->zip), filename);
    if (appendix->sub_current->sub_file < 0) {
	eb_error = EB_ERR_FAIL_OPEN_APP;
	appendix->sub_current = NULL;
	goto failed;
    }

    /*
     * Initialize the subbook.
     */
    if (eb_initialize_appendix_subbook(appendix) < 0)
	goto failed;

    return 0;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_appendix_subbook(appendix);
    return -1;
}


/*
 * Unset the current subbook.
 */
void
eb_unset_appendix_subbook(appendix)
    EB_Appendix *appendix;
{
    /*
     * Close the file of the current subbook.
     */
    if (appendix->sub_current != NULL) {
	eb_zclose(&(appendix->sub_current->zip),
	    appendix->sub_current->sub_file);
	appendix->sub_current = NULL;
    }
}


