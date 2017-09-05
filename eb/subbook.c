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
#include "font.h"
#include "internal.h"

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef __STDC__
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ */
#endif

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
 * Unexported functions.
 */
static int eb_initialize_indexes EB_P((EB_Book *));


/*
 * Get information about the current subbook.
 */
int
eb_initialize_subbook(book)
    EB_Book *book;
{
    EB_Subbook *sub = book->sub_current;
    int i;

    /*
     * Current subbook must have been set.
     */
    if (sub == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * Initialize search and text-output status.
     */
    eb_initialize_search();
    eb_initialize_text();

    /*
     * If the subbook has already initialized, return immediately.
     */
    if (sub->initialized)
	return 0;

    /*
     * Initialize members in EB_Subbook.
     */
    sub->narw_current = NULL;
    sub->wide_current = NULL;
    sub->multi_count = 0;
    if (book->disc_code == EB_DISC_EB)
	sub->font_count = 0;

    /*
     * Initialize search method information.
     */
    sub->word_alpha.page = 0;
    sub->word_asis.page = 0;
    sub->word_kana.page = 0;
    sub->endword_alpha.page = 0;
    sub->endword_asis.page = 0;
    sub->endword_kana.page = 0;
    sub->keyword.page = 0;
    sub->menu.page = 0;
    sub->graphic.page = 0;
    sub->copyright.page = 0;

    for (i = 0; i < EB_MAX_MULTI_SEARCHES; i++)
	sub->multi[i].page = 0;

    if (0 <= sub->sub_file) {
	/*
	 * Read index information.
	 */
	if (eb_initialize_indexes(book) < 0)
	    return -1;

	/*
	 * Read font information.
	 */
	if (eb_initialize_fonts(book) < 0)
	    return -1;

	/*
	 * Read font information.
	 */
	if (eb_initialize_multi_search(book) < 0)
	    return -1;

	/*
	 * Rewind the file descriptor of the start file.
	 */
	if (eb_zlseek(&(sub->zip), sub->sub_file, 
	    (sub->index_page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    eb_error = EB_ERR_FAIL_SEEK_START;
	    return -1;
	}
    }

    sub->initialized = 1;
    return 0;
}


/*
 * Get information about all subbooks in the book.
 */
int
eb_initialize_all_subbooks(book)
    EB_Book *book;
{
    EB_Subbook_Code cur;
    EB_Subbook *sub;
    int i;

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    /*
     * Get the current subbook.
     */
    if (book->sub_current != NULL)
	cur = book->sub_current->code;
    else
	cur = -1;

    /*
     * Initialize each subbook.
     */
    for (i = 0, sub = book->subbooks; i < book->sub_count; i++, sub++) {
	if (eb_set_subbook(book, sub->code) < 0)
	    return -1;
    }

    /*
     * Restore the current subbook.
     */
    if (cur < 0)
	eb_unset_subbook(book);
    else if (eb_set_subbook(book, cur) < 0)
	return -1;

    return 0;
}


/*
 * Get index information in the current subbook.
 *
 * If succeeds, the number of indexes is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
static int
eb_initialize_indexes(book)
    EB_Book *book;
{
    EB_Subbook *sub = book->sub_current;
    EB_Search search;
    char buf[EB_SIZE_PAGE];
    char *bufp;
    int id;
    int count;
    int avail;
    int global_avail; 
    int i;

    /*
     * Read the index table in the subbook.
     */
    if (eb_zlseek(&(sub->zip), sub->sub_file, 0, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_START;
	return -1;
    }
    if (eb_zread(&(sub->zip), sub->sub_file, buf, EB_SIZE_PAGE)
	!= EB_SIZE_PAGE) {
	eb_error = EB_ERR_FAIL_READ_START;
	return -1;
    }

    /*
     * Get start page numbers of the indexes in the subbook.
     */
    count = eb_uint1(buf + 1);
    if (EB_SIZE_PAGE / 16 - 1 <= count) {
	eb_error = EB_ERR_UNEXP_START;
	return -1;
    }

    /*
     * Get availavility flag of the index information.
     */
    global_avail = eb_uint1(buf + 4);
    if (0x02 < global_avail)
	global_avail = 0;

    /*
     * Set each search method information.
     */
    for (i = 0, bufp = buf + 16; i < count; i++, bufp += 16) {
	/*
	 * Set index style.
	 */
	avail = eb_uint1(bufp + 10);
	if ((global_avail == 0x00 && avail == 0x01) || global_avail == 0x02) {
	    unsigned int flags;

	    flags = eb_uint3(bufp + 11);
	    search.katakana = (flags & 0xc00000) >> 22;
	    search.lower = (flags & 0x300000) >> 20;
	    if ((flags & 0x0c0000) >> 18 == 0)
		search.mark = EB_INDEX_STYLE_DELETE;
	    else
		search.mark = EB_INDEX_STYLE_ASIS;
	    search.long_vowel = (flags & 0x030000) >> 16;
	    search.double_consonant = (flags & 0x00c000) >> 14;
	    search.contracted_sound = (flags & 0x003000) >> 12;
	    search.voiced_consonant = (flags & 0x000c00) >> 10;
	    search.small_vowel = (flags & 0x000300) >> 8;
	    search.p_sound = (flags & 0x0000c0) >> 6;
	} else {
	    search.katakana = EB_INDEX_STYLE_CONVERT;
	    search.lower = EB_INDEX_STYLE_CONVERT;
	    search.mark = EB_INDEX_STYLE_DELETE;
	    search.long_vowel = EB_INDEX_STYLE_CONVERT;
	    search.double_consonant = EB_INDEX_STYLE_CONVERT;
	    search.contracted_sound = EB_INDEX_STYLE_CONVERT;
	    search.voiced_consonant = EB_INDEX_STYLE_CONVERT;
	    search.small_vowel = EB_INDEX_STYLE_CONVERT;
	    search.p_sound = EB_INDEX_STYLE_CONVERT;
	}
	if (book->char_code == EB_CHARCODE_ISO8859_1)
	    search.space = EB_INDEX_STYLE_ASIS;
	else
	    search.space = EB_INDEX_STYLE_DELETE;

	search.page = eb_uint4(bufp + 2);
	search.entry_count = 0;

	/*
	 * Identify search method.
	 */
	id = eb_uint1(bufp);
	switch (id) {
	case 0x01:
	    memcpy(&sub->menu, &search, sizeof(EB_Search));
	    break;
	case 0x02:
	case 0x21:
	    memcpy(&sub->copyright, &search, sizeof(EB_Search));
	    break;
	case 0x70:
	    memcpy(&sub->endword_kana, &search, sizeof(EB_Search));
	    break;
	case 0x71:
	    memcpy(&sub->endword_asis, &search, sizeof(EB_Search));
	    break;
	case 0x72:
	    memcpy(&sub->endword_alpha, &search, sizeof(EB_Search));
	    break;
	case 0x80:
	    memcpy(&sub->keyword, &search, sizeof(EB_Search));
	    break;
	case 0x90:
	    memcpy(&sub->word_kana, &search, sizeof(EB_Search));
	    break;
	case 0x91:
	    memcpy(&sub->word_asis, &search, sizeof(EB_Search));
	    break;
	case 0x92:
	    memcpy(&sub->word_alpha, &search, sizeof(EB_Search));
	    break;
	case 0xf1:
	case 0xf2:
	case 0xf3:
	case 0xf4:
	    if (book->disc_code == EB_DISC_EB
		&& sub->font_count < EB_MAX_FONTS * 2)
		sub->fonts[sub->font_count++].page = search.page;
	    break;
	case 0xff:
	    if (sub->multi_count < EB_MAX_MULTI_SEARCHES) {
		memcpy(&sub->multi[sub->multi_count], &search,
		    sizeof(EB_Search));
		sub->multi_count++;
	    }
	    break;
	}
    }

    return count;
}


/*
 * Return the number of subbooks in the book.
 */
int
eb_subbook_count(book)
    EB_Book *book;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    return book->sub_count;
}


/*
 * Make a subbook list in the book.
 */
int
eb_subbook_list(book, list)
    EB_Book *book;
    EB_Subbook_Code *list;
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

    for (i = 0, lp = list; i < book->sub_count; i++, lp++)
	*lp = i;

    return book->sub_count;
}


/*
 * Return a subbook-code of the current subbook.
 */
EB_Subbook_Code
eb_subbook(book)
    EB_Book *book;
{
    /*
     * The current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    return book->sub_current->code;
}


/*
 * Return a title of the current subbook.
 */
const char *
eb_subbook_title(book)
    EB_Book *book;
{
    /*
     * The current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return NULL;
    }

    return book->sub_current->title;
}


/*
 * Return a title of the specified subbook `code'.
 */
const char *
eb_subbook_title2(book, code)
    EB_Book *book;
    EB_Subbook_Code code;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return NULL;
    }

    /*
     * Check for the subbook-code.
     */
    if (code < 0 || book->sub_count <= code) {
	eb_error = EB_ERR_NO_SUCH_SUB;
	return NULL;
    }

    return (book->subbooks + code)->title;
}


/*
 * Return a directory name of the current subbook.
 */
const char *
eb_subbook_directory(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return NULL;
    }

    return book->sub_current->directory;
}


