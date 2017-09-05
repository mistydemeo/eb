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
 * Examine whether the current subbook in `book' supports `KEYWORD SEARCH'
 * or not.
 */
int
eb_have_keyword_search(book)
    EB_Book *book;
{
    eb_lock(&book->lock);
    LOG(("in: eb_have_keyword_search(book=%d)", (int)book->code));

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL)
	goto failed;

    if (book->subbook_current->keyword.start_page == 0)
	goto failed;

    LOG(("out: eb_have_keyword_search() = %d", 1));
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_have_keyword_search() = %d", 0));
    eb_unlock(&book->lock);
    return 0;
}


/*
 * Keyword search.
 */
EB_Error_Code
eb_search_keyword(book, input_words)
    EB_Book *book;
    const char * const input_words[];
{
    EB_Error_Code error_code;
    EB_Search_Context *context;
    EB_Word_Code word_code;
    int word_count;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);
    LOG(("in: eb_search_keyword(book=%d, input_words=[below])",
	(int)book->code));

    if (eb_log_flag) {
	for (i = 0; i < EB_MAX_KEYWORDS && input_words[i] != NULL; i++) {
	    LOG(("    input_words[%d]=%s", i,
		eb_quoted_string(input_words[i])));
	}
	LOG(("    input_words[%d]=NULL", i));
    }

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
    if (book->subbook_current->keyword.start_page == 0) {
	error_code = EB_ERR_NO_SUCH_SEARCH;
	goto failed;
    }

    /*
     * Attach a search context for each keyword, and pre-search the
     * keywords.
     */
    eb_reset_search_contexts(book);
    word_count = 0;

    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	if (input_words[i] == NULL)
	    break;

	/*
	 * Initialize search context.
	 */
	context = book->search_contexts + word_count;
	context->code = EB_SEARCH_KEYWORD;
	if (book->character_code == EB_CHARCODE_ISO8859_1) {
	    context->compare_pre    = eb_exact_pre_match_word_latin;
	    context->compare_single = eb_exact_match_word_latin;
	    context->compare_group  = eb_exact_match_word_latin;
	} else {
	    context->compare_pre    = eb_exact_pre_match_word_jis;
	    context->compare_single = eb_exact_match_word_jis;
	    context->compare_group  = eb_exact_match_word_jis_kana;
	}
	context->page = book->subbook_current->keyword.start_page;

	/*
	 * Make a fixed word and a canonicalized word to search from
	 * `input_words[i]'.
	 */
	error_code = eb_set_keyword(book, input_words[i], context->word,
	    context->canonicalized_word, &word_code);
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
    } else if (EB_MAX_KEYWORDS <= i && input_words[i] != NULL) {
	error_code =  EB_ERR_TOO_MANY_WORDS;
	goto failed;
    }

    /*
     * Set `EB_SEARCH_NONE' to the rest unused search context.
     */
    for (i = word_count; i < EB_MAX_KEYWORDS; i++)
	(book->search_contexts + i)->code = EB_SEARCH_NONE;

    LOG(("out: eb_search_keyword() = %s", eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_reset_search_contexts(book);
    LOG(("out: eb_search_keyword() = %s", eb_error_string(error_code)));
    eb_unlock(&book->lock);
    return error_code;
}


