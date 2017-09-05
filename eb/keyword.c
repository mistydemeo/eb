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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"


/*
 * Examine whether the current subbook in `book' supports `KEYWORD SEARCH'
 * or not.
 */
int
eb_have_keyword_search(book)
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

    if (book->subbook_current->keyword.index_page == 0)
	goto failed;

    return 1;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&book->lock);
    return 0;
}


/*
 * Keyword search.
 */
EB_Error_Code
eb_search_keyword(book, input_words)
    EB_Book *book;
    const char *input_words[];
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
    if (book->subbook_current->keyword.index_page == 0) {
	error_code = EB_ERR_NO_SUCH_SEARCH;
	goto failed;
    }

    /*
     * Attach a search context for each keyword, and pre-search the
     * keywords.
     */
    word_count = 0;
    for (i = 0; i < EB_MAX_KEYWORDS; i++) {
	if (input_words[i] == NULL)
	    break;

	/*
	 * Initialize search context.
	 */
	context = book->search_contexts + word_count;
	context->code = EB_SEARCH_KEYWORD;
	context->compare = eb_match_exactword;
	context->page = book->subbook_current->keyword.index_page;

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
    } else if (EB_MAX_KEYWORDS <= i) {
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