/*
 * Return a directory name of the specified subbook `code'.
 */
const char *
eb_subbook_directory2(book, code)
    EB_Book *book;
    EB_Subbook_Code code;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return NULL;
    }

    /*
     * Check for the subbook-code.
     */
    if (code < 0 || book->sub_count <= code) {
	eb_error = EB_ERR_NO_SUCH_SUB;
	return NULL;
    }

    return (book->subbooks + code)->directory;
}


/*
 * Set the subbook `code' as the current subbook.
 */
int
eb_set_subbook(book, code)
    EB_Book *book;
    EB_Subbook_Code code;
{
    char start[PATH_MAX + 1];

    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	goto failed;
    }

    /*
     * Check for the subbook-code.
     */
    if (code < 0 || book->sub_count <= code) {
	eb_error = EB_ERR_NO_SUCH_SUB;
	goto failed;
    }

    /*
     * If the subbook has already been set as the current subbook,
     * there is nothing to be done.
     * Otherwise close the previous subbook.
     */
    if (book->sub_current != NULL) {
	if (book->sub_current->code == code)
	    return 0;
	eb_unset_subbook(book);
    }

    /*
     * Open `START' file if exists.
     */
    book->sub_current = book->subbooks + code;
    if (book->disc_code == EB_DISC_EB) {
	sprintf(start, "%s/%s/%s", book->path, book->sub_current->directory,
	    EB_FILENAME_START);
    } else {
	sprintf(start, "%s/%s/%s/%s", book->path, book->sub_current->directory,
	    EB_DIRNAME_DATA, EB_FILENAME_HONMON);
    }
    eb_fix_filename(book, start);
    book->sub_current->sub_file =
	eb_zopen(&(book->sub_current->zip), start);

    /*
     * Initialize the subbook.
     */
    if (eb_initialize_subbook(book) < 0)
	goto failed;

    return 0;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_subbook(book);
    return -1;
}


/*
 * Unset the current subbook.
 */
void
eb_unset_subbook(book)
    EB_Book *book;
{
    /*
     * Close the file of the current subbook.
     */
    if (book->sub_current != NULL) {
	eb_unset_font(book);
	if (0 <= book->sub_current->sub_file) {
	    eb_zclose(&(book->sub_current->zip),
		book->sub_current->sub_file);
	}
	book->sub_current = NULL;
    }

    /*
     * Initialize search and text output status.
     */
    eb_initialize_search();
    eb_initialize_text();
}


