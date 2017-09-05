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

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

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

#include "eb.h"
#include "error.h"
#include "font.h"
#include "internal.h"

/*
 * Get font information of the current font.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned.
 */
EB_Error_Code
eb_initialize_font(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook = book->subbook_current;
    char buffer[16];
    int character_count;
    EB_Zip *zip;
    int font_file;

    if (book->disc_code == EB_DISC_EB) {
	zip = &book->subbook_current->text_zip;
	font_file = book->subbook_current->text_file;
    } else {
	zip = &book->subbook_current->narrow_current->zip;
	font_file = book->subbook_current->narrow_current->font_file;
    }

    /*
     * Narrow font.
     */
    if (subbook->narrow_current != NULL) {
	/*
	 * Read information from the text file.
	 */
	if (eb_zlseek(zip, font_file, (subbook->narrow_current->page - 1)
	    * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_FONT;
	    goto failed;
	}
	if (eb_zread(zip, font_file, buffer, 16) != 16) {
	    error_code = EB_ERR_FAIL_READ_FONT;
	    goto failed;
	}

	/*
	 * Set the information.
	 * (If the number of characters (`character_count') is 0,
	 * the font is unavailable).
	 */
	character_count = eb_uint2(buffer + 12);
	if (character_count == 0) {
	    subbook->narrow_current->font_code = EB_FONT_INVALID;
	    subbook->narrow_current = NULL;
	} else {
	    subbook->narrow_current->start = eb_uint2(buffer + 10);
	    if (book->character_code == EB_CHARCODE_ISO8859_1) {
		subbook->narrow_current->end = subbook->narrow_current->start
		    + ((character_count / 0xfe) << 8)
		    + (character_count % 0xfe) - 1;
		if (0xfe < (subbook->narrow_current->end & 0xff))
		    subbook->narrow_current->end += 3;
	    } else {
		subbook->narrow_current->end = subbook->narrow_current->start
		    + ((character_count / 0x5e) << 8)
		    + (character_count % 0x5e) - 1;
		if (0x7e < (subbook->narrow_current->end & 0xff))
		    subbook->narrow_current->end += 0xa3;
	    }
	}
    }

    /*
     * Wide fonts.
     */
    if (subbook->wide_current != NULL) {
	/*
	 * Read information from the text file.
	 */
	if (eb_zlseek(zip, font_file, (subbook->wide_current->page - 1)
	    * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_FONT;
	    goto failed;
	}
	if (eb_zread(zip, font_file, buffer, 16) != 16) {
	    error_code = EB_ERR_FAIL_READ_FONT;
	    goto failed;
	}

	/*
	 * Set the information.
	 * (If the number of characters (`character_count') is 0,
	 * the font is unavailable).
	 */
	character_count = eb_uint2(buffer + 12);
	if (character_count == 0) {
	    subbook->wide_current->font_code = EB_FONT_INVALID;
	    subbook->wide_current = NULL;
	} else {
	    subbook->wide_current->start = eb_uint2(buffer + 10);
	    if (book->character_code == EB_CHARCODE_ISO8859_1) {
		subbook->wide_current->end = subbook->wide_current->start
		    + ((character_count / 0xfe) << 8)
		    + (character_count % 0xfe) - 1;
		if (0xfe < (subbook->wide_current->end & 0xff))
		    subbook->wide_current->end += 3;
	    } else {
		subbook->wide_current->end = subbook->wide_current->start
		    + ((character_count / 0x5e) << 8)
		    + (character_count % 0x5e) - 1;
		if (0x7e < (subbook->wide_current->end & 0xff))
		    subbook->wide_current->end += 0xa3;
	    }
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
 * Look up the height of the current font of the current subbook in
 * `book'.
 */
EB_Error_Code
eb_font(book, font_code)
    EB_Book *book;
    EB_Font_Code *font_code;
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
     * Look up the height of the current font.
     */
    if (book->subbook_current->narrow_current != NULL)
	*font_code = book->subbook_current->narrow_current->font_code;
    else if (book->subbook_current->wide_current != NULL)
	*font_code = book->subbook_current->wide_current->font_code;
    else {
	error_code = EB_ERR_NO_CUR_FONT;
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
    *font_code = EB_FONT_INVALID;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Set the font with `font_code' as the current font of the current
 * subbook in `book'.
 */
EB_Error_Code
eb_set_font(book, font_code)
    EB_Book *book;
    EB_Font_Code font_code;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook = book->subbook_current;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Check `font_code'.
     */
    if (font_code < 0 || EB_MAX_FONTS <= font_code) {
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    /*
     * Current subbook must have been set.
     */
    if (subbook == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * If the current font is the font with `font_code', return immediately.
     * Otherwise close the current font and continue.
     */
    if (subbook->narrow_current != NULL) {
	if (subbook->narrow_current->font_code == font_code)
	    goto succeeded;
	if (book->disc_code == EB_DISC_EPWING) {
	    eb_zclose(&subbook->narrow_current->zip,
		subbook->narrow_current->font_file);
	}
	subbook->narrow_current = NULL;
    }
    if (subbook->wide_current != NULL) {
	if (subbook->wide_current->font_code == font_code)
	    return 0;
	if (book->disc_code == EB_DISC_EPWING) {
	    eb_zclose(&subbook->wide_current->zip,
		subbook->wide_current->font_file);
	}
	subbook->wide_current = NULL;
    }

    /*
     * Set the current font.
     */
    if (subbook->narrow_fonts[font_code].font_code != EB_FONT_INVALID)
	subbook->narrow_current = subbook->narrow_fonts + font_code;
    if (subbook->wide_fonts[font_code].font_code != EB_FONT_INVALID)
	subbook->wide_current = subbook->wide_fonts + font_code;

    if (subbook->narrow_current == NULL && subbook->wide_current == NULL) {
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    /*
     * Initialize current font informtaion.
     */
    if (subbook->narrow_current != NULL) {
	error_code = eb_initialize_narrow_font(book);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }
    if (subbook->wide_current != NULL) {
	error_code = eb_initialize_wide_font(book);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }

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
    eb_unset_font(book);
    return error_code;
}


/*
 * Unset the font in the current subbook in `book'.
 */
void
eb_unset_font(book)
    EB_Book *book;
{
    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current != NULL) {
	/*
	 * Close font files if the book is EPWING and font files are
	 * opened.
	 */
	if (book->disc_code == EB_DISC_EPWING) {
	    if (book->subbook_current->narrow_current != NULL) {
		eb_zclose(&book->subbook_current->narrow_current->zip,
		    book->subbook_current->narrow_current->font_file);
	    }
	    if (book->subbook_current->wide_current != NULL) {
		eb_zclose(&book->subbook_current->wide_current->zip,
		    book->subbook_current->wide_current->font_file);
	    }
	}

	book->subbook_current->narrow_current = NULL;
	book->subbook_current->wide_current = NULL;
    }

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);
}


/*
 * Make a list of fonts in the current subbook in `book'.
 */
EB_Error_Code
eb_font_list(book, font_list, font_count)
    EB_Book *book;
    EB_Font_Code *font_list;
    int *font_count;
{
    EB_Error_Code error_code;
    EB_Subbook *subbook = book->subbook_current;
    EB_Font_Code *list_p;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (subbook == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Scan the font table in the book.
     */
    list_p = font_list;
    *font_count = 0;
    for (i = 0; i < EB_MAX_FONTS; i++) {
	if (subbook->narrow_fonts[i].font_code != EB_FONT_INVALID
	    || subbook->wide_fonts[i].font_code != EB_FONT_INVALID) {
	    *list_p++ = i;
	    *font_count += 1;
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
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Test whether the current subbook in `book' has a font with
 * `font_code' or not.
 */
int
eb_have_font(book, font_code)
    EB_Book *book;
    EB_Font_Code font_code;
{
    EB_Subbook *subbook = book->subbook_current;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Check `font_code'.
     */
    if (font_code < 0 || EB_MAX_FONTS <= font_code)
	goto failed;

    /*
     * Current subbook must have been set.
     */
    if (subbook == NULL)
	goto failed;

    if (subbook->narrow_fonts[font_code].font_code == EB_FONT_INVALID
	&& subbook->wide_fonts[font_code].font_code == EB_FONT_INVALID)
	goto failed;
    
    /*
     * Unlock the book.
     */
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
 * Get height of the font `font_code' in the current subbook of `book'.
 */
EB_Error_Code
eb_font_height(book, height)
    EB_Book *book;
    int *height;
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
     * The narrow font must be exist in the current subbook.
     */
    if (book->subbook_current->narrow_current != NULL)
	font_code = book->subbook_current->narrow_current->font_code;
    else if (book->subbook_current->wide_current != NULL)
	font_code = book->subbook_current->wide_current->font_code;
    else {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    /*
     * Calculate height.
     */
    error_code = eb_font_height2(font_code, height);
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
    *height = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/* 
 * Get height of the font `font_code'.
 */
EB_Error_Code
eb_font_height2(font_code, height)
    EB_Font_Code font_code;
    int *height;
{
    EB_Error_Code error_code;

    switch (font_code) {
    case EB_FONT_16:
	*height = EB_HEIGHT_FONT_16;
	break;
    case EB_FONT_24:
	*height = EB_HEIGHT_FONT_24;
	break;
    case EB_FONT_30:
	*height = EB_HEIGHT_FONT_30;
	break;
    case EB_FONT_48:
	*height = EB_HEIGHT_FONT_48;
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
    *height = 0;
    return error_code;
}


