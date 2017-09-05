/*
 * Copyright (c) 1997, 98, 99, 2000  Motoyuki Kasahara
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
static EB_Error_Code eb_initialize_indexes EB_P((EB_Book *));


/*
 * Get information about the current subbook.
 */
EB_Error_Code
eb_initialize_subbook(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook = book->subbook_current;
    int i;

    /*
     * Current subbook must have been set.
     */
    if (subbook == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Initialize search and text-output status.
     */
    eb_initialize_search(book);
    eb_initialize_text(book);

    /*
     * If the subbook has already initialized, return immediately.
     */
    if (subbook->initialized)
	goto succeeded;

    /*
     * Initialize members in EB_Subbook.
     */
    subbook->narrow_current = NULL;
    subbook->wide_current = NULL;
    subbook->multi_count = 0;
    if (book->disc_code == EB_DISC_EB)
	subbook->font_count = 0;

    /*
     * Initialize search method information.
     */
    subbook->word_alphabet.index_page = 0;
    subbook->word_asis.index_page = 0;
    subbook->word_kana.index_page = 0;
    subbook->endword_alphabet.index_page = 0;
    subbook->endword_asis.index_page = 0;
    subbook->endword_kana.index_page = 0;
    subbook->keyword.index_page = 0;
    subbook->menu.index_page = 0;
    subbook->graphic.index_page = 0;
    subbook->copyright.index_page = 0;

    for (i = 0; i < EB_MAX_MULTI_SEARCHES; i++)
	subbook->multis[i].search.index_page = 0;

    if (0 <= subbook->text_file) {
	/*
	 * Read index information.
	 */
	if (eb_initialize_indexes(book) < 0)
	    goto failed;

	/*
	 * Read font information.
	 */
	if (eb_initialize_fonts(book) < 0)
	    goto failed;

	/*
	 * Read font information.
	 */
	if (eb_initialize_multi_search(book) < 0)
	    goto failed;

	/*
	 * Rewind the file descriptor of the start file.
	 */
	if (eb_zlseek(&(subbook->zip), subbook->text_file, 
	    (subbook->index_page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_TEXT;
	    goto failed;
	}
    }

    subbook->initialized = 1;

  succeeded:
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
}


/*
 * Get information about all subbooks in the book.
 */
EB_Error_Code
eb_initialize_all_subbooks(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook_Code current_code;
    EB_Subbook *subbook;
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
     * Get the current subbook.
     */
    if (book->subbook_current != NULL)
	current_code = book->subbook_current->code;
    else
	current_code = -1;

    /*
     * Initialize each subbook.
     */
    for (i = 0, subbook = book->subbooks; i < book->subbook_count;
	 i++, subbook++) {
	if (eb_set_subbook(book, subbook->code) < 0)
	    goto failed;
    }

    /*
     * Restore the current subbook.
     */
    if (current_code < 0)
	eb_unset_subbook(book);
    else {
	error_code = eb_set_subbook(book, current_code);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }

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
 * Get index information in the current subbook.
 *
 * If succeeds, the number of indexes is returned.
 * Otherwise, -1 is returned.
 */
static EB_Error_Code
eb_initialize_indexes(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook = book->subbook_current;
    EB_Search search;
    char buffer[EB_SIZE_PAGE];
    char *buffer_p;
    int id;
    int index_count;
    int availability;
    int global_availability; 
    int i;

    /*
     * Read the index table in the subbook.
     */
    if (eb_zlseek(&(subbook->zip), subbook->text_file, 0, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_TEXT;
	goto failed;
    }
    if (eb_zread(&(subbook->zip), subbook->text_file, buffer, EB_SIZE_PAGE)
	!= EB_SIZE_PAGE) {
	error_code = EB_ERR_FAIL_READ_TEXT;
	goto failed;
    }

    /*
     * Get start page numbers of the indexes in the subbook.
     */
    index_count = eb_uint1(buffer + 1);
    if (EB_SIZE_PAGE / 16 - 1 <= index_count) {
	error_code = EB_ERR_UNEXP_TEXT;
	goto failed;
    }

    /*
     * Get availavility flag of the index information.
     */
    global_availability = eb_uint1(buffer + 4);
    if (0x02 < global_availability)
	global_availability = 0;

    /*
     * Set each search method information.
     */
    for (i = 0, buffer_p = buffer + 16; i < index_count; i++, buffer_p += 16) {
	/*
	 * Set index style.
	 */
	availability = eb_uint1(buffer_p + 10);
	if ((global_availability == 0x00 && availability == 0x02)
	    || global_availability == 0x02) {
	    unsigned int flags;

	    flags = eb_uint3(buffer_p + 11);
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
	if (book->character_code == EB_CHARCODE_ISO8859_1)
	    search.space = EB_INDEX_STYLE_ASIS;
	else
	    search.space = EB_INDEX_STYLE_DELETE;

	search.index_page = eb_uint4(buffer_p + 2);
	search.candidates_page = 0;
	*(search.label) = '\0';

	/*
	 * Identify search method.
	 */
	id = eb_uint1(buffer_p);
	switch (id) {
	case 0x01:
	    memcpy(&subbook->menu, &search, sizeof(EB_Search));
	    break;
	case 0x02:
	case 0x21:
	    memcpy(&subbook->copyright, &search, sizeof(EB_Search));
	    break;
	case 0x70:
	    memcpy(&subbook->endword_kana, &search, sizeof(EB_Search));
	    break;
	case 0x71:
	    memcpy(&subbook->endword_asis, &search, sizeof(EB_Search));
	    break;
	case 0x72:
	    memcpy(&subbook->endword_alphabet, &search, sizeof(EB_Search));
	    break;
	case 0x80:
	    memcpy(&subbook->keyword, &search, sizeof(EB_Search));
	    break;
	case 0x90:
	    memcpy(&subbook->word_kana, &search, sizeof(EB_Search));
	    break;
	case 0x91:
	    memcpy(&subbook->word_asis, &search, sizeof(EB_Search));
	    break;
	case 0x92:
	    memcpy(&subbook->word_alphabet, &search, sizeof(EB_Search));
	    break;
	case 0xf1:
	case 0xf2:
	case 0xf3:
	case 0xf4:
	    if (book->disc_code == EB_DISC_EB
		&& subbook->font_count < EB_MAX_FONTS * 2) {
		subbook->fonts[subbook->font_count++].page = search.index_page;
	    }
	    break;
	case 0xff:
	    if (subbook->multi_count < EB_MAX_MULTI_SEARCHES) {
		memcpy(&subbook->multis[subbook->multi_count].search, &search,
		    sizeof(EB_Search));
		subbook->multi_count++;
	    }
	    break;
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
 * Make a subbook list in the book.
 */
EB_Error_Code
eb_subbook_list(book, subbook_list, subbook_count)
    EB_Book *book;
    EB_Subbook_Code *subbook_list;
    int *subbook_count;
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

    for (i = 0, listp = subbook_list; i < book->subbook_count; i++, listp++)
	*listp = i;
    *subbook_count = book->subbook_count;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *subbook_count = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a subbook-code of the current subbook.
 */
EB_Error_Code
eb_subbook(book, subbook_code)
    EB_Book *book;
    EB_Subbook_Code *subbook_code;
{
    EB_Error_Code error_code;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * The current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    *subbook_code = book->subbook_current->code;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *subbook_code = EB_SUBBOOK_INVALID;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a title of the current subbook.
 */
EB_Error_Code
eb_subbook_title(book, title)
    EB_Book *book;
    char *title;
{
    EB_Error_Code error_code;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * The current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    strcpy(title, book->subbook_current->title);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *title = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a title of the specified subbook `subbook_code'.
 */
EB_Error_Code
eb_subbook_title2(book, subbook_code, title)
    EB_Book *book;
    EB_Subbook_Code subbook_code;
    char *title;
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
     * Check for the subbook-code.
     */
    if (subbook_code < 0 || book->subbook_count <= subbook_code) {
	error_code = EB_ERR_NO_SUCH_SUB;
	goto failed;
    }

    strcpy(title, (book->subbooks + subbook_code)->title);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *title = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a directory name of the current subbook.
 */
EB_Error_Code
eb_subbook_directory(book, directory)
    EB_Book *book;
    char *directory;
{
    EB_Error_Code error_code;

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

    strcpy(directory, book->subbook_current->directory);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *directory = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a directory name of the specified subbook `subbook_code'.
 */
EB_Error_Code
eb_subbook_directory2(book, subbook_code, directory)
    EB_Book *book;
    EB_Subbook_Code subbook_code;
    char *directory;
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
     * Check for the subbook-code.
     */
    if (subbook_code < 0 || book->subbook_count <= subbook_code) {
	error_code = EB_ERR_NO_SUCH_SUB;
	goto failed;
    }

    strcpy(directory, (book->subbooks + subbook_code)->directory);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *directory = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Set the subbook `subbook_code' as the current subbook.
 */
EB_Error_Code
eb_set_subbook(book, subbook_code)
    EB_Book *book;
    EB_Subbook_Code subbook_code;
{
    EB_Error_Code error_code;
    char text_file_name[PATH_MAX + 1];

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
     * Check for the subbook-code.
     */
    if (subbook_code < 0 || book->subbook_count <= subbook_code) {
	error_code = EB_ERR_NO_SUCH_SUB;
	goto failed;
    }

    /*
     * If the subbook has already been set as the current subbook,
     * there is nothing to be done.
     * Otherwise close the previous subbook.
     */
    if (book->subbook_current != NULL) {
	if (book->subbook_current->code == subbook_code)
	    goto succeeded;
	eb_unset_subbook(book);
    }

    /*
     * Open `START' file if exists.
     */
    book->subbook_current = book->subbooks + subbook_code;
    if (book->disc_code == EB_DISC_EB) {
	sprintf(text_file_name, "%s/%s/%s", book->path,
	    book->subbook_current->directory, EB_FILE_NAME_START);
    } else {
	sprintf(text_file_name, "%s/%s/%s/%s", book->path,
	    book->subbook_current->directory, EB_DIRECTORY_NAME_DATA,
	    EB_FILE_NAME_HONMON);
    }
    eb_fix_file_name(book, text_file_name);
    book->subbook_current->text_file
	= eb_zopen(&(book->subbook_current->zip), text_file_name);

    /*
     * Initialize the subbook.
     */
    error_code = eb_initialize_subbook(book);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Unlock the book.
     */
  succeeded:
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_subbook(book);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Unset the current subbook.
 */
void
eb_unset_subbook(book)
    EB_Book *book;
{
    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Close the file of the current subbook.
     */
    if (book->subbook_current != NULL) {
	eb_unset_font(book);
	if (0 <= book->subbook_current->text_file) {
	    eb_zclose(&(book->subbook_current->zip),
		book->subbook_current->text_file);
	}
	book->subbook_current = NULL;
    }

    /*
     * Initialize search and text output status.
     */
    eb_initialize_search(book);
    eb_initialize_text(book);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);
}


