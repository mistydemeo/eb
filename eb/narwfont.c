/*
 * Copyright (c) 1997, 1998, 1999  Motoyuki Kasahara
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
#include "font.h"
#include "internal.h"

/*
 * Unexported functions.
 */
static int eb_narrow_character_bitmap_jis EB_P((EB_Book *, int, char *));
static int eb_narrow_character_bitmap_latin EB_P((EB_Book *, int, char *));


/*
 * Examine whether the current subbook in `book' has a narrow font.
 */
int
eb_have_narrow_font(book)
    EB_Book *book;
{
    EB_Font *fnt;
    int i;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    /*
     * If the narrow font has already set, the subbook has narrow fonts.
     */
    if (book->sub_current->narw_current != NULL)
	return 1;

    /*
     * Scan the font table.
     */
    for (i = 0, fnt = book->sub_current->fonts;
	 i < book->sub_current->font_count; i++, fnt++) {
	if (eb_narrow_font_width2(fnt->height) == fnt->width)
	    return 1;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return 0;
}


/* 
 * Get the width of the current narrow font in the current subbook in
 * `book'.
 */
int
eb_narrow_font_width(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    return book->sub_current->narw_current->width;
}


/* 
 * Get the width of the font with `hegiht' in the current subbook in
 * `book'.
 */
int
eb_narrow_font_width2(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
	return EB_WIDTH_NARROW_FONT_16;
    case EB_FONT_24:
	return EB_WIDTH_NARROW_FONT_24;
    case EB_FONT_30:
	return EB_WIDTH_NARROW_FONT_30;
    case EB_FONT_48:
	return EB_WIDTH_NARROW_FONT_48;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}


/*
 * Get the bitmap size of a character of the current font of the current
 * subbook in `book'.
 */
int
eb_narrow_font_size(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    return (book->sub_current->narw_current->width / 8)
	* book->sub_current->narw_current->height;
}


/*
 * Get the bitmap size of a character of the font with `height' of the
 * current subbook in `book'.
 */
int
eb_narrow_font_size2(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
	return EB_SIZE_NARROW_FONT_16;
    case EB_FONT_24:
	return EB_SIZE_NARROW_FONT_24;
    case EB_FONT_30:
	return EB_SIZE_NARROW_FONT_30;
    case EB_FONT_48:
	return EB_SIZE_NARROW_FONT_48;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}


/*
 * Get the filename of the current narrow font in the current subbook
 * in `book'.
 */
const char *
eb_narrow_font_filename(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return NULL;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return NULL;
    }

    /*
     * For EB/EBG/EBXA books, NULL is always returned because they
     * have font data in the `START' file.
     */
    if (book->disc_code == EB_DISC_EB)
	return NULL;

    return book->sub_current->narw_current->filename;
}


/*
 * Get the filename of the narrow font with `height' in the current subbook
 * in `book'.
 */
const char *
eb_narrow_font_filename2(book, height)
    EB_Book *book;
    EB_Font_Code height;
{
    EB_Font *fnt;
    int width;
    int i;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    /*
     * Calculate the width of the font.
     */
    width = eb_narrow_font_width2(height);
    if (width < 0)
	return NULL;

    /*
     * Scan the font table.
     */
    for (i = 0, fnt = book->sub_current->fonts;
	 i < book->sub_current->font_count; i++, fnt++) {
	if (fnt->height == height && fnt->width == width)
	    break;
    }
    if (fnt == NULL) {
	eb_error = EB_ERR_NO_SUCH_FONT;
	return NULL;
    }

    if (book->disc_code == EB_DISC_EB)
	return NULL;

    return fnt->filename;
}


/*
 * Get the character number of the start of the narrow font of the current
 * subbook in `book'.
 */
int
eb_narrow_font_start(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    return book->sub_current->narw_current->start;
}


/*
 * Get the character number of the end of the narrow font of the current
 * subbook in `book'.
 */
int
eb_narrow_font_end(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    return book->sub_current->narw_current->end;
}


/*
 * Get bitmap data of the character with character number `ch' in the
 * current narrow font of the current subbook in `book'.
 */
int
eb_narrow_font_character_bitmap(book, ch, bitmap)
    EB_Book *book;
    int ch;
    char *bitmap;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    if (book->char_code == EB_CHARCODE_ISO8859_1)
	return eb_narrow_character_bitmap_latin(book, ch, bitmap);
    else
	return eb_narrow_character_bitmap_jis(book, ch, bitmap);

    /* not reached */
    return 0;
}


/*
 * Get bitmap data of the character with character number `ch' in the
 * current narrow font of the current subbook in `book'.
 */
