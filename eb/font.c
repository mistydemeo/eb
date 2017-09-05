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

#ifndef HAVE_MEMMOVE
#define memmove eb_memmove
#endif

#include "eb.h"
#include "error.h"
#include "font.h"
#include "internal.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
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
static EB_Error_Code eb_initialize_eb_fonts EB_P((EB_Book *));
static EB_Error_Code eb_initialize_epwing_fonts EB_P((EB_Book *));

/*
 * Get font information in the current subbook.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned.
 */
EB_Error_Code
eb_initialize_fonts(book)
    EB_Book *book;
{
    EB_Error_Code error_code;

    /*
     * Read font information.
     */
    if (book->disc_code == EB_DISC_EB)
	error_code = eb_initialize_eb_fonts(book);
    else
	error_code = eb_initialize_epwing_fonts(book);

    return error_code;
}


/*
 * For EB*, get font information in the current subbook.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned.
 */
static EB_Error_Code
eb_initialize_eb_fonts(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Font *font;
    char buffer[16];
    int character_count;
    int i;

    font = book->subbook_current->fonts;
    i = 0;
    while (i < book->subbook_current->font_count) {
	/*
	 * Read information from the text file.
	 */
	if (eb_zlseek(&(book->subbook_current->zip),
	    book->subbook_current->text_file,
	    (font->page - 1) * EB_SIZE_PAGE, SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_TEXT;
	    goto failed;
	}
	if (eb_zread(&(book->subbook_current->zip),
	    book->subbook_current->text_file, buffer, 16) != 16) {
	    error_code = EB_ERR_FAIL_READ_TEXT;
	    goto failed;
	}

	/*
	 * Set the information.
	 * (If the number of characters (`character_count') is 0,
	 * the font is unavailable).
	 */
	character_count = eb_uint2(buffer + 12);
	if (character_count == 0) {
	    book->subbook_current->font_count--;
	    if (i < book->subbook_current->font_count) {
		memmove(font, font + 1,
		    sizeof(EB_Font) * (book->subbook_current->font_count - i));
	    }
	    continue;
	}
	font->width = eb_uint1(buffer + 8);
	font->height = eb_uint1(buffer + 9);
	font->start = eb_uint2(buffer + 10);
	if (book->character_code == EB_CHARCODE_ISO8859_1) {
	    font->end = font->start + ((character_count / 0xfe) << 8)
		+ (character_count % 0xfe) - 1;
	    if (0xfe < (font->end & 0xff))
		font->end += 3;
	} else {
	    font->end = font->start + ((character_count / 0x5e) << 8)
		+ (character_count % 0x5e) - 1;
	    if (0x7e < (font->end & 0xff))
		font->end += 0xa3;
	}
	font++;
	i++;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
}


/*
 * For EPWING, get font information in the current subbook.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned.
 */
static EB_Error_Code
eb_initialize_epwing_fonts(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Font *font;
    EB_Zip zip;
    char font_file_name[PATH_MAX + 1];
    int file;
    char buffer[16];
    int character_count;
    int i;
    
    font = book->subbook_current->fonts;
    i = 0;
    while (i < book->subbook_current->font_count) {
	/*
	 * Open a font file.
	 */
	sprintf(font_file_name, "%s/%s/%s/%s", book->path,
	    book->subbook_current->directory, EB_DIRECTORY_NAME_GAIJI,
	    font->file_name);
	eb_fix_file_name(book, font_file_name);
	file = eb_zopen(&zip, font_file_name);
	if (file < 0) {
	    error_code = EB_ERR_FAIL_OPEN_FONT;
	    goto failed;
	}

	/*
	 * Read information.
	 */
	if (eb_zread(&zip, file, buffer, 16) != 16) {
	    error_code = EB_ERR_FAIL_READ_FONT;
	    goto failed;
	}

	/*
	 * Set the information.
	 * (If the length parameter is 0, the font is unavailable).
	 */
	character_count = eb_uint2(buffer + 12);
	if (character_count == 0) {
	    book->subbook_current->font_count--;
	    if (i < book->subbook_current->font_count) {
		memmove(font, font + 1,
		    sizeof(EB_Font) * (book->subbook_current->font_count - i));
	    }
	    continue;
	}
	font->page = 1;
	font->width = eb_uint1(buffer + 8);
	font->height = eb_uint1(buffer + 9);
	font->start = eb_uint2(buffer + 10);
	if (book->character_code == EB_CHARCODE_ISO8859_1) {
	    font->end = font->start + ((character_count / 0xfe) << 8)
		+ (character_count % 0xfe) - 1;
	    if (0xfe < (font->end & 0xff))
		font->end += 3;
	} else {
	    font->end = font->start + ((character_count / 0x5e) << 8)
		+ (character_count % 0x5e) - 1;
	    if (0x7e < (font->end & 0xff))
		font->end += 0xa3;
	}

	/*
	 * Close the font file.
	 */
	eb_zclose(&zip, file);

	font++;
	i++;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
 failed:
    if (0 <= file)
	eb_zclose(&zip, file);
    book->subbook_current->font_count = 0;

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
	*font_code = book->subbook_current->narrow_current->height;
    else if (book->subbook_current->wide_current != NULL)
	*font_code = book->subbook_current->wide_current->height;
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
 * Set the font with `height' as the current font of the current
 * subbook in `book'.
 */
EB_Error_Code
eb_set_font(book, height)
    EB_Book *book;
    EB_Font_Code height;
{
    EB_Error_Code error_code;
    char font_file_name[PATH_MAX + 1];
    EB_Subbook *subbook = book->subbook_current;
    EB_Font *font;
    int narrow_width;
    int wide_width;
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
     * Get the width of the new fonts.
     */
    error_code = eb_narrow_font_width2(height, &narrow_width);
    if (error_code != EB_SUCCESS)
	goto failed;
    error_code = eb_wide_font_width2(height, &wide_width);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * If the current font is the font with `height', return immediately.
     * Otherwise close the current font and continue.
     */
    if (subbook->narrow_current != NULL) {
	if (subbook->narrow_current->height == height)
	    goto succeeded;
	if (book->disc_code == EB_DISC_EPWING) {
	    eb_zclose(&(subbook->narrow_current->zip),
		subbook->narrow_current->font_file);
	}
	subbook->narrow_current = NULL;
    }
    if (subbook->wide_current != NULL) {
	if (subbook->wide_current->height == height)
	    return 0;
	if (book->disc_code == EB_DISC_EPWING) {
	    eb_zclose(&(subbook->wide_current->zip),
		subbook->wide_current->font_file);
	}
	subbook->wide_current = NULL;
    }

    /*
     * Scan the font table in the book.
     */
    for (i = 0, font = subbook->fonts; i < subbook->font_count; i++, font++) {
	if (font->height == height && font->width == narrow_width) {
	    subbook->narrow_current = font;
	    break;
	}
    }
    for (i = 0, font = subbook->fonts; i < subbook->font_count; i++, font++) {
	if (font->height == height && font->width == wide_width) {
	    subbook->wide_current = font;
	    break;
	}
    }

    if (subbook->narrow_current == NULL && subbook->wide_current == NULL) {
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    /*
     * Set file discriptors for fonts.
     * If the book is EBWING, open font files.
     * (In EB books, font data are stored in the `START' file.)
     */
    if (subbook->narrow_current != NULL) {
	if (book->disc_code == EB_DISC_EB)
	    subbook->narrow_current->font_file = subbook->text_file;
	else {
	    sprintf(font_file_name, "%s/%s/%s/%s", book->path,
		subbook->directory, EB_DIRECTORY_NAME_GAIJI,
		subbook->narrow_current->file_name);
	    eb_fix_file_name(book, font_file_name);
	    subbook->narrow_current->font_file
		= eb_zopen(&(subbook->narrow_current->zip), font_file_name);
	    if (subbook->narrow_current->font_file < 0) {
		book->subbook_current->narrow_current = NULL;
		error_code = EB_ERR_FAIL_OPEN_FONT;
		goto failed;
	    }
	}
    }
    if (subbook->wide_current != NULL) {
	if (book->disc_code == EB_DISC_EB)
	    subbook->wide_current->font_file = subbook->text_file;
	else {
	    sprintf(font_file_name, "%s/%s/%s/%s", book->path,
		subbook->directory, EB_DIRECTORY_NAME_GAIJI,
		subbook->wide_current->file_name);
	    eb_fix_file_name(book, font_file_name);
	    subbook->wide_current->font_file
		= eb_zopen(&(subbook->wide_current->zip), font_file_name);
	    if (subbook->wide_current->font_file < 0) {
		book->subbook_current->wide_current = NULL;
		error_code = EB_ERR_FAIL_OPEN_FONT;
		goto failed;
	    }
	}
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
    if (book->subbook_current == NULL) {
	/*
	 * Close font files if the book is EPWING and font files are
	 * opened.
	 */
	if (book->disc_code == EB_DISC_EPWING) {
	    if (book->subbook_current->narrow_current != NULL) {
		eb_zclose(&(book->subbook_current->narrow_current->zip),
		    book->subbook_current->narrow_current->font_file);
	    }
	    if (book->subbook_current->wide_current != NULL) {
		eb_zclose(&(book->subbook_current->wide_current->zip),
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
    EB_Font_Code *list_p = font_list;
    EB_Font *font1, *font2;
    int i, j;

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
     * Scan the font table in the book.
     */
    *font_count = 0;
    for (i = 0, font1 = book->subbook_current->fonts;
	 i < book->subbook_current->font_count; i++, font1++) {
	for (j = 0, font2 = book->subbook_current->fonts; j < i;
	     j++, font2++) {
	    if (font1->height == font2->height)
		break;
	}
	if (i <= j) {
	    *list_p++ = font1->height;
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
 * `height' or not.
 */
int
eb_have_font(book, height)
    EB_Book *book;
    EB_Font_Code height;
{
    EB_Font *font;
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
     * Scan font entries.
     */
    for (i = 0, font = book->subbook_current->fonts;
	 i < book->subbook_current->font_count; i++, font++) {
	if (font->height == height)
	    break;
    }
    if (book->subbook_current->font_count <= i)
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


