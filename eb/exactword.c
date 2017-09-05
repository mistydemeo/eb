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
 * Examine whether the current subbook in `book' supports `EXACTWORD SEARCH'
 * or not.
 */
int
eb_have_exactword_search(book)
    EB_Book *book;
{
    eb_lock(&book->lock);
    LOG(("in: eb_have_exactword_search(book=%d)", (int)book->code));

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL)
	goto failed;

    /*
     * Check for the index page of word search.
     */
    if (book->subbook_current->word_alphabet.start_page == 0
	&& book->subbook_current->word_asis.start_page == 0
	&& book->subbook_current->word_kana.start_page == 0)
	goto failed;

    LOG(("out: eb_have_exactword_search() = %d", 1));
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_have_exactword_search() = %d", 0));
    eb_unlock(&book->lock);
    return 0;
}


/*
 * Exactword search.
 */
EB_Error_Code
eb_search_exactword(book, input_word)
    EB_Book *book;
    const char *input_word;
{
    EB_Error_Code error_code;
    EB_Word_Code word_code;
    EB_Search_Context *context;

    eb_lock(&book->lock);
    LOG(("in: eb_search_exactword(book=%d, input_word=%s)", (int)book->code,
	eb_quoted_string(input_word)));

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Initialize search context.
     */
    eb_reset_search_contexts(book);
    context = book->search_contexts;
    context->code = EB_SEARCH_EXACTWORD;
    context->compare_pre = eb_exact_match_canonicalized_word;
    if (book->character_code == EB_CHARCODE_ISO8859_1)
	context->compare_hit = eb_exact_match_word_latin;
    else
	context->compare_hit = eb_exact_match_word_jis;

    /*
     * Make a fixed word and a canonicalized word to search from
     * `input_word'.
     */
    error_code = eb_set_word(book, input_word, context->word,
	context->canonicalized_word, &word_code);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Get a page number.
     */
    switch (word_code) {
    case EB_WORD_ALPHABET:
	if (book->subbook_current->word_alphabet.start_page != 0)
	    context->page = book->subbook_current->word_alphabet.start_page;
	else if (book->subbook_current->word_asis.start_page != 0)
	    context->page = book->subbook_current->word_asis.start_page;
	else {
	    error_code = EB_ERR_NO_SUCH_SEARCH;
	    goto failed;
	}
	break;

    case EB_WORD_KANA:
	if (book->subbook_current->word_kana.start_page != 0)
	    context->page = book->subbook_current->word_kana.start_page;
	else if (book->subbook_current->word_asis.start_page != 0)
	    context->page = book->subbook_current->word_asis.start_page;
	else {
	    error_code = EB_ERR_NO_SUCH_SEARCH;
	    goto failed;
	}
	break;

    case EB_WORD_OTHER:
	if (book->subbook_current->word_asis.start_page != 0)
	    context->page = book->subbook_current->word_asis.start_page;
	else {
	    error_code = EB_ERR_NO_SUCH_SEARCH;
	    goto failed;
	}
	break;

    default:
	error_code = EB_ERR_NO_SUCH_SEARCH;
	goto failed;
    }

    /*
     * Pre-search.
     */
    error_code = eb_presearch_word(book, context);
    if (error_code != EB_SUCCESS)
	goto failed;

    LOG(("out: eb_search_exactword() = %s", eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_reset_search_contexts(book);
    LOG(("out: eb_search_exactword() = %s", eb_error_string(error_code)));
    eb_unlock(&book->lock);
    return error_code;
}
