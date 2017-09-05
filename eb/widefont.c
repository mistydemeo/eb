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
static EB_Error_Code eb_wide_character_bitmap_jis EB_P((EB_Book *, int,
    char *));
static EB_Error_Code eb_wide_character_bitmap_latin EB_P((EB_Book *, int,
    char *));

/*
 * Get font information of the current font.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned.
 */
EB_Error_Code
eb_initialize_wide_font(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook;
    char font_path_name[PATH_MAX + 1];
    char buffer[16];
    int character_count;
    Zio *zio;
    Zio_Code zio_code;

    subbook = book->subbook_current;

    /*
     * If the book is EBWING, open the wide font file.
     * (In EB books, font data are stored in the `START' file.)
     */
    if (book->disc_code == EB_DISC_EB) {
	if (zio_mode(&subbook->wide_current->zio) != ZIO_INVALID)
	    zio_code = ZIO_REOPEN;
	else
	    zio_code = zio_mode(&subbook->text_zio);
	eb_compose_path_name2(book->path, subbook->directory_name, 
	    subbook->text_file_name, font_path_name);

    } else {
	if (zio_mode(&subbook->wide_current->zio) != ZIO_INVALID)
	    zio_code = ZIO_REOPEN;
	else {
	    eb_canonicalize_file_name(subbook->wide_current->file_name);
	    if (eb_find_file_name3(book->path,
		subbook->directory_name, subbook->gaiji_directory_name,
		subbook->wide_current->file_name, 
		subbook->wide_current->file_name) != EB_SUCCESS) {
		error_code = EB_ERR_FAIL_OPEN_FONT;
		goto failed;
	    }
	}
	eb_compose_path_name3(book->path, subbook->directory_name,
	    subbook->gaiji_directory_name, subbook->wide_current->file_name,
	    font_path_name);
	eb_path_name_zio_code(font_path_name, ZIO_PLAIN, &zio_code);
    }

    if (zio_open(&subbook->wide_current->zio, font_path_name,
	zio_code) < 0) {
	error_code = EB_ERR_FAIL_OPEN_FONT;
	goto failed;
    }

    /*
     * Read information from the text file.
     */
    zio = &subbook->wide_current->zio;

    if (zio_lseek(zio, 
	(off_t)(subbook->wide_current->page - 1) * EB_SIZE_PAGE,
	SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_FONT;
	goto failed;
    }
    if (zio_read(zio, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_FONT;
	goto failed;
    }

    /*
     * If the number of characters (`character_count') is 0, the font
     * is unavailable).
     */
    character_count = eb_uint2(buffer + 12);
    if (character_count == 0) {
	subbook->wide_current->font_code = EB_FONT_INVALID;
	subbook->wide_current = NULL;
	goto succeeded;
    }

    /*
     * Set the information.
     */
    subbook->wide_current->start = eb_uint2(buffer + 10);
    if (book->character_code == EB_CHARCODE_ISO8859_1) {
	subbook->wide_current->end = subbook->wide_current->start
	    + ((character_count / 0xfe) << 8) + (character_count % 0xfe) - 1;
	if (0xfe < (subbook->wide_current->end & 0xff))
	    subbook->wide_current->end += 3;
    } else {
	subbook->wide_current->end = subbook->wide_current->start
	    + ((character_count / 0x5e) << 8) + (character_count % 0x5e) - 1;
	if (0x7e < (subbook->wide_current->end & 0xff))
	    subbook->wide_current->end += 0xa3;
    }

  succeeded:
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
}


/*
 * Examine whether the current subbook in `book' has a wide font.
 */
int
eb_have_wide_font(book)
    EB_Book *book;
{
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
    for (i = 0; i < EB_MAX_FONTS; i++) {
	if (book->subbook_current->wide_fonts[i].font_code
	    != EB_FONT_INVALID)
	    break;
    }

    if (EB_MAX_FONTS <= i)
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
 * Get width of the font `font_code' in the current subbook of `book'.
 */
EB_Error_Code
eb_wide_font_width(book, width)
    EB_Book *book;
    int *width;
{
    EB_Error_Code error_code;
    EB_Font_Code font_code;

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
     * Calculate width.
     */
    font_code = book->subbook_current->wide_current->font_code;
    error_code = eb_wide_font_width2(font_code, width);
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
    *width = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/* 
 * Get width of the font `font_code'.
 */
EB_Error_Code
eb_wide_font_width2(font_code, width)
    EB_Font_Code font_code;
    int *width;
{
    EB_Error_Code error_code;

    switch (font_code) {
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
 * Get the bitmap size of the font `font_code' in the current subbook
 * of `book'.
 */
EB_Error_Code
eb_wide_font_size(book, size)
    EB_Book *book;
    size_t *size;
{
    EB_Error_Code error_code;
    EB_Font_Code font_code;
    int width;
    int height;

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
     * Calculate size.
     */
    font_code = book->subbook_current->wide_current->font_code;
    error_code = eb_wide_font_width2(font_code, &width);
    if (error_code != EB_SUCCESS)
	goto failed;
    error_code = eb_font_height2(font_code, &height);
    if (error_code != EB_SUCCESS)
	goto failed;
    *size = (width / 8) * height;

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
 * Get the bitmap size of a character in `font_code' of the current
 * subbook.
 */
EB_Error_Code
eb_wide_font_size2(font_code, size)
    EB_Font_Code font_code;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (font_code) {
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
    int start;
    int end;
    int character_index;
    off_t location;
    int width;
    int height;
    size_t size;
    Zio *zio;

    start = book->subbook_current->wide_current->start;
    end = book->subbook_current->wide_current->end;

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
    error_code = eb_wide_font_width(book, &width);
    if (error_code != EB_SUCCESS)
	goto failed;
    error_code = eb_font_height(book, &height);
    if (error_code != EB_SUCCESS)
	goto failed;
    size = (width / 8) * height;

    character_index = ((character_number >> 8) - (start >> 8)) * 0x5e
	+ ((character_number & 0xff) - (start & 0xff));
    location
	= (off_t)book->subbook_current->wide_current->page * EB_SIZE_PAGE
	+ (character_index / (1024 / size)) * 1024
	+ (character_index % (1024 / size)) * size;

    /*
     * Read bitmap data.
     */
    if (book->disc_code == EB_DISC_EB)
	zio = &book->subbook_current->text_zio;
    else
	zio = &book->subbook_current->wide_current->zio;

    if (zio_lseek(zio, location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_FONT;
	goto failed;
    }
    if (zio_read(zio, bitmap, size) != size) {
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
    int start;
    int end;
    int character_index;
    off_t location;
    int width;
    int height;
    size_t size;
    Zio *zio;

    start = book->subbook_current->wide_current->start;
    end = book->subbook_current->wide_current->end;

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
    error_code = eb_wide_font_width(book, &width);
    if (error_code != EB_SUCCESS)
	goto failed;
    error_code = eb_font_height(book, &height);
    if (error_code != EB_SUCCESS)
	goto failed;
    size = (width / 8) * height;

    character_index = ((character_number >> 8) - (start >> 8)) * 0xfe
	+ ((character_number & 0xff) - (start & 0xff));
    location
	= (off_t)book->subbook_current->wide_current->page * EB_SIZE_PAGE
	+ (character_index / (1024 / size)) * 1024
	+ (character_index % (1024 / size)) * size;

    /*
     * Read bitmap data.
     */
    if (book->disc_code == EB_DISC_EB)
	zio = &book->subbook_current->text_zio;
    else
	zio = &book->subbook_current->wide_current->zio;

    if (zio_lseek(zio, location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_FONT;
	goto failed;
    }
    if (zio_read(zio, bitmap, size) != size) {
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


