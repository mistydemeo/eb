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

#include "build-pre.h"
#include "eb.h"
#include "error.h"
#include "font.h"
#include "build-post.h"

/*
 * Initialize all fonts in the current subbook.
 */
void
eb_initialize_fonts(book)
    EB_Book *book;
{
    EB_Subbook *subbook;
    EB_Font *font;
    int i;

    LOG(("in: eb_initialize_fonts(book=%d)", (int)book->code));

    subbook = book->subbook_current;

    for (i = 0, font = subbook->narrow_fonts; i < EB_MAX_FONTS; i++, font++) {
	font->font_code = EB_FONT_INVALID;
	font->start = -1;
	font->end = -1;
	font->page = 0;
	zio_initialize(&font->zio);
    }

    for (i = 0, font = subbook->wide_fonts; i < EB_MAX_FONTS; i++, font++) {
	font->font_code = EB_FONT_INVALID;
	font->start = -1;
	font->end = -1;
	font->page = 0;
	zio_initialize(&font->zio);
    }

    LOG(("out: eb_initialize_fonts()"));
}


/*
 * Finalize all fonts in the current subbook.
 */
void
eb_finalize_fonts(book)
    EB_Book *book;
{
    EB_Subbook *subbook;
    EB_Font *font;
    int i;

    LOG(("in: eb_finalize_fonts(book=%d)", (int)book->code));

    subbook = book->subbook_current;

    for (i = 0, font = subbook->narrow_fonts; i < EB_MAX_FONTS; i++, font++)
	zio_finalize(&font->zio);

    for (i = 0, font = subbook->wide_fonts; i < EB_MAX_FONTS; i++, font++)
	zio_finalize(&font->zio);

    LOG(("out: eb_finalize_fonts()"));
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

    eb_lock(&book->lock);
    LOG(("in: eb_font(book=%d)", (int)book->code));

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

    LOG(("out: eb_font(font_code=%d) = %s", (int)*font_code,
	eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *font_code = EB_FONT_INVALID;
    LOG(("out: eb_font() = %s", eb_error_string(error_code)));
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

    eb_lock(&book->lock);
    LOG(("in: eb_set_font(book=%d, font_code=%d)", (int)book->code,
	(int)font_code));

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
	if (book->disc_code == EB_DISC_EPWING)
	    zio_close(&subbook->narrow_current->zio);
	subbook->narrow_current = NULL;
    }
    if (subbook->wide_current != NULL) {
	if (subbook->wide_current->font_code == font_code)
	    return 0;
	if (book->disc_code == EB_DISC_EPWING)
	    zio_close(&subbook->wide_current->zio);
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
	error_code = eb_load_narrow_font(book);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }
    if (subbook->wide_current != NULL) {
	error_code = eb_load_wide_font(book);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }

  succeeded:
    LOG(("out: eb_set_font() = %s", eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_font(book);
    LOG(("out: eb_set_font() = %s", eb_error_string(error_code)));
    return error_code;
}


/*
 * Unset the font in the current subbook in `book'.
 */
void
eb_unset_font(book)
    EB_Book *book;
{
    eb_lock(&book->lock);
    LOG(("in: eb_unset_font(book=%d)", (int)book->code));

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current != NULL) {
	/*
	 * Close font files if the book is EPWING and font files are
	 * opened.
	 */
	if (book->disc_code == EB_DISC_EPWING) {
	    if (book->subbook_current->narrow_current != NULL)
		zio_close(&book->subbook_current->narrow_current->zio);
	    if (book->subbook_current->wide_current != NULL)
		zio_close(&book->subbook_current->wide_current->zio);
	}

	book->subbook_current->narrow_current = NULL;
	book->subbook_current->wide_current = NULL;
    }

    LOG(("out: eb_unset_font()"));
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

    eb_lock(&book->lock);
    LOG(("in: eb_font_list(book=%d)", (int)book->code));

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

    LOG(("out: eb_font(font_count=%d) = %s", *font_count,
	eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_font_list() = %s", eb_error_string(error_code)));
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

    eb_lock(&book->lock);
    LOG(("in: eb_have_font(book=%d, font_code=%d)", (int)book->code,
	(int)font_code));

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
    
    LOG(("out: eb_have_font() = %d", 1));
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_have_font() = %d", 0));
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

    eb_lock(&book->lock);
    LOG(("in: eb_font_height(book=%d)", (int)book->code));

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

    LOG(("out: eb_font_heigt(height=%d) = %s", *height,
	eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *height = 0;
    LOG(("out: eb_font_height() = %s", eb_error_string(error_code)));
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

    LOG(("in: eb_font_height2(font_code=%d)", (int)font_code));

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

    LOG(("out: eb_font_heigt2(height=%d) = %s", *height,
	eb_error_string(EB_SUCCESS)));

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *height = 0;
    LOG(("out: eb_font_height2() = %s", eb_error_string(error_code)));
    return error_code;
}


