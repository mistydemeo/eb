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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"

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
 * Get information about the current subbook.
 */
int
eb_initialize_multi_search(book)
    EB_Book *book;
{
    EB_Subbook *sub = book->sub_current;
    EB_Search *multi;
    EB_Multi_Entry *entry;
    char buf[EB_SIZE_PAGE];
    char *bufp;
    int indexcount;
    int id;
    int page;
    int i, j, k;

    for (i = 0, multi = sub->multi; i < sub->multi_count; i++, multi++) {
	/*
	 * Read the index table page of the multi search.
	 */
	if (eb_zlseek(&(sub->zip), sub->sub_file,
	    (multi->page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    eb_error = EB_ERR_FAIL_SEEK_START;
	    return -1;
	}
	if (eb_zread(&(sub->zip), sub->sub_file, buf, EB_SIZE_PAGE)
	    != EB_SIZE_PAGE) {
	    eb_error = EB_ERR_FAIL_READ_START;
	    return -1;
	}

	/*
	 * Get the number of entries in this multi search.
	 */
	multi->entry_count = eb_uint2(buf);
	if (EB_MAX_MULTI_SEARCHES <= multi->entry_count) {
	    eb_error = EB_ERR_UNEXP_START;
	    return -1;
	}

	bufp = buf + 16;
	for (j = 0, entry = multi->entries;
	     j < multi->entry_count; j++, entry++) {
	    /*
	     * Get the number of indexes in this entry, and title
	     * of this entry.
	     */
	    indexcount = eb_uint1(bufp);
	    strncpy(entry->label, bufp + 2, EB_MAXLEN_MULTI_LABEL);
	    entry->label[EB_MAXLEN_MULTI_LABEL] = '\0';
	    eb_jisx0208_to_euc(entry->label, entry->label);
	    bufp += EB_MAXLEN_MULTI_LABEL + 2;

	    /*
	     * Initialize index page information of the entry.
	     */
	    entry->page_word_asis = 0;
	    entry->page_endword_asis = 0;
	    entry->page_keyword = 0;

	    for (k = 0; k < indexcount; k++) {
		/*
		 * Get the index page information of the entry.
		 */
		id = eb_uint1(bufp);
		page = eb_uint4(bufp + 2);
		switch (id) {
		case 0x71:
		    entry->page_endword_asis = page;
		    break;
		case 0x91:
		    entry->page_word_asis = page;
		    break;
		case 0xa1:
		    entry->page_keyword = page;
		    break;
		}
		bufp += 16;
	    }
	}
    }

    return sub->multi_count;
}


/*
 * Examine whether the current subbook in `book' supports `MULTI SEARCH'
 * or not.
 */
int
eb_have_multi_search(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    if (book->sub_current->multi_count == 0) {
	eb_error = EB_ERR_NO_SUCH_SEARCH;
	return 0;
    }

    return 1;
}


/*
 * 
 */
int
eb_multi_search_count(book)
    EB_Book *book;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    return book->sub_current->multi_count;
}


/*
 * 
 */
int
eb_multi_search_list(book, list)
    EB_Book *book;
    EB_Multi_Search_Code *list;
{
    EB_Subbook_Code *lp;
    int i;

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    for (i = 0, lp = list; i < book->sub_current->multi_count; i++, lp++)
	*lp = i;

    return book->sub_current->multi_count;
}


/*
 * 
 */
int
eb_multi_entry_count(book, multi_id)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->sub_current->multi_count <= multi_id) {
	eb_error = EB_ERR_NO_SUCH_MULTI_ID;
	return -1;
    }

    return book->sub_current->multi[multi_id].entry_count;
}


/*
 * 
 */
int
eb_multi_entry_list(book, multi_id, list)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code *list;
{
    EB_Subbook_Code *lp;
    int entry_count;
    int i;

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->sub_current->multi_count <= multi_id) {
	eb_error = EB_ERR_NO_SUCH_MULTI_ID;
	return -1;
    }

    entry_count = book->sub_current->multi[multi_id].entry_count;
    for (i = 0, lp = list; i < entry_count; i++, lp++)
	*lp = i;

    return entry_count;
}


/*
 * 
 */
const char *
eb_multi_entry_label(book, multi_id, entry_id)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return NULL;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return NULL;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->sub_current->multi_count <= multi_id) {
	eb_error = EB_ERR_NO_SUCH_MULTI_ID;
	return NULL;
    }

    /*
     * `entry_id' must be a valid code.
     */
    if (entry_id < 0
	|| book->sub_current->multi[multi_id].entry_count <= entry_id) {
	eb_error = EB_ERR_NO_SUCH_ENTRY_ID;
	return NULL;
    }

    return book->sub_current->multi[multi_id].entries[entry_id].label;
}


/*
 * 
 */
int
eb_multi_entry_have_exactword_search(book, multi_id, entry_id)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
{
    return eb_multi_entry_have_word_search(book, multi_id, entry_id);
}

/*
 * 
 */
int
eb_multi_entry_have_word_search(book, multi_id, entry_id)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
{
    EB_Search *multi;

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return 0;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->sub_current->multi_count <= multi_id) {
	eb_error = EB_ERR_NO_SUCH_MULTI_ID;
	return 0;
    }

    /*
     * `entry_id' must be a valid code.
     */
    multi = book->sub_current->multi + multi_id;
    if (entry_id < 0 || multi->entry_count <= entry_id) {
	eb_error = EB_ERR_NO_SUCH_ENTRY_ID;
	return 0;
    }

    return (multi->entries[entry_id].page_word_asis != 0);
}


/*
 * 
 */
int
eb_multi_entry_have_endword_search(book, multi_id, entry_id)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
{
    EB_Search *multi;

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return 0;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->sub_current->multi_count <= multi_id) {
	eb_error = EB_ERR_NO_SUCH_MULTI_ID;
	return 0;
    }

    /*
     * `entry_id' must be a valid code.
     */
    multi = book->sub_current->multi + multi_id;
    if (entry_id < 0 || multi->entry_count <= entry_id) {
	eb_error = EB_ERR_NO_SUCH_ENTRY_ID;
	return 0;
    }

    return (multi->entries[entry_id].page_endword_asis != 0);
}


/*
 * 
 */
int
eb_multi_entry_have_keyword_search(book, multi_id, entry_id)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
{
    EB_Search *multi;

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return 0;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->sub_current->multi_count <= multi_id) {
	eb_error = EB_ERR_NO_SUCH_MULTI_ID;
	return 0;
    }

    /*
     * `entry_id' must be a valid code.
     */
    multi = book->sub_current->multi + multi_id;
    if (entry_id < 0 || multi->entry_count <= entry_id) {
	eb_error = EB_ERR_NO_SUCH_ENTRY_ID;
	return 0;
    }

    return (multi->entries[entry_id].page_keyword != 0);
}


