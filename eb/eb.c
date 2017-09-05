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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "language.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

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
static int eb_initialize_catalog EB_P((EB_Book *));

/*
 * Unexported variable.
 */
/* Book ID counter. */
static EB_Book_Code counter = 0;


/*
 * Initialize `book'.
 */
void
eb_initialize(book)
    EB_Book *book;
{
    book->code = counter++;
    book->path = NULL;
    book->sub_current = NULL;
    book->subbooks = NULL;
    book->languages = NULL;
    book->lang_current = NULL;
    book->messages = NULL;
}


/*
 * Bind `book' to `path'.
 */
int
eb_bind(book, path)
    EB_Book *book;
    const char *path;
{
    char tmppath[PATH_MAX + 1];
    int count;

    /*
     * Clear the book.
     */
    eb_clear(book);

    /*
     * Set the path of the appendix.
     * The length of the filename "<path>/subdir/subsubdir/file.ebz;1" must
     * not exceed PATH_MAX.
     */
    if (PATH_MAX < strlen(path)) {
	eb_error = EB_ERR_TOO_LONG_FILENAME;
	goto failed;
    }
    strcpy(tmppath, path);
    if (eb_canonicalize_filename(tmppath) < 0)
	goto failed;

    book->path_length = strlen(tmppath);
    if (PATH_MAX < book->path_length + (1 + EB_MAXLEN_BASENAME + 1
	+ EB_MAXLEN_BASENAME + 1 + EB_MAXLEN_BASENAME + 6)) {
	eb_error = EB_ERR_TOO_LONG_FILENAME;
	goto failed;
    }

    book->path = (char *)malloc(book->path_length + 1);
    if (book->path == NULL) {
	eb_error = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }
    strcpy(book->path, tmppath);

    /*
     * Get disc type and filename mode.
     */
    if (eb_catalog_filename(book) < 0)
	goto failed;

    /*
     * Read information from the `LANGUAGE' file.
     * If failed to initialize, JISX0208 is assumed.
     */
    if (eb_initialize_languages(book) < 0) {
	book->char_code = EB_CHARCODE_JISX0208;
	eb_error = EB_NO_ERR;
    }

    /*
     * Read information from the `CATALOG(S)' file.
     */
    count = eb_initialize_catalog(book);
    if (count < 0)
	goto failed;
    
    return count;

    /*
     * An error occurs...
     */
  failed:
    eb_clear(book);
    return -1;
}


/*
 * Suspend using `book'.
 */
void
eb_suspend(book)
    EB_Book *book;
{
    eb_unset_subbook(book);
    eb_unset_language(book);
}


/*
 * Finish using `book'.
 */
void
eb_clear(book)
    EB_Book *book;
{
    eb_unset_subbook(book);
    eb_unset_language(book);

    if (book->languages != NULL)
	free(book->languages);

    if (book->subbooks != NULL)
	free(book->subbooks);

    if (book->messages != NULL)
	free(book->messages);

    if (book->path != NULL)
	free(book->path);

    eb_initialize(book);
    eb_zclear();
}