static int
eb_narrow_character_bitmap_jis(book, ch, bitmap)
    EB_Book *book;
    int ch;
    char *bitmap;
{
    int start = book->sub_current->narw_current->start;
    int end = book->sub_current->narw_current->end;
    int chindex;
    off_t location;
    size_t size;
    EB_Zip *zip;
    int font_file;

    /*
     * Check for `ch'.  Is it in a range of bitmaps?
     * This test works collectly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (ch < start || end < ch || (ch & 0xff) < 0x21 || 0x7e < (ch & 0xff)) {
	eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
	return -1;
    }

    /*
     * Calculate the size and the location of bitmap data.
     */
    size = (book->sub_current->narw_current->width / 8)
	* book->sub_current->narw_current->height;

    chindex = ((ch >> 8) - (start >> 8)) * 0x5e
	+ ((ch & 0xff) - (start & 0xff));

    location = book->sub_current->narw_current->page * EB_SIZE_PAGE
	+ (chindex / (1024 / size)) * 1024
	+ (chindex % (1024 / size)) * size;

    /*
     * Read bitmap data.
     */
    if (book->disc_code == EB_DISC_EB) {
	zip = &(book->sub_current->zip);
	font_file = book->sub_current->sub_file;
    } else {
	zip = &(book->sub_current->narw_current->zip);
	font_file = book->sub_current->narw_current->font_file;
    }
    if (eb_zlseek(zip, font_file, location, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_FONT;
	return -1;
    }
    if (eb_zread(zip, font_file, bitmap, size) != size) {
	eb_error = EB_ERR_FAIL_READ_FONT;
	return -1;
    }

    return 0;
}


/*
 * Get bitmap data of the character with character number `ch' in the
 * current narrow font of the current subbook in `book'.
 */
static int
eb_narrow_character_bitmap_latin(book, ch, bitmap)
    EB_Book *book;
    int ch;
    char *bitmap;
{
    int start = book->sub_current->narw_current->start;
    int end = book->sub_current->narw_current->end;
    int chindex;
    off_t location;
    size_t size;
    EB_Zip *zip;
    int font_file;

    /*
     * Check for `ch'.  Is it in a range of bitmaps?
     * This test works collectly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (ch < start || end < ch || (ch & 0xff) < 0x01 || 0xfe < (ch & 0xff)) {
	eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
	return -1;
    }

    /*
     * Calculate the size and the location of bitmap data.
     */
    size = (book->sub_current->narw_current->width / 8)
	* book->sub_current->narw_current->height;

    chindex = ((ch >> 8) - (start >> 8)) * 0xfe
	+ ((ch & 0xff) - (start & 0xff));

    location = book->sub_current->narw_current->page * EB_SIZE_PAGE
	+ (chindex / (1024 / size)) * 1024
	+ (chindex % (1024 / size)) * size;

    /*
     * Read bitmap data.
     */
    if (book->disc_code == EB_DISC_EB) {
	zip = &(book->sub_current->zip);
	font_file = book->sub_current->sub_file;
    } else {
	zip = &(book->sub_current->narw_current->zip);
	font_file = book->sub_current->narw_current->font_file;
    }
    if (eb_zlseek(zip, font_file, location, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_FONT;
	return -1;
    }
    if (eb_zread(zip, font_file, bitmap, size) != size) {
	eb_error = EB_ERR_FAIL_READ_FONT;
	return -1;
    }

    return 0;
}


/*
 * Return next `n'th character number from `ch'.
 */
int
eb_forward_narrow_font_character(book, ch, n)
    EB_Book *book;
    int ch;
    int n;
{
    int start;
    int end;
    int i;

    if (n < 0)
	return eb_backward_narrow_font_character(book, ch, -n);

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    start = book->sub_current->narw_current->start;
    end = book->sub_current->narw_current->end;

    if (book->char_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `ch'. (ISO 8859 1)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x01
	    || 0xfe < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
	    return -1;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if (0xfe <= (ch & 0xff))
		ch += 3;
	    else
		ch++;
	    if (end < ch) {
		eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
		return -1;
	    }
	}
    } else {
	/*
	 * Check for `ch'. (JIS X 0208)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x21
	    || 0x7e < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
	    return -1;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if (0x7e <= (ch & 0xff))
		ch += 0xa3;
	    else
		ch++;
	    if (end < ch) {
		eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
		return -1;
	    }
	}
    }

    return ch;
}


/*
 * Return previous `n'th character number from `ch'.
 */
int
eb_backward_narrow_font_character(book, ch, n)
    EB_Book *book;
    int ch;
    int n;
{
    int start;
    int end;
    int i;

    if (n < 0)
	return eb_forward_narrow_font_character(book, ch, -n);

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * The narrow font must be exist in the current subbook.
     */
    if (book->sub_current->narw_current == NULL) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    start = book->sub_current->narw_current->start;
    end = book->sub_current->narw_current->end;

    if (book->char_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `ch'. (ISO 8859 1)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x01
	    || 0xfe < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
	    return -1;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if ((ch & 0xff) <= 0x01)
		ch -= 3;
	    else
		ch--;
	    if (ch < start) {
		eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
		return -1;
	    }
	}
    } else {
	/*
	 * Check for `ch'. (JIS X 0208)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x21
	    || 0x7e < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
	    return -1;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if ((ch & 0xff) <= 0x21)
		ch -= 0xa3;
	    else
		ch--;
	    if (ch < start) {
		eb_error = EB_ERR_NO_SUCH_CHAR_BMP;
		return -1;
	    }
	}
    }

    return ch;
}


