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
 * Examine whether the current subbook in `book' supports `KEYWORD SEARCH'
 * or not.
 */
int
eb_have_keyword_search(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    if (book->sub_current->keyword.page == 0) {
	eb_error = EB_ERR_NO_SUCH_SEARCH;
	return 0;
    }

    return 1;
}


#if 0
/*
 * Unexported variables.
 */
static int (*compare) EB_P((const char *, const char *, size_t));

/*
 * Keyword search.
 */
int
eb_search_keyword(book, hitlist, inputword, maxhits)
    EB_Book *book;
    EB_Hit *hitlist;
    const char *inputword;
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
     * Record the parameters for eb_next_keyword().
     */
    bookcode = book->code;
    subcode = book->sub_current->code;
    method = SEARCH_KEYWORD;

    /*
     * Choose a function to compare words.
     */
    compare = eb_match_word;

    /*
     * Make a fixed word and a canonicalized word to search from
     * `inputword'.
     */
    if (eb_set_word(book, word, canonword, inputword) < 0)
	return -1;

    /*
     * Get a page number.
     */
    page = book->sub_current->keyword.page;
    if (page == 0L) {
	eb_error = EB_ERR_NO_SUCH_SEARCH;
	return -1;
    }

    /*
     * Presearch.
     */
    if (eb_presearch(book) < 0)
	return -1;

    /*
     * Search.
     */
    return eb_next_keyword(book, hitlist, maxhits);
}


/*
 * Continue the last keyword search.
 */
int
eb_next_keyword(book, hitlist, maxhits)
    EB_Book *book;
    EB_Hit *hitlist;
    int maxhits;
{
    EB_Hit *hit = hitlist;
    int hitsnum = 0;
    int len;
    int grpid;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * If Book, subbook, or method is changed from the last search,
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
    if (method != SEARCH_ENDWORD) {
	eb_error = EB_ERR_DIFF_SEARCH;
	return -1;
    }

    if (cmp < 0)
	return 0;

    for (;;) {
	if (id == 0x80 || id == 0xa0 || id == 0xc0 || id == 0xe0) {
	    /*
	     * The leaf index for alphabetic words.
	     * Find text and heading locations.
	     */
	    while (i < count) {
		if (EB_SIZE_PAGE < offset + 1) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		len = eb_uint1(bufp);
		if (EB_SIZE_PAGE < offset + len + 13) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}

		cmp = compare(word, bufp + 1, len);
		if (cmp == 0) {
		    hit->heading.page = eb_uint4(bufp + len + 7);
		    hit->heading.offset = eb_uint2(bufp + len + 11);
		    hit->text.page = eb_uint4(bufp + len + 1);
		    hit->text.offset = eb_uint2(bufp + len + 5);
		    hitsnum++;
		    hit++;
		}
		i++;
		offset += len + 13;
		bufp += len + 13;

		if (cmp < 0 || maxhits <= hitsnum)
		    return hitsnum;
	    }
	} else if (id == 0x90 || id == 0xb0 || id == 0xd0 || id == 0xf0) {
	    /*
	     * The leaf index for KANA words.
	     * Find text and heading locations.
	     */
	    while (i < count) {
		if (EB_SIZE_PAGE < offset + 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		grpid = eb_uint1(bufp);

		if (grpid == 0x00) {
		    /*
		     * Single entry.
		     */
		    len = eb_uint1(bufp + 1);
		    if (EB_SIZE_PAGE < offset + len + 14) {
			eb_error = EB_ERR_UNEXP_START;
			return -1;
		    }

		    cmp = compare(canonword, bufp + 2, len);
		    if (cmp == 0 && compare(word, bufp + 2, len) == 0) {
			hit->heading.page = eb_uint4(bufp + len + 8);
			hit->heading.offset = eb_uint2(bufp +len + 12);
			hit->text.page = eb_uint4(bufp + len + 2);
			hit->text.offset = eb_uint2(bufp + len + 6);
			hitsnum++;
			hit++;
		    }
		    offset += len + 14;
		    bufp += len + 14;

		} else if (grpid == 0x80) {
		    /*
		     * Start of the group entry.
		     */
		    len = eb_uint1(bufp + 1);
		    if (EB_SIZE_PAGE < offset + len + 4) {
			eb_error = EB_ERR_UNEXP_START;
			return -1;
		    }
		    cmp = compare(canonword, bufp + 4, len);
		    bufp += len + 4;
		    offset += len + 4;

		} else if (grpid == 0xc0) {
		    /*
		     * Element of the group entry.
		     */
		    if (EB_SIZE_PAGE < offset + 7) {
			eb_error = EB_ERR_UNEXP_START;
			return -1;
		    }

		    if (cmp == 0 && compare(word, bufp + 2, len) == 0) {
			hit->heading.page = headpage;
			hit->heading.offset = headoffset;
			hit->text.page = eb_uint4(bufp + len + 1);
			hit->text.offset = eb_uint2(bufp + len + 5);
			hitsnum++;
			hit++;
		    }
		    offset += 7;
		    bufp += 7;

		} else {
		    /*
		     * Unknown group ID.
		     */
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}

		i++;
		if (cmp < 0 || maxhits <= hitsnum)
		    return hitsnum;
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
	    cmp = -1;
	    return hitsnum;
	}

	page++;
	if (eb_zlseek(book->sub_current->zip, book->sub_current->sub_file,
	    (page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    eb_error = EB_ERR_FAIL_SEEK_START;
	    return -1;
	}
	if (eb_zread(book->sub_current->zip, book->sub_current->sub_file,
	    buf, EB_SIZE_PAGE) != EB_SIZE_PAGE) {
	    eb_error = EB_ERR_FAIL_READ_START;
	    return -1;
	}

	offset = 4;
	id = eb_uint1(buf);
	count = eb_uint2(buf + 2);
	i = 0;
	bufp = buf + 4;
    }

    /* not rearched */
    return 0;
}
#endif
