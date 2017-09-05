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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "eb.h"
#include "error.h"
#include "font.h"
#include "internal.h"

/*
 * Unexported functions.
 */
static EB_Error_Code eb_wide_character_bitmap_jis EB_P((EB_Book *, int,
    char *));
static EB_Error_Code eb_wide_character_bitmap_latin EB_P((EB_Book *, int,
    char *));

/*
 * Examine whether the current subbook in `book' has a wide font.
 */
int
eb_have_wide_font(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Font *font;
    int width;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL)
	goto failed;

    /*
     * If the wide font has already set, the subbook has wide fonts.
     */
    if (book->subbook_current->wide_current != NULL)
	goto succeeded;

    /*
     * Scan the font table.
     */
    for (i = 0, font = book->subbook_current->fonts;
	 i < book->subbook_current->font_count; i++, font++) {
	error_code = eb_wide_font_width2(font->height, &width);
	if (error_code == EB_SUCCESS && width == font->width)
	    break;
    }

    if (book->subbook_current->font_count <= i)
	goto failed;

    /*
     * Unlock the book.
     */
  succeeded:
    eb_unlock(&book->lock);
    return 1;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&book->lock);
    return 0;
}


/* 
 * Get the width of the current wide font in the current subbook in
 * `book'.
 */
EB_Error_Code
eb_wide_font_width(book, width)
    EB_Book *book;
    int *width;
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

    /*
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    *width = book->subbook_current->wide_current->width;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *width = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/* 
 * Get the width of the font with `hegiht' in the current subbook in
 * `book'.
 */
EB_Error_Code
eb_wide_font_width2(height, width)
    EB_Font_Code height;
    int *width;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
	*width = EB_WIDTH_WIDE_FONT_16;
	break;
    case EB_FONT_24:
	*width = EB_WIDTH_WIDE_FONT_24;
	break;
    case EB_FONT_30:
	*width = EB_WIDTH_WIDE_FONT_30;
	break;
    case EB_FONT_48:
	*width = EB_WIDTH_WIDE_FONT_48;
	break;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *width = 0;
    return error_code;
}


/*
 * Get the bitmap size of a character of the current font of the current
 * subbook in `book'.
 */
EB_Error_Code
eb_wide_font_size(book, size)
    EB_Book *book;
    size_t *size;
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

    /*
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    *size = (book->subbook_current->wide_current->width / 8)
	* book->subbook_current->wide_current->height;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Get the bitmap size of a character of the font with `height' of the
 * current subbook in `book'.
 */
EB_Error_Code
eb_wide_font_size2(height, size)
    EB_Font_Code height;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
	*size = EB_SIZE_WIDE_FONT_16;
    case EB_FONT_24:
	*size = EB_SIZE_WIDE_FONT_24;
    case EB_FONT_30:
	*size = EB_SIZE_WIDE_FONT_30;
    case EB_FONT_48:
	*size = EB_SIZE_WIDE_FONT_48;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    return error_code;
}


/*
 * Get the file name of the current wide font in the current subbook
 * in `book'.
 */
EB_Error_Code
eb_wide_font_file_name(book, file_name)
    EB_Book *book;
    char *file_name;
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

    /*
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    /*
     * For EB* books, NULL is always returned because they
     * have font data in the `START' file.
     */
    if (book->disc_code == EB_DISC_EB)
	*file_name = '\0';
    else
	strcpy(file_name, book->subbook_current->wide_current->file_name);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *file_name = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Get the file name of the wide font with `height' in the current subbook
 * in `book'.
 */
