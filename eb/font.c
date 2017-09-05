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
static int eb_initialize_eb_fonts EB_P((EB_Book *));
static int eb_initialize_epwing_fonts EB_P((EB_Book *));


/*
 * Get font information in the current subbook.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
int
eb_initialize_fonts(book)
    EB_Book *book;
{
    /*
     * Read font information.
     */
    if (book->disc_code == EB_DISC_EB)
	return eb_initialize_eb_fonts(book);
    else
	return eb_initialize_epwing_fonts(book);

    /* not reached */
    return -1;
}


/*
 * For EB*, get font information in the current subbook.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
static int
eb_initialize_eb_fonts(book)
    EB_Book *book;
{
    EB_Font *fnt;
    char buf[16];
    int len;
    int i;

    fnt = book->sub_current->fonts;
    i = 0;
    while (i < book->sub_current->font_count) {
	/*
	 * Read information from the `START' file.
	 */
	if (eb_zlseek(&(book->sub_current->zip),
	    book->sub_current->sub_file, (fnt->page - 1) * EB_SIZE_PAGE,
	    SEEK_SET) < 0) {
	    eb_error = EB_ERR_FAIL_SEEK_START;
	    return -1;
	}
	if (eb_zread(&(book->sub_current->zip),
	    book->sub_current->sub_file, buf, 16) != 16) {
	    eb_error = EB_ERR_FAIL_READ_START;
	    return -1;
	}

	/*
	 * Set the information.
	 * (If the length parameter is 0, the font is unavailable).
	 */
	len = eb_uint2(buf + 12);
	if (len == 0) {
	    book->sub_current->font_count--;
	    if (i < book->sub_current->font_count) {
		memcpy(fnt, fnt + 1,
		    sizeof(EB_Font) * (book->sub_current->font_count - i));
	    }
	    continue;
	}
	fnt->width = eb_uint1(buf + 8);
	fnt->height = eb_uint1(buf + 9);
	fnt->start = eb_uint2(buf + 10);
	if (book->char_code == EB_CHARCODE_ISO8859_1) {
	    fnt->end = fnt->start + ((len / 0xfe) << 8) + (len % 0xfe) - 1;
	    if (0xfe < (fnt->end & 0xff))
		fnt->end += 3;
	} else {
	    fnt->end = fnt->start + ((len / 0x5e) << 8) + (len % 0x5e) - 1;
	    if (0x7e < (fnt->end & 0xff))
		fnt->end += 0xa3;
	}
	fnt++;
	i++;
    }

    return 0;
}


/*
 * For EPWING, get font information in the current subbook.
 *
 * If succeeded, 0 is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
static int
eb_initialize_epwing_fonts(book)
    EB_Book *book;
{
    EB_Font *fnt;
    EB_Zip zip;
    char filename[PATH_MAX + 1];
    int file;
    char buf[16];
    int len;
    int i;
    
    fnt = book->sub_current->fonts;
    i = 0;
    while (i < book->sub_current->font_count) {
	/*
	 * Open a font file.
	 */
	sprintf(filename, "%s/%s/%s/%s", book->path,
	    book->sub_current->directory, EB_DIRNAME_GAIJI, fnt->filename);
	eb_fix_filename(book, filename);
	file = eb_zopen(&zip, filename);
	if (file < 0) {
	    eb_error = EB_ERR_FAIL_OPEN_FONT;
	    goto failed;
	}

	/*
	 * Read information.
	 */
	if (eb_zread(&zip, file, buf, 16) != 16) {
	    eb_error = EB_ERR_FAIL_READ_FONT;
	    goto failed;
	}

	/*
	 * Set the information.
	 * (If the length parameter is 0, the font is unavailable).
	 */
	len = eb_uint2(buf + 12);
	if (len == 0) {
	    book->sub_current->font_count--;
	    if (i < book->sub_current->font_count) {
		memcpy(fnt, fnt + 1,
		    sizeof(EB_Font) * (book->sub_current->font_count - i));
	    }
	    continue;
	}
	fnt->page = 1;
	fnt->width = eb_uint1(buf + 8);
	fnt->height = eb_uint1(buf + 9);
	fnt->start = eb_uint2(buf + 10);
	if (book->char_code == EB_CHARCODE_ISO8859_1) {
	    fnt->end = fnt->start + ((len / 0xfe) << 8) + (len % 0xfe) - 1;
	    if (0xfe < (fnt->end & 0xff))
		fnt->end += 3;
	} else {
	    fnt->end = fnt->start + ((len / 0x5e) << 8) + (len % 0x5e) - 1;
	    if (0x7e < (fnt->end & 0xff))
		fnt->end += 0xa3;
	}

	/*
	 * Close the font file.
	 */
	eb_zclose(&zip, file);

	fnt++;
	i++;
    }

    return 0;

    /*
     * An error occurs...
     */
 failed:
    if (0 <= file)
	eb_zclose(&zip, file);
    book->sub_current->font_count = 0;

    return -1;
}


/*
 * Look up the height of the current font of the current subbook in
 * `book'.
 */
EB_Font_Code
eb_font(book)
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
     * Look up the height of the current font.
     */
    if (book->sub_current->narw_current != NULL)
	return book->sub_current->narw_current->height;

    if (book->sub_current->wide_current != NULL)
	return book->sub_current->wide_current->height;

    eb_error = EB_ERR_NO_CUR_FONT;
    return -1;
}


/*
 * Set the font with `height' as the current font of the current
 * subbook in `book'.
 */
