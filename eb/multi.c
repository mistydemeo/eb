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

#include "build-pre.h"
#include "eb.h"
#include "error.h"
#include "build-post.h"

/*
 * Initialize all multi searches in the current subbook.
 */
void
eb_initialize_multi_searches(book)
    EB_Book *book;
{
    EB_Subbook *subbook;
    EB_Multi_Search *multi;
    EB_Search *entry;
    int i, j;

    LOG(("in: eb_initialize_multi_searches(book=%d)", (int)book->code));

    subbook = book->subbook_current;

    for (i = 0, multi = subbook->multis; i < EB_MAX_KEYWORDS;
	 i++, multi++) {
	eb_initialize_search(&multi->search);
	multi->entry_count = 0;
	for (j = 0, entry = multi->entries;
	     j < multi->entry_count; j++, entry++) {
	    eb_initialize_search(entry);
	}
    }

    LOG(("out: eb_initialize_multi_searches()"));
}


/*
 * Finalize all multi searches in the current subbook.
 */
void
eb_finalize_multi_searches(book)
    EB_Book *book;
{
    EB_Subbook *subbook;
    EB_Multi_Search *multi;
    EB_Search *entry;
    int i, j;

    LOG(("in: eb_finalize_multi_searches(book=%d)", (int)book->code));

    subbook = book->subbook_current;

    for (i = 0, multi = subbook->multis; i < EB_MAX_KEYWORDS;
	 i++, multi++) {
	eb_finalize_search(&multi->search);
	multi->entry_count = 0;
	for (j = 0, entry = multi->entries;
	     j < multi->entry_count; j++, entry++) {
	    eb_finalize_search(entry);
	}
    }

    LOG(("out: eb_finalize_multi_searches()"));
}


/*
 * Get information about the current subbook.
 */
