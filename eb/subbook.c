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
static EB_Error_Code eb_initialize_subbook EB_P((EB_Book *));
static EB_Error_Code eb_initialize_indexes EB_P((EB_Book *));


/*
 * Get information about the current subbook.
 */
static EB_Error_Code
eb_initialize_subbook(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook;
    int i;

    subbook = book->subbook_current;
    
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
	error_code = eb_initialize_indexes(book);
	if (error_code != EB_SUCCESS)
	    goto failed;

	/*
	 * Read font information.
	 */
	error_code = eb_initialize_multi_search(book);
	if (error_code != EB_SUCCESS)
	    goto failed;

	/*
	 * Rewind the file descriptor of the start file.
	 */
	if (eb_zlseek(&subbook->text_zip, subbook->text_file, 
	    (off_t)(subbook->index_page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
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
    EB_Subbook_Code subbook_code;
    EB_Font_Code font_code;
    EB_Subbook *subbook;
    int i, j;

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
     * Get the current subbook and the current font.
     */
    if (book->subbook_current != NULL) {
	subbook_code = book->subbook_current->code;
	if (book->subbook_current->narrow_current != NULL)
	    font_code = book->subbook_current->narrow_current->font_code;
	else if (book->subbook_current->wide_current != NULL)
	    font_code = book->subbook_current->wide_current->font_code;
	else
	    font_code = EB_FONT_INVALID;
    } else {
	subbook_code = EB_SUBBOOK_INVALID;
	font_code = EB_FONT_INVALID;
    }

    /*
     * Initialize each subbook.
     */
    for (i = 0, subbook = book->subbooks; i < book->subbook_count;
	 i++, subbook++) {
	error_code = eb_set_subbook(book, subbook->code);
	if (error_code != EB_SUCCESS)
	    goto failed;

	/*
	 * Initialize each font.
	 */
	for (j = 0; j < EB_MAX_FONTS; j++) {
	    if (subbook->narrow_fonts[j].font_code == EB_FONT_INVALID
		&& subbook->wide_fonts[j].font_code == EB_FONT_INVALID)
		continue;
	    if (eb_set_font(book, j) != EB_SUCCESS)
		goto failed;
	}
    }

    /*
     * Restore the current subbook and the current font.
     */
    if (subbook_code == EB_SUBBOOK_INVALID)
	eb_unset_subbook(book);
    else {
	error_code = eb_set_subbook(book, subbook_code);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }

    if (font_code == EB_FONT_INVALID)
	eb_unset_font(book);
    else {
	error_code = eb_set_font(book, font_code);
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
    eb_unset_subbook(book);
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
    EB_Subbook *subbook;
    EB_Search search;
    char buffer[EB_SIZE_PAGE];
    char *buffer_p;
    int id;
    int index_count;
    int availability;
    int global_availability; 
    int i;

    subbook = book->subbook_current;

    /*
     * Initialize zip information (EB* only, for S-EBXA compression).
     */
    if (book->disc_code == EB_DISC_EB
	&& subbook->text_zip.code == EB_ZIP_NONE) {
	subbook->text_zip.zip_start_location = 0;
	subbook->text_zip.zip_end_location   = 0;
	subbook->text_zip.index_base         = 0;
	subbook->text_zip.index_location     = 0;
	subbook->text_zip.index_length       = 0;
    }

    /*
     * Read the index table in the subbook.
     */
    if (eb_zlseek(&subbook->text_zip, subbook->text_file, 0, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_TEXT;
	goto failed;
    }
    if (eb_zread(&subbook->text_zip, subbook->text_file, buffer, EB_SIZE_PAGE)
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
	case 0x00:
	    if (book->disc_code == EB_DISC_EB
		&& subbook->text_zip.code == EB_ZIP_NONE) {
		subbook->text_zip.zip_start_location
		    = (off_t)(eb_uint4(buffer_p + 2) - 1) * EB_SIZE_PAGE;
		subbook->text_zip.zip_end_location
		    = (off_t)subbook->text_zip.zip_start_location
		    + eb_uint4(buffer_p + 6) * EB_SIZE_PAGE;
	    }
	    break;
	case 0x01:
	    memcpy(&subbook->menu, &search, sizeof(EB_Search));
	    break;
	case 0x02:
	    memcpy(&subbook->copyright, &search, sizeof(EB_Search));
	    break;
	case 0x21:
	    if (book->disc_code == EB_DISC_EB
		&& subbook->text_zip.code == EB_ZIP_NONE) {
		subbook->text_zip.index_base
		    = (eb_uint4(buffer_p + 2) - 1) * EB_SIZE_PAGE;
	    }
	    break;
	case 0x22:
	    if (book->disc_code == EB_DISC_EB
		&& subbook->text_zip.code == EB_ZIP_NONE) {
		subbook->text_zip.index_location
		    = (off_t)(eb_uint4(buffer_p + 2) - 1) * EB_SIZE_PAGE;
		subbook->text_zip.index_length
		    = (off_t)eb_uint4(buffer_p + 6) * EB_SIZE_PAGE;
	    }
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
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_16].page = search.index_page;
		subbook->wide_fonts[EB_FONT_16].font_code = EB_FONT_16;
	    }
	    break;
	case 0xf2:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_16].page = search.index_page;
		subbook->narrow_fonts[EB_FONT_16].font_code = EB_FONT_16;
	    }
	    break;
	case 0xf3:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_24].page = search.index_page;
		subbook->wide_fonts[EB_FONT_24].font_code = EB_FONT_24;
	    }
	    break;
	case 0xf4:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_24].page = search.index_page;
		subbook->narrow_fonts[EB_FONT_24].font_code = EB_FONT_24;
	    }
	    break;
	case 0xf5:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_30].page = search.index_page;
		subbook->wide_fonts[EB_FONT_30].font_code = EB_FONT_30;
	    }
	    break;
	case 0xf6:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_30].page = search.index_page;
		subbook->narrow_fonts[EB_FONT_30].font_code = EB_FONT_30;
	    }
	    break;
	case 0xf7:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_48].page = search.index_page;
		subbook->wide_fonts[EB_FONT_48].font_code = EB_FONT_48;
	    }
	    break;
	case 0xf8:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_48].page = search.index_page;
		subbook->narrow_fonts[EB_FONT_48].font_code = EB_FONT_48;
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

    /*
     * Set S-EBXA compression flag.
     * The text file has been opend with eb_zopen_none(), but we don't
     * re-open the file with eb_zopen_sebxa().
     */
    if (book->disc_code == EB_DISC_EB
	&& subbook->text_zip.code == EB_ZIP_NONE
	&& subbook->text_zip.zip_end_location != 0
	&& subbook->text_zip.index_length != 0)
	subbook->text_zip.code = EB_ZIP_SEBXA;

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
    EB_Subbook_Code *list_p;
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

    for (i = 0, list_p = subbook_list; i < book->subbook_count; i++, list_p++)
	*list_p = i;
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
    char *p;

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
     * Copy directory name.
     * Lower letters are converted to upper letters.
     */
    strcpy(directory, book->subbook_current->directory_name);
    for (p = directory; *p != '\0'; p++) {
	if ('a' <= *p && *p <= 'z')
	    *p = *p - ('a' - 'A');
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
    char *p;

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
     * Copy directory name.
     * Lower letters are converted to upper letters.
     */
    strcpy(directory, (book->subbooks + subbook_code)->directory_name);
    for (p = directory; *p != '\0'; p++) {
	if ('a' <= *p && *p <= 'z')
	    *p = *p - ('a' - 'A');
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
    EB_Subbook *subbook;
    char text_path_name[PATH_MAX + 1];
    EB_Zip_Code zip_code;

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
     * Set the current subbook.
     */
    book->subbook_current = book->subbooks + subbook_code;
    subbook = book->subbook_current;

    /*
     * Adjust directory names.
     */
    if (book->disc_code == EB_DISC_EPWING) {
	strcpy(subbook->data_directory_name, EB_DIRECTORY_NAME_DATA);
	eb_fix_directory_name2(book->path, subbook->directory_name,
	    subbook->data_directory_name);

	strcpy(subbook->gaiji_directory_name, EB_DIRECTORY_NAME_GAIJI);
	eb_fix_directory_name2(book->path, subbook->directory_name,
	    subbook->gaiji_directory_name);

	strcpy(subbook->stream_directory_name, EB_DIRECTORY_NAME_STREAM);
	eb_fix_directory_name2(book->path, subbook->directory_name,
	    subbook->stream_directory_name);

	strcpy(subbook->movie_directory_name, EB_DIRECTORY_NAME_MOVIE);
	eb_fix_directory_name2(book->path, subbook->directory_name,
	    subbook->movie_directory_name);
    }

    /*
     * Open a text file if exists.
     */
    zip_code = EB_ZIP_INVALID;

    if (book->disc_code == EB_DISC_EB) {
	if (eb_compose_path_name2(book->path, subbook->directory_name, 
	    EB_FILE_NAME_START, EB_SUFFIX_NONE, text_path_name) == 0) {
	    zip_code = EB_ZIP_NONE;
	} else if (eb_compose_path_name2(book->path, subbook->directory_name, 
	    EB_FILE_NAME_START, EB_SUFFIX_EBZ, text_path_name) == 0) {
	    zip_code = EB_ZIP_EBZIP1;
	}
    } else {
	if (eb_compose_path_name3(book->path, subbook->directory_name,
	    subbook->data_directory_name, EB_FILE_NAME_HONMON2,
	    EB_SUFFIX_NONE, text_path_name) == 0) {
	    if (book->version < 6)
		zip_code = EB_ZIP_EPWING;
	    else
		zip_code = EB_ZIP_EPWING6;

	} else if (eb_compose_path_name3(book->path, subbook->directory_name,
	    subbook->data_directory_name, EB_FILE_NAME_HONMON,
	    EB_SUFFIX_NONE, text_path_name) == 0) {
	    zip_code = EB_ZIP_NONE;
	} else if (eb_compose_path_name3(book->path, subbook->directory_name,
	    subbook->data_directory_name, EB_FILE_NAME_HONMON,
	    EB_SUFFIX_EBZ, text_path_name) == 0) {
	    zip_code = EB_ZIP_EBZIP1;
	}
    }

    if (zip_code != EB_ZIP_INVALID) {
	subbook->text_file = eb_zopen(&subbook->text_zip, text_path_name,
	    zip_code);
	if (subbook->text_file < 0) {
	    subbook = NULL;
	    error_code = EB_ERR_FAIL_OPEN_TEXT;
	    goto failed;
	}
    }

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
	    eb_zclose(&book->subbook_current->text_zip,
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


