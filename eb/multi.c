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
EB_Error_Code
eb_initialize_multi_search(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook *sub = book->subbook_current;
    EB_Multi_Search *multi;
    EB_Search *entry;
    char buffer[EB_SIZE_PAGE];
    char *bufp;
    int index_count;
    int index_id;
    int page;
    int i, j, k;

    for (i = 0, multi = sub->multis; i < sub->multi_count; i++, multi++) {
	/*
	 * Read the index table page of the multi search.
	 */
	if (eb_zlseek(&sub->text_zip, sub->text_file,
	    (multi->search.index_page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_TEXT;
	    goto failed;
	}
	if (eb_zread(&sub->text_zip, sub->text_file, buffer, EB_SIZE_PAGE)
	    != EB_SIZE_PAGE) {
	    error_code = EB_ERR_FAIL_READ_TEXT;
	    goto failed;
	}

	/*
	 * Get the number of entries in this multi search.
	 */
	multi->entry_count = eb_uint2(buffer);
	if (EB_MAX_MULTI_SEARCHES <= multi->entry_count) {
	    error_code = EB_ERR_UNEXP_TEXT;
	    goto failed;
	}

	bufp = buffer + 16;
	for (j = 0, entry = multi->entries;
	     j < multi->entry_count; j++, entry++) {
	    /*
	     * Get the number of indexes in this entry, and title
	     * of this entry.
	     */
	    index_count = eb_uint1(bufp);
	    strncpy(entry->label, bufp + 2, EB_MAX_MULTI_LABEL_LENGTH);
	    entry->label[EB_MAX_MULTI_LABEL_LENGTH] = '\0';
	    eb_jisx0208_to_euc(entry->label, entry->label);
	    bufp += EB_MAX_MULTI_LABEL_LENGTH + 2;

	    /*
	     * Initialize index page information of the entry.
	     */
	    entry->index_page = 0;
	    entry->candidates_page = 0;

	    for (k = 0; k < index_count; k++) {
		/*
		 * Get the index page information of the entry.
		 */
		index_id = eb_uint1(bufp);
		page = eb_uint4(bufp + 2);
		switch (index_id) {
		case 0x71:
		    if (entry->index_page == 0)
			entry->index_page = page;
		    break;
		case 0x91:
		case 0xa1:
		    entry->index_page = page;
		    break;
		case 0x01:
		    entry->candidates_page = page;
		    break;
		}
		bufp += 16;
	    }
	}
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
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
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL)
	goto failed;

    if (book->subbook_current->multi_count == 0)
	goto failed;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&book->lock);
    return 0;
}


/*
 * Return a list of multi search ids in `book'.
 */
EB_Error_Code
eb_multi_search_list(book, search_list, search_count)
    EB_Book *book;
    EB_Multi_Search_Code *search_list;
    int *search_count;
{
    EB_Error_Code error_code;
    EB_Subbook_Code *listp;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	error_code = EB_ERR_UNBOUND_BOOK;
	goto failed;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    *search_count = book->subbook_current->multi_count;
    for (i = 0, listp = search_list; i < *search_count; i++, listp++)
	*listp = i;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *search_count = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a list of entries that the multi search `multi_id' in `book' has.
 */
EB_Error_Code
eb_multi_entry_list(book, multi_id, entry_list, entry_count)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code *entry_list;
    int *entry_count;
{
    EB_Error_Code error_code;
    EB_Subbook_Code *listp;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	error_code = EB_ERR_UNBOUND_BOOK;
	goto failed;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->subbook_current->multi_count <= multi_id) {
	error_code = EB_ERR_NO_SUCH_MULTI_ID;
	goto failed;
    }

    *entry_count = book->subbook_current->multis[multi_id].entry_count;
    for (i = 0, listp = entry_list; i < *entry_count; i++, listp++)
	*listp = i;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *entry_count = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a lable of the entry `entry_id' in the multi search `multi_id'.
 */
EB_Error_Code
eb_multi_entry_label(book, multi_id, entry_id, label)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
    char *label;
{
    EB_Error_Code error_code;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	error_code = EB_ERR_UNBOUND_BOOK;
	goto failed;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->subbook_current->multi_count <= multi_id) {
	error_code = EB_ERR_NO_SUCH_MULTI_ID;
	goto failed;
    }

    /*
     * `entry_id' must be a valid code.
     */
    if (entry_id < 0
	|| book->subbook_current->multis[multi_id].entry_count <= entry_id) {
	error_code = EB_ERR_NO_SUCH_ENTRY_ID;
	goto failed;
    }

    strcpy(label,
	book->subbook_current->multis[multi_id].entries[entry_id].label);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *label = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Whether the entry `entry_id' in the multi search `multi_id' has
 * candidates or not.
 */
int
eb_multi_entry_have_candidates(book, multi_id, entry_id)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
{
    EB_Multi_Search *multi;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * The book must have been bound.
     */
    if (book->path == NULL)
	goto failed;

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL)
	goto failed;

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->subbook_current->multi_count <= multi_id)
	goto failed;

    /*
     * `entry_id' must be a valid code.
     */
    multi = book->subbook_current->multis + multi_id;
    if (entry_id < 0 || multi->entry_count <= entry_id)
	goto failed;

    if (multi->entries[entry_id].candidates_page == 0)
	goto failed;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&book->lock);
    return 0;
}