EB_Error_Code
eb_load_multi_searches(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook;
    EB_Multi_Search *multi;
    EB_Search *entry;
    char buffer[EB_SIZE_PAGE];
    char *buffer_p;
    int index_count;
    int index_id;
    int page;
    int i, j, k;

    LOG(("in: eb_load_multi_searches(book=%d)", book->code));

    subbook = book->subbook_current;

    for (i = 0, multi = subbook->multis; i < subbook->multi_count;
	 i++, multi++) {
	/*
	 * Read the index table page of the multi search.
	 */
	if (zio_lseek(&subbook->text_zio, 
	    (off_t)(multi->search.start_page - 1) * EB_SIZE_PAGE, SEEK_SET)
	    < 0) {
	    error_code = EB_ERR_FAIL_SEEK_TEXT;
	    goto failed;
	}
	if (zio_read(&subbook->text_zio, buffer, EB_SIZE_PAGE)
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

	buffer_p = buffer + 16;
	for (j = 0, entry = multi->entries;
	     j < multi->entry_count; j++, entry++) {
	    /*
	     * Get the number of indexes in this entry, and title
	     * of this entry.
	     */
	    index_count = eb_uint1(buffer_p);
	    strncpy(entry->label, buffer_p + 2, EB_MAX_MULTI_LABEL_LENGTH);
	    entry->label[EB_MAX_MULTI_LABEL_LENGTH] = '\0';
	    eb_jisx0208_to_euc(entry->label, entry->label);
	    buffer_p += EB_MAX_MULTI_LABEL_LENGTH + 2;

	    /*
	     * Initialize index page information of the entry.
	     */
	    for (k = 0; k < index_count; k++) {
		/*
		 * Get the index page information of the entry.
		 */
		index_id = eb_uint1(buffer_p);
		page = eb_uint4(buffer_p + 2);
		switch (index_id) {
		case 0x71:
		    if (entry->start_page == 0)
			entry->start_page = page;
		    entry->index_id = index_id;
		    break;
		case 0x91:
		case 0xa1:
		    entry->start_page = page;
		    entry->index_id = index_id;
		    break;
		case 0x01:
		    entry->candidates_page = page;
		    entry->index_id = index_id;
		    break;
		}
		buffer_p += 16;
	    }
	}
    }

    LOG(("out: eb_load_multi_searches() = %s", eb_error_string(EB_SUCCESS)));
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_load_multi_searches() = %s", eb_error_string(error_code)));
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
    eb_lock(&book->lock);
    LOG(("in: eb_have_multi_search(book=%d)", (int)book->code));

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL)
	goto failed;

    if (book->subbook_current->multi_count == 0)
	goto failed;

    LOG(("out: eb_have_multi_search() = %d", 1));
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_have_multi_search() = %d", 0));
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
    EB_Subbook_Code *list_p;
    int i;

    eb_lock(&book->lock);
    LOG(("in: eb_multi_search_list(book=%d)", (int)book->code));

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
    for (i = 0, list_p = search_list; i < *search_count; i++, list_p++)
	*list_p = i;

    LOG(("out: eb_multi_search_list(search_count=%d) = %s", *search_count,
	eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *search_count = 0;
    LOG(("out: eb_multi_search_list() = %s", eb_error_string(error_code)));
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
    EB_Subbook_Code *list_p;
    int i;

    eb_lock(&book->lock);
    LOG(("in: eb_multi_entry_list(book=%d, multi_id=%d)", (int)book->code,
	(int)multi_id));

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
    for (i = 0, list_p = entry_list; i < *entry_count; i++, list_p++)
	*list_p = i;

    LOG(("out: eb_multi_entry_list(entry_count=%d) = %s", (int)*entry_count,
	eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *entry_count = 0;
    LOG(("out: eb_multi_entry_list() = %s", eb_error_string(error_code)));
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

    eb_lock(&book->lock);
    LOG(("in: eb_multi_entry_label(book=%d, multi_id=%d, entry_id=%d)",
	(int)book->code, (int)multi_id, (int)entry_id));

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

    LOG(("out: eb_multi_entry_label(label=%s) = %s", label,
	eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *label = '\0';
    LOG(("out: eb_multi_entry_label() = %s", eb_error_string(error_code)));
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

    eb_lock(&book->lock);
    LOG(("in: eb_multi_entry_have_candidates(book=%d, multi_id=%d, \
entry_id=%d)",
	(int)book->code, (int)multi_id, (int)entry_id));

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

    LOG(("out: eb_multi_entry_have_candidates() = %d", 1));
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_multi_entry_have_candidates() = %d", 0));
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

    eb_lock(&book->lock);
    LOG(("in: eb_multi_entry_candidates(book=%d, multi_id=%d, entry_id=%d)",
	(int)book->code, (int)multi_id, (int)entry_id));

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
	error_code = EB_ERR_NO_CANDIDATES;
	goto failed;
    }

    position->page = multi->entries[entry_id].candidates_page;
    position->offset = 0;

    LOG(("out: eb_multi_entry_candidates(position={%d,%d}) = %s", 
	position->page, position->offset, eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_multi_entry_candidates() = %s", 
	eb_error_string(error_code)));
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

    eb_lock(&book->lock);
    LOG(("in: eb_search_multi(book=%d, multi_id=%d, input_words=[below])",
	(int)book->code, (int)multi_id));
#ifdef ENABLE_DEBUG
    for (i = 0; i < EB_MAX_KEYWORDS && input_words[i] != NULL; i++)
	LOG(("    input_words[%d]=%s", i, eb_quoted_string(input_words[i])));
    LOG(("    input_words[%d]=NULL", i));
#endif

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
    eb_reset_search_contexts(book);
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
	context->compare_pre = eb_exact_match_canonicalized_word;
	if (book->character_code == EB_CHARCODE_ISO8859_1)
	    context->compare_hit = eb_exact_match_word_latin;
	else
	    context->compare_hit = eb_exact_match_word_jis;
	context->page = entry->start_page;
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

    LOG(("out: eb_search_multi() = %s", eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_reset_search_contexts(book);
    LOG(("out: eb_search_multi() = %s", eb_error_string(error_code)));
    eb_unlock(&book->lock);
    return error_code;
}