/*
 * Read information from the `CATALOG(S)' file in 'book'.
 *
 * If succeeded, the number of subbooks in the book is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
static int
eb_initialize_catalog(book)
    EB_Book *book;
{
    char buf[EB_SIZE_PAGE];
    char catalog[PATH_MAX + 1];
    char *space;
    EB_Subbook *sub;
    size_t catalogsize;
    size_t titlesize;
    int file = -1;
    int i;

    if (book->disc_code == EB_DISC_EB) {
	catalogsize = EB_SIZE_EB_CATALOG;
	titlesize = EB_MAXLEN_EB_TITLE;
    } else {
	catalogsize = EB_SIZE_EPWING_CATALOG;
	titlesize = EB_MAXLEN_EPWING_TITLE;
    }
	
    /*
     * Open the catalog file.
     */
    if (book->disc_code == EB_DISC_EB)
	sprintf(catalog, "%s/%s", book->path, EB_FILENAME_CATALOG);
    else
	sprintf(catalog, "%s/%s", book->path, EB_FILENAME_CATALOGS);
    eb_fix_filename(book, catalog);
    file = open(catalog, O_RDONLY | O_BINARY);
    if (file < 0)
	goto failed;

    /*
     * Get the number of subbooks in this book.
     */
    if (eb_read_all(file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_CAT;
	goto failed;
    }
    book->sub_count = eb_uint2(buf);
    if (EB_MAX_SUBBOOKS < book->sub_count)
	book->sub_count = EB_MAX_SUBBOOKS;
    if (EB_MAX_SUBBOOKS == 0) {
	book->sub_count = EB_ERR_UNEXP_CAT;
	goto failed;
    }

    /*
     * Allocate memories for subbook entries.
     */
    book->subbooks = (EB_Subbook *) malloc(sizeof(EB_Subbook)
	* book->sub_count);
    if (book->subbooks == NULL) {
	eb_error = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }

    /*
     * Read information about subbook.
     */
    for (i = 0, sub = book->subbooks; i < book->sub_count; i++, sub++) {
	/*
	 * Read data from the catalog file.
	 */
	if (eb_read_all(file, buf, catalogsize) != catalogsize) {
	    eb_error = EB_ERR_FAIL_READ_CAT;
	    goto failed;
	}

	/*
	 * Set a directory name.
	 */
	strncpy(sub->directory, buf + 2 + titlesize, EB_MAXLEN_BASENAME);
	sub->directory[EB_MAXLEN_BASENAME] = '\0';
	space = strchr(sub->directory, ' ');
	if (space != NULL)
	    *space = '\0';

	/*
	 * Set an index page.
	 */
	if (book->disc_code == EB_DISC_EB)
	    sub->index_page = 1;
	else {
	    sub->index_page = eb_uint2(buf + (2 + EB_MAXLEN_EPWING_TITLE
		+ EB_MAXLEN_BASENAME + 4));
	}

	/*
	 * Set a title.  (Convert from JISX0208 to EUC JP)
	 */
	strncpy(sub->title, buf + 2, titlesize);
	sub->title[titlesize] = '\0';
	if (book->char_code != EB_CHARCODE_ISO8859_1)
	    eb_jisx0208_to_euc(sub->title, sub->title);

	/*
	 * If the book is EPWING, get font filenames.
	 */
	if (book->disc_code == EB_DISC_EPWING) {
	    EB_Font *fnt = sub->fonts;
	    char *bufp;
	    int j;
	    int count = 0;

	    for (j = 0, bufp = buf + 2 + titlesize + 18;
		 j < EB_MAX_FONTS * 2; j++, bufp += EB_MAXLEN_BASENAME) {
		/*
		 * Skip this entry when the first character of the
		 * filename is not valid.
		 */
		if (*bufp == '\0' || 0x80 <= *((unsigned char *)bufp))
		    continue;
		strncpy(fnt->filename, bufp, EB_MAXLEN_BASENAME);
		fnt->filename[EB_MAXLEN_BASENAME] = '\0';
		space = strchr(fnt->filename, ' ');
		if (space != NULL)
		    *space = '\0';
		fnt++;
		count++;
	    }
	    sub->font_count = count;
	}

	sub->initialized = 0;
	sub->code = i;
    }

    /*
     * Close the catalog file.
     */
    close(file);

    /*
     * EB library cannot identify the character-code of the following
     * books correctly.  We fix the character code at this point.
     *
     *   1. CD-ROM of SONY DataDiskMan (DD-DR1)
     *      title of the 1st subbook: "%;%s%A%e%j!\%S%8%M%9!\%/%i%&%s"
     *   2. Shin Eiwa Waei Chujiten (earliest edition)
     *      title of the 1st subbook: "8&5f<R!!?71QOBCf<-E5".
     *   3. EB Kagakugijutu Yougo Daijiten (YRRS-048)
     *      title of the 1st subbook: "#E#B2J3X5;=QMQ8lBg<-E5".
     */
    if (strcmp(book->subbooks->title, "%;%s%A%e%j!\\%S%8%M%9!\\%/%i%&%s") == 0
	|| strcmp(book->subbooks->title, "8&5f<R!!?71QOBCf<-E5") == 0
	|| strcmp(book->subbooks->title, "#E#B2J3X5;=QMQ8lBg<-E5") == 0) {
	book->char_code = EB_CHARCODE_JISX0208;
	for (i = 0, sub = book->subbooks; i < book->sub_count; i++, sub++)
	    eb_jisx0208_to_euc(sub->title, sub->title);
    }

    return book->sub_count;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);

    if (book->subbooks != NULL) {
	free(book->subbooks);
	book->subbooks = NULL;
    }

    return -1;
}


/*
 * Test whether `book' is bound.
 */
int
eb_is_bound(book)
    EB_Book *book;
{
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return 0;
    }

    return 1;
}


/*
 * Return the bound path of `book'.
 */
const char *
eb_path(book)
    EB_Book *book;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return NULL;
    }

    return book->path;
}


/*
 * Inspect a disc type.
 */
EB_Disc_Code
eb_disc_type(book)
    EB_Book *book;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    return book->disc_code;
}


/*
 * Inspect a character code used in the book.
 */
EB_Character_Code
eb_character_code(book)
    EB_Book *book;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    return book->char_code;
}


