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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"

/*
 * Unexported variables.
 */
/* Function which compares word to search and pattern in an index page. */
static int (*compare) EB_P((const char *, const char *, size_t));

/* Result of comparison by `compare'. */
static int cmpval;

/* Book-code of the book in which you want to search a word. */
static EB_Book_Code bookcode = -1;

/* Current subbook-code of the book. */
static EB_Subbook_Code subcode = -1;

/* Word to search */
static char word[EB_MAXLEN_WORD + 1];

/* Canonicalized word to search */
static char canonword[EB_MAXLEN_WORD + 1];

/* Page which is searched currently */
static int page;

/* Offset which is searched currently in the page */
static int offset;

/* Page ID of the current page */
static int id;

/* How many entries in the current page */
static int entrycount;

/* Entry index pointer */
static int entryindex;

/* Cache buffer for the current page. */
static char pagebuf[EB_SIZE_PAGE];

/* Current pointer to the cached page data. */
static char *pagebufp;

/*
 * Unexported functinos.
 */
static int eb_search_word_internal EB_P((EB_Book *));
static int eb_hit_list_internal EB_P((EB_Book *, EB_Hit *, int));


/*
 * Intialize the current search status.
 */
void
eb_initialize_search()
{
    bookcode = -1;
    subcode = -1;
    page = 0;
}


/*
 * Examine whether the current subbook in `book' supports `EXACTWORD SEARCH'
 * or not.
 */
int
eb_have_exactword_search(book)
    EB_Book *book;
{
    return eb_have_word_search(book);
}


/*
 * Exactword search.
 */
int
eb_search_exactword(book, inputword)
    EB_Book *book;
    const char *inputword;
{
    EB_Word_Code wordcode;

    page = 0;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * Record the parameters for eb_next_exactword().
     */
    bookcode = book->code;
    subcode = book->sub_current->code;

    /*
     * Choose a function to compare words.
     */
    compare = eb_match_exactword;

    /*
     * Make a fixed word and a canonicalized word to search from
     * `inputword'.
     */
    wordcode = eb_set_word(book, word, canonword, inputword);

    /*
     * Get a page number.
     */
    switch (wordcode) {
    case EB_WORD_ALPHA:
	if (book->sub_current->word_alpha.page != 0)
	    page = book->sub_current->word_alpha.page;
	else if (book->sub_current->word_asis.page != 0)
	    page = book->sub_current->word_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    case EB_WORD_KANA:
	if (book->sub_current->word_kana.page != 0)
	    page = book->sub_current->word_kana.page;
	else if (book->sub_current->word_asis.page != 0)
	    page = book->sub_current->word_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    case EB_WORD_OTHER:
	if (book->sub_current->word_asis.page != 0)
	    page = book->sub_current->word_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    default:
	return -1;
    }

    /*
     * Presearch.
     */
    if (eb_search_word_internal(book) < 0) {
	page = 0;
	return -1;
    }

    return 0;
}


/*
 * Examine whether the current subbook in `book' supports `WORD SEARCH'
 * or not.
 */
int
eb_have_word_search(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    if (book->sub_current->word_alpha.page == 0
	&& book->sub_current->word_asis.page == 0
	&& book->sub_current->word_kana.page == 0) {
	eb_error = EB_ERR_NO_SUCH_SEARCH;
	return 0;
    }

    return 1;
}


/*
 * Word search.
 */
int
eb_search_word(book, inputword)
    EB_Book *book;
    const char *inputword;
{
    EB_Word_Code wordcode;

    page = 0;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * Record the parameters for eb_next_word().
     */
    bookcode = book->code;
    subcode = book->sub_current->code;

    /*
     * Choose a function to compare words.
     */
    compare = eb_match_word;

    /*
     * Make a fixed word and a canonicalized word to search from
     * `inputword'.
     */
    wordcode = eb_set_word(book, word, canonword, inputword);
	
    /*
     * Get a page number.
     */
    switch (wordcode) {
    case EB_WORD_ALPHA:
	if (book->sub_current->word_alpha.page != 0)
	    page = book->sub_current->word_alpha.page;
	else if (book->sub_current->word_asis.page != 0)
	    page = book->sub_current->word_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    case EB_WORD_KANA:
	if (book->sub_current->word_kana.page != 0)
	    page = book->sub_current->word_kana.page;
	else if (book->sub_current->word_asis.page != 0)
	    page = book->sub_current->word_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    case EB_WORD_OTHER:
	if (book->sub_current->word_asis.page != 0)
	    page = book->sub_current->word_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    default:
	return -1;
    }

    /*
     * Presearch.
     */
    if (eb_search_word_internal(book) < 0) {
	page = 0;
	return -1;
    }

    return 0;
}


/*
 * Examine whether the current subbook in `book' supports `ENDWORD SEARCH'
 * or not.
 */
int
eb_have_endword_search(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    if (book->sub_current->endword_alpha.page == 0
	&& book->sub_current->endword_asis.page == 0
	&& book->sub_current->endword_kana.page == 0) {
	eb_error = EB_ERR_NO_SUCH_SEARCH;
	return 0;
    }

    return 1;
}