/*
 * Return a position of candidates for the entry `entry_id' in the multi
 * search `multi_id'.
 */
EB_Error_Code
eb_multi_entry_candidates(book, multi_id, entry_id, position)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    EB_Multi_Entry_Code entry_id;
    EB_Position *position;
{
    EB_Error_Code error_code;
    EB_Multi_Search *multi;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	error_code = EB_ERR_UNBOUND_BOOK;
	goto failed;
    }

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * `multi_id' must be a valid code.
     */
    if (multi_id < 0 || book->subbook_current->multi_count <= multi_id) {
	error_code = EB_ERR_NO_SUCH_MULTI_ID;
	goto failed;
    }

    /*
     * `entry_id' must be a valid code.
     */
    multi = book->subbook_current->multis + multi_id;
    if (entry_id < 0 || multi->entry_count <= entry_id) {
	error_code = EB_ERR_NO_SUCH_ENTRY_ID;
	goto failed;
    }

    if (multi->entries[entry_id].candidates_page == 0) {
	error_code = EB_ERR_NO_SUCH_SEARCH;
	goto failed;
    }

    position->page = multi->entries[entry_id].candidates_page;
    position->offset = 0;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Multi search.
 */
EB_Error_Code
eb_search_multi(book, multi_id, input_words)
    EB_Book *book;
    EB_Multi_Search_Code multi_id;
    const char * const input_words[];
{
    EB_Error_Code error_code;
    EB_Search_Context *context;
    EB_Search *entry;
    EB_Word_Code word_code;
    int word_count;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Check whether the current subbook has keyword search.
     */
    if (multi_id < 0 || book->subbook_current->multi_count <= multi_id) {
	error_code = EB_ERR_NO_SUCH_SEARCH;
	goto failed;
    }

    /*
     * Attach a search context for each keyword, and pre-search the
     * keywords.
     */
    word_count = 0;
    for (i = 0, entry = book->subbook_current->multis[multi_id].entries;
	 i < book->subbook_current->multis[multi_id].entry_count;
	 i++, entry++) {

	if (input_words[i] == NULL)
	    break;

	/*
	 * Initialize search context.
	 */
	context = book->search_contexts + word_count;
	context->code = EB_SEARCH_MULTI;
	context->compare = eb_match_exactword;
	context->page = entry->index_page;
	if (context->page == 0)
	    continue;

	/*
	 * Make a fixed word and a canonicalized word to search from
	 * `input_words[i]'.
	 */
	error_code = eb_set_multiword(book, multi_id, i, input_words[i],
	    context->word, context->canonicalized_word, &word_code);
	if (error_code == EB_ERR_EMPTY_WORD)
	    continue;
	else if (error_code != EB_SUCCESS)
	    goto failed;

	/*
	 * Pre-search.
	 */
	error_code = eb_presearch_word(book, context);
	if (error_code != EB_SUCCESS)
	    goto failed;

	word_count++;
    }
    if (word_count == 0) {
	error_code = EB_ERR_NO_WORD;
	goto failed;
    } else if (book->subbook_current->multis[multi_id].entry_count <= i
	&& input_words[i] != NULL) {
	error_code =  EB_ERR_TOO_MANY_WORDS;
	goto failed;
    }

    /*
     * Set `EB_SEARCH_NONE' to the rest unused search context.
     */
    for (i = word_count; i < EB_MAX_KEYWORDS; i++)
	(book->search_contexts + i)->code = EB_SEARCH_NONE;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    book->search_contexts->code = EB_SEARCH_NONE;
    eb_unlock(&book->lock);
    return error_code;
}