EB_Error_Code
eb_wide_font_file_name2(book, height, file_name)
    EB_Book *book;
    EB_Font_Code height;
    char *file_name;
{
    EB_Error_Code error_code;
    EB_Font *font;
    int width;
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
     * Calculate the width of the font.
     */
    error_code = eb_wide_font_width2(height, &width);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Scan the font table.
     */
    for (i = 0, font = book->subbook_current->fonts;
	 i < book->subbook_current->font_count; i++, font++) {
	if (font->height == height && font->width == width)
	    break;
    }
    if (font == NULL) {
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    /*
     * For EB* books, NULL is always returned because they
     * have font data in the `START' file.
     */
    if (book->disc_code == EB_DISC_EB)
	*file_name = '\0';
    else
	strcpy(file_name, font->file_name);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *file_name = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Get the character number of the start of the wide font of the current
 * subbook in `book'.
 */
EB_Error_Code
eb_wide_font_start(book, start)
    EB_Book *book;
    int *start;
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

    /*
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    *start = book->subbook_current->wide_current->start;

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
 * Get the character number of the end of the wide font of the current
 * subbook in `book'.
 */
EB_Error_Code
eb_wide_font_end(book, end)
    EB_Book *book;
    int *end;
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

    /*
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    *end = book->subbook_current->wide_current->end;

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
 * Get bitmap data of the character with character number `character_number'
 * in the current wide font of the current subbook in `book'.
 */
EB_Error_Code
eb_wide_font_character_bitmap(book, character_number, bitmap)
    EB_Book *book;
    int character_number;
    char *bitmap;
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

    /*
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    if (book->character_code == EB_CHARCODE_ISO8859_1) {
	error_code = eb_wide_character_bitmap_latin(book, character_number,
	    bitmap);
    } else {
	error_code = eb_wide_character_bitmap_jis(book, character_number,
	    bitmap);
    }
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *bitmap = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Get bitmap data of the character with character number `character_number'
 * in the current wide font of the current subbook in `book'.
 */
static EB_Error_Code
eb_wide_character_bitmap_jis(book, character_number, bitmap)
    EB_Book *book;
    int character_number;
    char *bitmap;
{
    EB_Error_Code error_code;
    int start = book->subbook_current->wide_current->start;
    int end = book->subbook_current->wide_current->end;
    int character_index;
    off_t location;
    size_t size;
    EB_Zip *zip;
    int font_file;

    /*
     * Check for `character_number'.  Is it in a range of bitmaps?
     * This test works correctly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (character_number < start
	|| end < character_number
	|| (character_number & 0xff) < 0x21
	|| 0x7e < (character_number & 0xff)) {
	error_code = EB_ERR_NO_SUCH_CHAR_BMP;
	goto failed;
    }

    /*
     * Calculate the size and the location of bitmap data.
     */
    size = (book->subbook_current->wide_current->width / 8)
	* book->subbook_current->wide_current->height;

    character_index = ((character_number >> 8) - (start >> 8)) * 0x5e
	+ ((character_number & 0xff) - (start & 0xff));

    location = book->subbook_current->wide_current->page * EB_SIZE_PAGE
	+ (character_index / (1024 / size)) * 1024
	+ (character_index % (1024 / size)) * size;

    /*
     * Read bitmap data.
     */
    if (book->disc_code == EB_DISC_EB) {
	zip = &(book->subbook_current->zip);
	font_file = book->subbook_current->text_file;
    } else {
	zip = &(book->subbook_current->wide_current->zip);
	font_file = book->subbook_current->wide_current->font_file;
    }
    if (eb_zlseek(zip, font_file, location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_FONT;
	goto failed;
    }
    if (eb_zread(zip, font_file, bitmap, size) != size) {
	error_code = EB_ERR_FAIL_READ_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *bitmap = '\0';
    return error_code;
}


/*
 * Get bitmap data of the character with character number `character_number'
 * in the current wide font of the current subbook in `book'.
 */
static EB_Error_Code
eb_wide_character_bitmap_latin(book, character_number, bitmap)
    EB_Book *book;
    int character_number;
    char *bitmap;
{
    EB_Error_Code error_code;
    int start = book->subbook_current->wide_current->start;
    int end = book->subbook_current->wide_current->end;
    int character_index;
    off_t location;
    size_t size;
    EB_Zip *zip;
    int font_file;

    /*
     * Check for `ch'.  Is it in a range of bitmaps?
     * This test works correctly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (character_number < start
	|| end < character_number
	|| (character_number & 0xff) < 0x01
	|| 0xfe < (character_number & 0xff)) {
	error_code = EB_ERR_NO_SUCH_CHAR_BMP;
	goto failed;
    }

    /*
     * Calculate the size and the location of bitmap data.
     */
    size = (book->subbook_current->wide_current->width / 8)
	* book->subbook_current->wide_current->height;

    character_index = ((character_number >> 8) - (start >> 8)) * 0xfe
	+ ((character_number & 0xff) - (start & 0xff));

    location = book->subbook_current->wide_current->page * EB_SIZE_PAGE
	+ (character_index / (1024 / size)) * 1024
	+ (character_index % (1024 / size)) * size;

    /*
     * Read bitmap data.
     */
    if (book->disc_code == EB_DISC_EB) {
	zip = &(book->subbook_current->zip);
	font_file = book->subbook_current->text_file;
    } else {
	zip = &(book->subbook_current->wide_current->zip);
	font_file = book->subbook_current->wide_current->font_file;
    }
    if (eb_zlseek(zip, font_file, location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_FONT;
	goto failed;
    }
    if (eb_zread(zip, font_file, bitmap, size) != size) {
	error_code = EB_ERR_FAIL_READ_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *bitmap = '\0';
    return error_code;
}


/*
 * Return next `n'th character number from `character_number'.
 */
EB_Error_Code
eb_forward_wide_font_character(book, n, character_number)
    EB_Book *book;
    int n;
    int *character_number;
{
    EB_Error_Code error_code;
    int start;
    int end;
    int i;

    if (n < 0)
	return eb_backward_wide_font_character(book, -n, character_number);

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
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    start = book->subbook_current->wide_current->start;
    end = book->subbook_current->wide_current->end;

    if (book->character_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `*character_number'. (ISO 8859 1)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x01
	    || 0xfe < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_BMP;
	    goto failed;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if (0xfe <= (*character_number & 0xff))
		*character_number += 3;
	    else
		*character_number += 1;
	    if (end < *character_number) {
		error_code = EB_ERR_NO_SUCH_CHAR_BMP;
		goto failed;
	    }
	}
    } else {
	/*
	 * Check for `*character_number'. (JIS X 0208)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x21
	    || 0x7e < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_BMP;
	    goto failed;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if (0x7e <= (*character_number & 0xff))
		*character_number += 0xa3;
	    else
		*character_number += 1;
	    if (end < *character_number) {
		error_code = EB_ERR_NO_SUCH_CHAR_BMP;
		goto failed;
	    }
	}
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
    *character_number = -1;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return previous `n'th character number from `*character_number'.
 */
EB_Error_Code
eb_backward_wide_font_character(book, n, character_number)
    EB_Book *book;
    int n;
    int *character_number;
{
    EB_Error_Code error_code;
    int start;
    int end;
    int i;

    if (n < 0)
	return eb_forward_wide_font_character(book, -n, character_number);

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
     * The wide font must be exist in the current subbook.
     */
    if (book->subbook_current->wide_current == NULL) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    start = book->subbook_current->wide_current->start;
    end = book->subbook_current->wide_current->end;

    if (book->character_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `*character_number'. (ISO 8859 1)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x01
	    || 0xfe < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_BMP;
	    goto failed;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if ((*character_number & 0xff) <= 0x01)
		*character_number -= 3;
	    else
		*character_number -= 1;
	    if (*character_number < start) {
		error_code = EB_ERR_NO_SUCH_CHAR_BMP;
		goto failed;
	    }
	}
    } else {
	/*
	 * Check for `*character_number'. (JIS X 0208)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x21
	    || 0x7e < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_BMP;
	    goto failed;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if ((*character_number & 0xff) <= 0x21)
		*character_number -= 0xa3;
	    else
		*character_number -= 1;
	    if (*character_number < start) {
		error_code = EB_ERR_NO_SUCH_CHAR_BMP;
		goto failed;
	    }
	}
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
    *character_number = -1;
    eb_unlock(&book->lock);
    return error_code;
}