/*
 * Endword search.
 */
int
eb_search_endword(book, inputword)
    EB_Book *book;
    const char *inputword;
{
    EB_Word_Code wordcode;

    page = 0;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * Record the parameters for eb_next_keyword().
     */
    bookcode = book->code;
    subcode = book->sub_current->code;

    /*
     * Choose a function to compare words.
     */
    compare = eb_match_word;

    /*
     * Make a fixed word and a canonicalized word to search from
     * `inputword'.
     */
    wordcode = eb_set_endword(book, word, canonword, inputword);
    if (wordcode < 0)
	return -1;

    /*
     * Get a page number.
     */
    switch (wordcode) {
    case EB_WORD_ALPHA:
	if (book->sub_current->endword_alpha.page != 0)
	    page = book->sub_current->endword_alpha.page;
	else if (book->sub_current->endword_asis.page != 0)
	    page = book->sub_current->endword_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    case EB_WORD_KANA:
	if (book->sub_current->endword_kana.page != 0)
	    page = book->sub_current->endword_kana.page;
	else if (book->sub_current->endword_asis.page != 0)
	    page = book->sub_current->endword_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    case EB_WORD_OTHER:
	if (book->sub_current->endword_asis.page != 0)
	    page = book->sub_current->endword_asis.page;
	else {
	    eb_error = EB_ERR_NO_SUCH_SEARCH;
	    return -1;
	}
	break;

    default:
	return -1;
    }

    /*
     * Presearch.
     */
    if (eb_search_word_internal(book) < 0) {
	page = 0;
	return -1;
    }

    return 0;
}


/*
 * Continue the last EXACTWORD SEARCH.
 */
int
eb_hit_list(book, hitlist, maxhits)
    EB_Book *book;
    EB_Hit *hitlist;
    int maxhits;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * If book or subbook is changed from the last search,
     * give up searching.
     */
    if (bookcode == -1) {
	eb_error = EB_ERR_NO_PREV_SEARCH;
	return -1;
    }
    if (bookcode != book->code) {
	eb_error = EB_ERR_DIFF_BOOK;
	return -1;
    }
    if (subcode != book->sub_current->code) {
	eb_error = EB_ERR_DIFF_SUBBOOK;
	return -1;
    }
    if (page == 0) {
	eb_error = EB_ERR_NO_PREV_SEARCH;
	return -1;
    }

    return eb_hit_list_internal(book, hitlist, maxhits);
}


/*
 * Search a word in intermediate indexes.
 * If succeeded, 0 is returned.  Otherwise -1 is returned.
 */
static int
eb_search_word_internal(book)
    EB_Book *book;
{
    int nextpage;
    int depth;
    int len;

    /*
     * Search the word in intermediate indexes.
     * Find a page number of the leaf index page.
     */
    for (depth = 0; depth < EB_MAX_INDEX_DEPTH; depth++) {
	nextpage = page;

	/*
	 * Seek and read a page.
	 */
	if (eb_zlseek(&(book->sub_current->zip), 
	    book->sub_current->sub_file, (page - 1) * EB_SIZE_PAGE,
	    SEEK_SET) < 0) {
	    eb_error = EB_ERR_FAIL_SEEK_START;
	    return -1;
	}
	if (eb_zread(&(book->sub_current->zip),
	    book->sub_current->sub_file, pagebuf, EB_SIZE_PAGE)
	    != EB_SIZE_PAGE) {
	    eb_error = EB_ERR_FAIL_READ_START;
	    return -1;
	}

	/*
	 * Get some data from the read page.
	 */
	id = eb_uint1(pagebuf);
	len = eb_uint1(pagebuf + 1);
	entrycount = eb_uint2(pagebuf + 2);
	pagebufp = pagebuf + 4;
	offset = 4;

	if (id != 0x00 && id != 0x20 && id != 0x40 && id != 0x60)
	    break;

	/*
	 * Search a page of next level index.
	 */
	for (entryindex = 0; entryindex < entrycount; entryindex++) {
	    if (EB_SIZE_PAGE < offset + len + 4) {
		eb_error = EB_ERR_UNEXP_START;
		return -1;
	    }
	    if (compare(canonword, pagebufp, len) <= 0) {
		nextpage = eb_uint4(pagebufp + len);
		break;
	    }

	    pagebufp += len + 4;
	    offset += len + 4;
	}
	if (entrycount <= entryindex || page == nextpage) {
	    cmpval = -1;
	    return 0;
	}
	page = nextpage;
    }

    /*
     * Check for the index depth.
     */
    if (depth == EB_MAX_INDEX_DEPTH) {
	eb_error = EB_ERR_UNEXP_START;
	return -1;
    }

    /*
     * Initialize some static variables.
     */
    entryindex = 0;
    cmpval = 1;

    return 0;
}


/*
 * Continue the last word search, endword search or exact-word search.
 */
