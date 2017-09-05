/*
 * Copyright (c) 1997, 98, 99, 2000, 01  
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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "font.h"
#include "internal.h"

/*
 * Unexported functions.
 */
static EB_Error_Code eb_initialize_subbook EB_P((EB_Book *));
static EB_Error_Code eb_initialize_indexes EB_P((EB_Book *));
static EB_Error_Code eb_set_subbook_eb EB_P((EB_Book *, EB_Subbook_Code));
static EB_Error_Code eb_set_subbook_epwing EB_P((EB_Book *, EB_Subbook_Code));

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
     * Initialize contexts.
     */
    eb_initialize_search(book);
    eb_initialize_text(book);
    eb_initialize_binary(book);

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
    subbook->word_alphabet.start_page = 0;
    subbook->word_asis.start_page = 0;
    subbook->word_kana.start_page = 0;
    subbook->endword_alphabet.start_page = 0;
    subbook->endword_asis.start_page = 0;
    subbook->endword_kana.start_page = 0;
    subbook->keyword.start_page = 0;
    subbook->menu.start_page = 0;
    subbook->copyright.start_page = 0;
    subbook->sound.start_page = 0;

    subbook->word_alphabet.end_page = 0;
    subbook->word_asis.end_page = 0;
    subbook->word_kana.end_page = 0;
    subbook->endword_alphabet.end_page = 0;
    subbook->endword_asis.end_page = 0;
    subbook->endword_kana.end_page = 0;
    subbook->keyword.end_page = 0;
    subbook->menu.end_page = 0;
    subbook->copyright.end_page = 0;
    subbook->sound.end_page = 0;

    for (i = 0; i < EB_MAX_MULTI_SEARCHES; i++) {
	subbook->multis[i].search.start_page = 0;
	subbook->multis[i].search.end_page = 0;
    }

    if (0 <= zio_file(&subbook->text_zio)) {
	/*
	 * Read index information.
	 */
	error_code = eb_initialize_indexes(book);
	if (error_code != EB_SUCCESS)
	    goto failed;

	/*
	 * Read mutli search information.
	 */
	error_code = eb_initialize_multi_search(book);
	if (error_code != EB_SUCCESS)
	    goto failed;

	/*
	 * Rewind the file descriptor of the start file.
	 */
	if (zio_lseek(&subbook->text_zio, 
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
    int index_count;
    int availability;
    int global_availability; 
    int i;

    subbook = book->subbook_current;

    /*
     * Initialize zio information (EB* only, for S-EBXA compression).
     */
    if (book->disc_code == EB_DISC_EB
	&& subbook->text_zio.code == ZIO_NONE) {
	subbook->text_zio.zio_start_location = 0;
	subbook->text_zio.zio_end_location   = 0;
	subbook->text_zio.index_base         = 0;
	subbook->text_zio.index_location     = 0;
	subbook->text_zio.index_length       = 0;
    }

    /*
     * Read the index table in the subbook.
     */
    if (zio_lseek(&subbook->text_zio, 0, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_TEXT;
	goto failed;
    }
    if (zio_read(&subbook->text_zio, buffer, EB_SIZE_PAGE) != EB_SIZE_PAGE) {
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

	search.start_page = eb_uint4(buffer_p + 2);
	search.end_page   = search.start_page + eb_uint4(buffer_p + 6);
	search.candidates_page = 0;
	*(search.label) = '\0';

	/*
	 * Identify search method.
	 */
	search.index_id = eb_uint1(buffer_p);
	switch (search.index_id) {
	case 0x00:
	    if (book->disc_code == EB_DISC_EB
		&& subbook->text_zio.code == ZIO_NONE) {
		subbook->text_zio.zio_start_location
		    = (off_t)(eb_uint4(buffer_p + 2) - 1) * EB_SIZE_PAGE;
		subbook->text_zio.zio_end_location
		    = (off_t)subbook->text_zio.zio_start_location
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
		&& subbook->text_zio.code == ZIO_NONE) {
		subbook->text_zio.index_base
		    = (eb_uint4(buffer_p + 2) - 1) * EB_SIZE_PAGE;
	    }
	    break;
	case 0x22:
	    if (book->disc_code == EB_DISC_EB
		&& subbook->text_zio.code == ZIO_NONE) {
		subbook->text_zio.index_location
		    = (off_t)(eb_uint4(buffer_p + 2) - 1) * EB_SIZE_PAGE;
		subbook->text_zio.index_length
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
	case 0xd8:
	    memcpy(&subbook->sound, &search, sizeof(EB_Search));
	    break;
	case 0xf1:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_16].page = search.start_page;
		subbook->wide_fonts[EB_FONT_16].font_code = EB_FONT_16;
	    }
	    break;
	case 0xf2:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_16].page = search.start_page;
		subbook->narrow_fonts[EB_FONT_16].font_code = EB_FONT_16;
	    }
	    break;
	case 0xf3:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_24].page = search.start_page;
		subbook->wide_fonts[EB_FONT_24].font_code = EB_FONT_24;
	    }
	    break;
	case 0xf4:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_24].page = search.start_page;
		subbook->narrow_fonts[EB_FONT_24].font_code = EB_FONT_24;
	    }
	    break;
	case 0xf5:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_30].page = search.start_page;
		subbook->wide_fonts[EB_FONT_30].font_code = EB_FONT_30;
	    }
	    break;
	case 0xf6:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_30].page = search.start_page;
		subbook->narrow_fonts[EB_FONT_30].font_code = EB_FONT_30;
	    }
	    break;
	case 0xf7:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->wide_fonts[EB_FONT_48].page = search.start_page;
		subbook->wide_fonts[EB_FONT_48].font_code = EB_FONT_48;
	    }
	    break;
	case 0xf8:
	    if (book->disc_code == EB_DISC_EB) {
		subbook->narrow_fonts[EB_FONT_48].page = search.start_page;
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

#if 0
    /*
     * Set S-EBXA compression flag.
     * The text file has been opend with zio_open_none(), but we don't
     * re-open the file with eb_zopen_sebxa().
     */
    if (book->disc_code == EB_DISC_EB
	&& subbook->text_zio.code == ZIO_NONE
	&& subbook->text_zio.zio_end_location != 0
	&& subbook->text_zio.index_length != 0)
	subbook->text_zio.code = ZIO_SEBXA;
#endif

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
     * Upper letters are converted to lower letters.
     */
    strcpy(directory, book->subbook_current->directory_name);
    for (p = directory; *p != '\0'; p++) {
	if ('A' <= *p && *p <= 'Z')
	    *p = tolower(*p);
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
     * Upper letters are converted to lower letters.
     */
    strcpy(directory, (book->subbooks + subbook_code)->directory_name);
    for (p = directory; *p != '\0'; p++) {
	if ('A' <= *p && *p <= 'Z')
	    *p = tolower(*p);
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
    EB_Error_Code error_code = EB_SUCCESS;

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

    /*
     * Dispatch.
     */
    if (book->disc_code == EB_DISC_EB)
	error_code = eb_set_subbook_eb(book, subbook_code);
    else
	error_code = eb_set_subbook_epwing(book, subbook_code);

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

    return error_code;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_subbook(book);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Hints of start file name.
 */
#define EB_HINT_INDEX_START		0
#define EB_HINT_INDEX_START_EBZ		1

static const char *start_hint_list[] = {
    "start", "start.ebz", NULL
};

/*
 * Set the subbook `subbook_code' as the current subbook.
 */
static EB_Error_Code
eb_set_subbook_eb(book, subbook_code)
    EB_Book *book;
    EB_Subbook_Code subbook_code;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook;
    char text_path_name[PATH_MAX + 1];
    char graphic_path_name[PATH_MAX + 1];
    Zio_Code text_zio_code;
    Zio_Code graphic_zio_code;
    int text_hint_index;

    subbook = book->subbook_current;

    /*
     * Iinitialize compression I/O descriptors.
     */
    if (!subbook->initialized) {
	zio_initialize(&subbook->text_zio);
	zio_initialize(&subbook->graphic_zio);
	zio_initialize(&subbook->sound_zio);
    }
    zio_initialize(&subbook->movie_zio);

    /*
     * Open a text file if exists.
     */
    if (subbook->initialized) {
	if (zio_mode(&subbook->text_zio) == ZIO_INVALID)
	    text_zio_code = ZIO_INVALID;
	else
	    text_zio_code = ZIO_REOPEN;
    } else {
	eb_find_file_name2(book->path, subbook->directory_name,
	    start_hint_list, subbook->text_file_name, &text_hint_index);

	switch (text_hint_index) {
	case EB_HINT_INDEX_START:
	    text_zio_code = ZIO_NONE;
	    break;

	case EB_HINT_INDEX_START_EBZ:
	    text_zio_code = ZIO_EBZIP1;
	    break;

	default:
	    text_zio_code = ZIO_INVALID;
	    break;
	}
    }

    if (text_zio_code != ZIO_INVALID) {
	eb_compose_path_name2(book->path, subbook->directory_name,
	    subbook->text_file_name, text_path_name);
	if (zio_open(&subbook->text_zio, text_path_name, text_zio_code) < 0) {
	    error_code = EB_ERR_FAIL_OPEN_TEXT;
	    goto failed;
	}
    }

    /*
     * Open a graphic file if exists.
     */
    graphic_zio_code = zio_mode(&subbook->graphic_zio);

    if (subbook->initialized) {
	if (zio_mode(&subbook->graphic_zio) == ZIO_INVALID)
	    graphic_zio_code = ZIO_INVALID;
	else
	    graphic_zio_code = ZIO_REOPEN;
    } else {
	strcpy(subbook->graphic_file_name, subbook->text_file_name);
	graphic_zio_code = text_zio_code;
    }

    if (graphic_zio_code != ZIO_INVALID) {
	eb_compose_path_name2(book->path, subbook->directory_name,
	    subbook->graphic_file_name, graphic_path_name);
	if (zio_open(&subbook->graphic_zio, graphic_path_name,
	    graphic_zio_code) < 0) {
	    error_code = EB_ERR_FAIL_OPEN_BINARY;
	    goto failed;
	}
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_subbook(book);
    return error_code;
}


/*
 * Hints of honmon file name.
 */
#define EB_HINT_INDEX_HONMON		0
#define EB_HINT_INDEX_HONMON_EBZ	1
#define EB_HINT_INDEX_HONMON2		2
#define EB_HINT_INDEX_HONMON2_EBZ	3
#define EB_HINT_INDEX_HONMON2_ORG	4

static const char *honmon_hint_list[] = {
    "honmon", "honmon.ebz", "honmon2", "honmon2.ebz", "honmon2.org", NULL
};

/*
 * Hints of honmon file name.
 */
#define EB_HINT_INDEX_HONMONG		0
#define EB_HINT_INDEX_HONMONG_EBZ	1
#define EB_HINT_INDEX_HONMONG_ORG	2

static const char *honmong_hint_list[] = {
    "honmong", "honmong.ebz", NULL
};

/*
 * Hints of honmon file name.
 */
#define EB_HINT_INDEX_HONMONS		0
#define EB_HINT_INDEX_HONMONS_EBZ	1
#define EB_HINT_INDEX_HONMONS_ORG	2

static const char *honmons_hint_list[] = {
    "honmons", "honmons.ebz", "honmons.org", NULL
};

/*
 * Set the subbook `subbook_code' as the current subbook.
 */
static EB_Error_Code
eb_set_subbook_epwing(book, subbook_code)
    EB_Book *book;
    EB_Subbook_Code subbook_code;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook;
    char text_path_name[PATH_MAX + 1];
    char graphic_path_name[PATH_MAX + 1];
    char sound_path_name[PATH_MAX + 1];
    Zio_Code text_zio_code;
    Zio_Code graphic_zio_code;
    Zio_Code sound_zio_code;
    int text_hint_index;
    int graphic_hint_index;
    int sound_hint_index;

    subbook = book->subbook_current;

    if (!subbook->initialized) {
	/*
	 * Iinitialize compression I/O descriptors.
	 */
	zio_initialize(&subbook->text_zio);
	zio_initialize(&subbook->graphic_zio);
	zio_initialize(&subbook->sound_zio);
	zio_initialize(&subbook->movie_zio);

	/*
	 * Adjust directory names.
	 */
	strcpy(subbook->data_directory_name, EB_DIRECTORY_NAME_DATA);
	eb_fix_directory_name2(book->path, subbook->directory_name,
	    subbook->data_directory_name);
    
	strcpy(subbook->gaiji_directory_name, EB_DIRECTORY_NAME_GAIJI);
	eb_fix_directory_name2(book->path, subbook->directory_name,
	    subbook->gaiji_directory_name);

	strcpy(subbook->movie_directory_name, EB_DIRECTORY_NAME_MOVIE);
	eb_fix_directory_name2(book->path, subbook->directory_name,
	    subbook->movie_directory_name);
    }

    /*
     * Open a text file if exists.
     */
    if (subbook->initialized) {
	if (zio_mode(&subbook->text_zio) == ZIO_INVALID)
	    text_zio_code = ZIO_INVALID;
	else
	    text_zio_code = ZIO_REOPEN;
    } else {
	eb_find_file_name3(book->path, subbook->directory_name,
	    subbook->data_directory_name, honmon_hint_list,
	    subbook->text_file_name, &text_hint_index);

	switch (text_hint_index) {
	case EB_HINT_INDEX_HONMON:
	case EB_HINT_INDEX_HONMON2_ORG:
	    text_zio_code = ZIO_NONE;
	    break;

	case EB_HINT_INDEX_HONMON_EBZ:
	case EB_HINT_INDEX_HONMON2_EBZ:
	    text_zio_code = ZIO_EBZIP1;
	    break;

	case EB_HINT_INDEX_HONMON2:
	    if (book->version < 6)
		text_zio_code = ZIO_EPWING;
	    else
		text_zio_code = ZIO_EPWING6;
	    break;

	default:
	    text_zio_code = ZIO_INVALID;
	    break;
	}
    }

    if (text_zio_code != ZIO_INVALID) {
	eb_compose_path_name3(book->path, subbook->directory_name,
	    subbook->data_directory_name, subbook->text_file_name, 
	    text_path_name);
	if (zio_open(&subbook->text_zio, text_path_name, text_zio_code) < 0) {
	    subbook = NULL;
	    error_code = EB_ERR_FAIL_OPEN_TEXT;
	    goto failed;
	}
    }

    /*
     * Open a graphic file if exists.
     */
    graphic_zio_code = zio_mode(&subbook->graphic_zio);

    if (subbook->initialized) {
	if (zio_mode(&subbook->graphic_zio) == ZIO_INVALID)
	    graphic_zio_code = ZIO_INVALID;
	else
	    graphic_zio_code = ZIO_REOPEN;
    } else {
	if (text_hint_index == EB_HINT_INDEX_HONMON2
	    || text_hint_index == EB_HINT_INDEX_HONMON2_EBZ) {
	    eb_find_file_name3(book->path, subbook->directory_name,
		subbook->data_directory_name, honmong_hint_list,
		subbook->graphic_file_name, &graphic_hint_index);

	    switch (graphic_hint_index) {
	    case EB_HINT_INDEX_HONMONG:
		graphic_zio_code = ZIO_NONE;
		break;

	    case EB_HINT_INDEX_HONMONG_EBZ:
		graphic_zio_code = ZIO_EBZIP1;
		break;

	    default:
		graphic_zio_code = ZIO_INVALID;
		break;
	    }
	} else {
	    strcpy(subbook->graphic_file_name, subbook->text_file_name);
	    graphic_zio_code = text_zio_code;
	}
    }

    if (graphic_zio_code != ZIO_INVALID) {
	eb_compose_path_name3(book->path, subbook->directory_name,
	    subbook->data_directory_name, subbook->graphic_file_name, 
	    graphic_path_name);
	if (zio_open(&subbook->graphic_zio, graphic_path_name,
	    graphic_zio_code) < 0) {
	    error_code = EB_ERR_FAIL_OPEN_BINARY;
	    goto failed;
	}
    }

    /*
     * Open a sound file if exists.
     */
    sound_zio_code = zio_mode(&subbook->sound_zio);

    if (subbook->initialized) {
	if (zio_mode(&subbook->sound_zio) == ZIO_INVALID)
	    sound_zio_code = ZIO_INVALID;
	else
	    sound_zio_code = ZIO_REOPEN;
    } else {
	if (text_hint_index == EB_HINT_INDEX_HONMON2
	    || text_hint_index == EB_HINT_INDEX_HONMON2_EBZ) {
	    eb_find_file_name3(book->path, subbook->directory_name,
		subbook->data_directory_name, honmons_hint_list,
		subbook->sound_file_name, &sound_hint_index);

	    switch (sound_hint_index) {
	    case EB_HINT_INDEX_HONMONG:
		sound_zio_code = ZIO_NONE;
		break;

	    case EB_HINT_INDEX_HONMONG_EBZ:
		sound_zio_code = ZIO_EBZIP1;
		break;

	    default:
		sound_zio_code = ZIO_INVALID;
		break;
	    }
	} else {
	    strcpy(subbook->sound_file_name, subbook->text_file_name);
	    sound_zio_code = text_zio_code;
	}
    }

    if (sound_zio_code != ZIO_INVALID) {
	eb_compose_path_name3(book->path, subbook->directory_name,
	    subbook->data_directory_name, subbook->sound_file_name, 
	    sound_path_name);
	if (zio_open(&subbook->sound_zio, sound_path_name,
	    sound_zio_code) < 0) {
	    error_code = EB_ERR_FAIL_OPEN_BINARY;
	    goto failed;
	}
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_subbook(book);
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
	zio_close(&book->subbook_current->text_zio);
	zio_close(&book->subbook_current->graphic_zio);
	zio_close(&book->subbook_current->sound_zio);
	zio_close(&book->subbook_current->movie_zio);
	zio_finalize(&book->subbook_current->movie_zio);
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