int
eb_set_font(book, height)
    EB_Book *book;
    EB_Font_Code height;
{
    char filename[PATH_MAX + 1];
    EB_Subbook *sub = book->sub_current;
    EB_Font *fnt;
    int narrowwidth, widewidth;
    int i;

    /*
     * Current subbook must have been set.
     */
    if (sub == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Get the width of the new fonts.
     */
    narrowwidth = eb_narrow_font_width2(height);
    widewidth = eb_wide_font_width2(height);
    if (narrowwidth < 0) {
	eb_error = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    /*
     * If the current font is the font with `height', return immediately.
     * Otherwise close the current font and continue.
     */
    if (sub->narw_current != NULL) {
	if (sub->narw_current->height == height)
	    return 0;
	if (book->disc_code == EB_DISC_EPWING) {
	    eb_zclose(&(sub->narw_current->zip),
		sub->narw_current->font_file);
	}
	sub->narw_current = NULL;
    }
    if (sub->wide_current != NULL) {
	if (sub->wide_current->height == height)
	    return 0;
	if (book->disc_code == EB_DISC_EPWING) {
	    eb_zclose(&(sub->wide_current->zip),
		sub->wide_current->font_file);
	}
	sub->wide_current = NULL;
    }

    /*
     * Scan the font table in the book.
     */
    for (i = 0, fnt = sub->fonts; i < sub->font_count; i++, fnt++) {
	if (fnt->height == height && fnt->width == narrowwidth) {
	    sub->narw_current = fnt;
	    break;
	}
    }
    for (i = 0, fnt = sub->fonts; i < sub->font_count; i++, fnt++) {
	if (fnt->height == height && fnt->width == widewidth) {
	    sub->wide_current = fnt;
	    break;
	}
    }

    if (sub->narw_current == NULL && sub->wide_current == NULL) {
	eb_error = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    /*
     * Set file discriptors for fonts.
     * If the book is EBWING, open font files.
     * (In EB books, font data are stored in the `START' file.)
     */
    if (sub->narw_current != NULL) {
	if (book->disc_code == EB_DISC_EB)
	    sub->narw_current->font_file = sub->sub_file;
	else {
	    sprintf(filename, "%s/%s/%s/%s", book->path, sub->directory,
		EB_DIRNAME_GAIJI, sub->narw_current->filename);
	    eb_fix_filename(book, filename);
	    sub->narw_current->font_file =
		eb_zopen(&(sub->narw_current->zip), filename);
	    if (sub->narw_current->font_file < 0) {
		book->sub_current->narw_current = NULL;
		eb_error = EB_ERR_FAIL_OPEN_FONT;
		goto failed;
	    }
	}
    }
    if (sub->wide_current != NULL) {
	if (book->disc_code == EB_DISC_EB)
	    sub->wide_current->font_file = sub->sub_file;
	else {
	    sprintf(filename, "%s/%s/%s/%s", book->path, sub->directory,
		EB_DIRNAME_GAIJI, sub->wide_current->filename);
	    eb_fix_filename(book, filename);
	    sub->wide_current->font_file =
		eb_zopen(&(sub->wide_current->zip), filename);
	    if (sub->wide_current->font_file < 0) {
		book->sub_current->wide_current = NULL;
		eb_error = EB_ERR_FAIL_OPEN_FONT;
		goto failed;
	    }
	}
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_font(book);
    return -1;
}


/*
 * Unset the font in the current subbook in `book'.
 */
void
eb_unset_font(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL)
	return;

    /*
     * Close font files if the book is EPWING and font files are
     * opened.
     */
    if (book->disc_code == EB_DISC_EPWING) {
	if (book->sub_current->narw_current != NULL) {
	    eb_zclose(&(book->sub_current->narw_current->zip),
		book->sub_current->narw_current->font_file);
	}
	if (book->sub_current->wide_current != NULL) {
	    eb_zclose(&(book->sub_current->wide_current->zip),
		book->sub_current->wide_current->font_file);
	}
    }

    book->sub_current->narw_current = NULL;
    book->sub_current->wide_current = NULL;
}


/*
 * Get the number of fonts in the current subbook in `book'.
 */
int
eb_font_count(book)
    EB_Book *book;
{
    EB_Font *fnt1, *fnt2;
    int i, j;
    int count = 0;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    /*
     * Scan the font table in the book.
     */
    for (i = 0, fnt1 = book->sub_current->fonts;
	 i < book->sub_current->font_count; i++, fnt1++) {
	for (j = 0, fnt2 = book->sub_current->fonts; j < i; j++, fnt2++) {
	    if (fnt1->height == fnt2->height)
		break;
	}
	if (i <= j)
	    count++;
    }

    return count;
}


/*
 * Make a list of fonts in the current subbook in `book'.
 */
int
eb_font_list(book, list)
    EB_Book *book;
    EB_Font_Code *list;
{
    EB_Font_Code *lp = list;
    EB_Font *fnt1, *fnt2;
    int i, j;
    int count = 0;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * Scan the font table in the book.
     */
    for (i = 0, fnt1 = book->sub_current->fonts;
	 i < book->sub_current->font_count; i++, fnt1++) {
	for (j = 0, fnt2 = book->sub_current->fonts; j < i; j++, fnt2++) {
	    if (fnt1->height == fnt2->height)
		break;
	}
	if (i <= j) {
	    *lp++ = fnt1->height;
	    count++;
	}
    }

    return count;
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
     * Scan font entries.
     */
    for (i = 0, fnt = book->sub_current->fonts;
	 i < book->sub_current->font_count; i++, fnt++) {
	if (fnt->height == height)
	    return 1;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return 0;
}


