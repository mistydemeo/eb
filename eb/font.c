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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "font.h"
#include "internal.h"

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
    EB_Subbook *subbook;

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
    subbook = book->subbook_current;
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
    if (subbook->narrow_fonts[font_code].font_code != EB_FONT_INVALID) {
	subbook->narrow_current = subbook->narrow_fonts + font_code;
	subbook->narrow_current->font_file = -1;
    }
    if (subbook->wide_fonts[font_code].font_code != EB_FONT_INVALID) {
	subbook->wide_current = subbook->wide_fonts + font_code;
	subbook->wide_current->font_file = -1;
    }

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
	    if (book->subbook_current->narrow_current != NULL
		&& 0 <= book->subbook_current->narrow_current->font_file) {
		eb_zclose(&book->subbook_current->narrow_current->zip,
		    book->subbook_current->narrow_current->font_file);
	    }
	    if (book->subbook_current->wide_current != NULL
		&& 0 <= book->subbook_current->wide_current->font_file) {
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
    EB_Subbook *subbook;
    EB_Font_Code *list_p;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    subbook = book->subbook_current;
    if (subbook == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Scan the font table in the book.
     */
    subbook = book->subbook_current;
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
    EB_Subbook *subbook;

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
    subbook = book->subbook_current;
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