static int
eb_hit_list_internal(book, hitlist, maxhits)
    EB_Book *book;
    EB_Hit *hitlist;
    int maxhits;
{
    EB_Hit *hit = hitlist;
    int hitcount = 0;
    int len;
    int grpid;

    if (cmpval < 0)
	return 0;

    for (;;) {
	if (id == 0x80 || id == 0xa0 || id == 0xc0 || id == 0xe0) {
	    /*
	     * The leaf index for alphabetic words.
	     * Find text and heading locations.
	     */
	    while (entryindex < entrycount) {
		if (EB_SIZE_PAGE < offset + 1) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		len = eb_uint1(pagebufp);
		if (EB_SIZE_PAGE < offset + len + 13) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}

		/*
		 * Compare word and pattern.
		 * If matched, add it to a hit list.
		 */
		cmpval = compare(word, pagebufp + 1, len);
		if (cmpval == 0) {
		    hit->heading.page = eb_uint4(pagebufp + len + 7);
		    hit->heading.offset = eb_uint2(pagebufp + len + 11);
		    hit->text.page = eb_uint4(pagebufp + len + 1);
		    hit->text.offset = eb_uint2(pagebufp + len + 5);
		    hit++;
		    hitcount++;
		}
		entryindex++;
		offset += len + 13;
		pagebufp += len + 13;

		if (cmpval < 0 || maxhits <= hitcount)
		    return hitcount;
	    }
	} else if (id == 0x90 || id == 0xb0 || id == 0xd0 || id == 0xf0) {
	    /*
	     * The leaf index for KANA words.
	     * Find text and heading locations.
	     */
	    while (entryindex < entrycount) {
		if (EB_SIZE_PAGE < offset + 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		grpid = eb_uint1(pagebufp);

		if (grpid == 0x00) {
		    /*
		     * 0x00 -- Single entry.
		     */
		    len = eb_uint1(pagebufp + 1);
		    if (EB_SIZE_PAGE < offset + len + 14) {
			eb_error = EB_ERR_UNEXP_START;
			return -1;
		    }

		    /*
		     * Compare word and pattern.
		     * If matched, add it to a hit list.
		     */
		    cmpval = compare(canonword, pagebufp + 2, len);
		    if (cmpval == 0 && compare(word, pagebufp + 2, len) == 0) {
			hit->heading.page = eb_uint4(pagebufp + len + 8);
			hit->heading.offset = eb_uint2(pagebufp +len + 12);
			hit->text.page = eb_uint4(pagebufp + len + 2);
			hit->text.offset = eb_uint2(pagebufp + len + 6);
			hit++;
			hitcount++;
		    }
		    offset += len + 14;
		    pagebufp += len + 14;

		} else if (grpid == 0x80) {
		    /*
		     * 0x80 -- Start of group entry.
		     */
		    len = eb_uint1(pagebufp + 1);
		    if (EB_SIZE_PAGE < offset + len + 4) {
			eb_error = EB_ERR_UNEXP_START;
			return -1;
		    }
		    cmpval = compare(canonword, pagebufp + 4, len);
		    pagebufp += len + 4;
		    offset += len + 4;

		} else if (grpid == 0xc0) {
		    /*
		     * Element of the group entry.
		     */
		    len = eb_uint1(pagebufp + 1);
		    if (EB_SIZE_PAGE < offset + 14) {
			eb_error = EB_ERR_UNEXP_START;
			return -1;
		    }

		    /*
		     * Compare word and pattern.
		     * If matched, add it to a hit list.
		     */
		    if (cmpval == 0 && compare(word, pagebufp + 2, len) == 0) {
			hit->heading.page = eb_uint4(pagebufp + len + 8);
			hit->heading.offset = eb_uint2(pagebufp + len + 12);
			hit->text.page = eb_uint4(pagebufp + len + 2);
			hit->text.offset = eb_uint2(pagebufp + len + 6);
			hit++;
			hitcount++;
		    }
		    offset += len + 14;
		    pagebufp += len + 14;

		} else {
		    /*
		     * Unknown group ID.
		     */
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}

		entryindex++;
		if (cmpval < 0 || maxhits <= hitcount)
		    return hitcount;
	    }
	} else {
	    /*
	     * Unknown index page ID.
	     */
	    eb_error = EB_ERR_UNEXP_START;
	    return -1;
	}

	/*
	 * Read next page.
	 */
	if (id == 0xa0 || id == 0xe0 || id == 0xb0 || id == 0xf0) {
	    cmpval = -1;
	    return hitcount;
	}

	page++;
	if (eb_zlseek(&(book->sub_current->zip),
	    book->sub_current->sub_file, (page - 1) * EB_SIZE_PAGE,
	    SEEK_SET) < 0) {
	    eb_error = EB_ERR_FAIL_SEEK_START;
	    return -1;
	}
	if (eb_zread(&(book->sub_current->zip),
	    book->sub_current->sub_file, pagebuf, EB_SIZE_PAGE)
	    != EB_SIZE_PAGE) {
	    eb_error = EB_ERR_FAIL_READ_START;
	    return -1;
	}

	offset = 4;
	id = eb_uint1(pagebuf);
	entrycount = eb_uint2(pagebuf + 2);
	entryindex = 0;
	pagebufp = pagebuf + 4;
    }

    /* not rearched */
    return 0;
}


