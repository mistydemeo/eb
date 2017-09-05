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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "language.h"

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
 * Read information from the `LANGUAGE' file in `book'.
 *
 * If succeeded, the number of languages in the book is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
int
eb_initialize_languages(book)
    EB_Book *book;
{
    EB_Language *lng;
    EB_Zip zip;
    int file = -1;
    int i;
    char language[PATH_MAX + 1];
    char buf[EB_SIZE_PAGE];
    char *bufp;

    /*
     * Open the language file.
     */
    sprintf(language, "%s/%s", book->path, EB_FILENAME_LANGUAGE);
    eb_fix_filename(book, language);
    file = eb_zopen(&zip, language);
    if (file < 0) {
	eb_error = EB_ERR_FAIL_OPEN_LANG;
	goto failed;
    }

    /*
     * Get a character code of the book, and get the number of langueages
     * in the file.
     */
    if (eb_zread(&zip, file, buf, 16) != 16) {
	eb_error = EB_ERR_FAIL_READ_LANG;
	goto failed;
    }

    book->char_code = eb_uint2(buf);
    if (book->char_code != EB_CHARCODE_ISO8859_1
	&& book->char_code != EB_CHARCODE_JISX0208
	&& book->char_code != EB_CHARCODE_JISX0208_GB2312) {
	eb_error = EB_ERR_UNEXP_LANG;
	goto failed;
    }

    book->lang_count = eb_uint2(buf + 2);
    if (EB_MAX_LANGUAGES < book->lang_count)
	book->lang_count = EB_MAX_LANGUAGES;
    if (book->lang_count == 0) {
	eb_error = EB_ERR_UNEXP_LANG;
	goto failed;
    }

    /*
     * Allocate memories for language entries.
     */
    book->languages = (EB_Language *) malloc(sizeof(EB_Language)
	* book->lang_count);
    if (book->languages == NULL) {
	eb_error = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }

    /*
     * Get languege names.
     */
    if (eb_zread(&zip, file, buf, (EB_MAXLEN_LANGNAME + 1)
	* book->lang_count) != (EB_MAXLEN_LANGNAME + 1) * book->lang_count) {
	eb_error = EB_ERR_FAIL_READ_LANG;
	goto failed;
    }
    for (i = 0, bufp = buf, lng = book->languages;
	 i < book->lang_count; i++, bufp += EB_MAXLEN_LANGNAME + 1, lng++) {
	lng->code = eb_uint1(bufp);
	lng->offset = 0;
	lng->msg_count = 0;
	memcpy(lng->name, bufp + 1, EB_MAXLEN_LANGNAME);
	*(lng->name + EB_MAXLEN_LANGNAME) = '\0';
    }

    /*
     * Close the language file.
     */
    eb_zclose(&zip, file);

    return book->lang_count;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	eb_zclose(&zip, file);

    if (book->languages != NULL) {
	free(book->languages);
	book->languages = NULL;
    }

    return -1;
}


/*
 * Return the number of languages int the book.
 */
int
eb_language_count(book)
    EB_Book *book;
{
    /*
     * The book must have been bound.
     */
    if (book->path == NULL) {
	eb_error = EB_ERR_UNBOUND_BOOK;
	return -1;
    }

    return book->lang_count;
}


/*
 * Make a list of languages the book has.
 */
int
eb_language_list(book, list)
    EB_Book *book;
    EB_Language_Code *list;
{
    EB_Language *lng;
    int i;

    /*
     * Language information in the book must have been initalized.
     */
    if (book->languages == NULL) {
	eb_error = EB_ERR_NO_LANG;
	return -1;
    }

    for (i = 0, lng = book->languages;
	 i < book->lang_count; i++, list++, lng++)
	*list = lng->code;

    return book->lang_count;
}


/*
 * Test that the book supports a language `code'.
 */
int
eb_have_language(book, code)
    EB_Book *book;
    EB_Language_Code code;
{
    EB_Language *lng;
    int i;

    /*
     * Language information in the book must have been loaded.
     */
    if (book->languages == NULL) {
	eb_error = EB_ERR_NO_LANG;
	return 0;
    }

    /*
     * Find the language entry.
     */
    for (i = 0, lng = book->languages; i < book->lang_count; i++, lng++) {
	if (lng->code == code)
	    return 1;
    }

    eb_error = EB_ERR_NO_SUCH_LANG;
    return 0;
}


/*
 * Return the current language code.
 */
EB_Language_Code
eb_language(book)
    EB_Book *book;
{
    /*
     * Current language must have been set.
     */
    if (book->lang_current == NULL) {
	eb_error = EB_ERR_NO_CUR_LANG;
	return -1;
    }

    return book->lang_current->code;
}


/*
 * Return a name of the current language.
 */
const char *
eb_language_name(book)
    EB_Book *book;
{
    /*
     * Current language must have been set.
     */
    if (book->lang_current == NULL) {
	eb_error = EB_ERR_NO_CUR_LANG;
	return NULL;
    }

    return book->lang_current->name;
}


/*
 * Return a language name of the number `code'.
 */
const char *
eb_language_name2(book, code)
    EB_Book *book;
    EB_Language_Code code;
{
    EB_Language *lng;
    int i;

    /*
     * Language information in the book must have been initialized.
     */
    if (book->languages == NULL) {
	eb_error = EB_ERR_NO_LANG;
	return NULL;
    }

    /*
     * Find the language entry.
     */
    for (i = 0, lng = book->languages; i < book->lang_count; i++, lng++) {
	if (lng->code == code)
	    return lng->name;
    }

    eb_error = EB_ERR_NO_SUCH_LANG;
    return NULL;
}


/*
 * Set a language `code' as the current language.
 */
int
eb_set_language(book, code)
    EB_Book *book;
    EB_Language_Code code;
{
    EB_Language *lng;
    EB_Zip zip;
    char *msg;
    char language[PATH_MAX + 1];
    int file = -1;
    int i;

    /*
     * Language information in the book must have been initalized,
     * and memories for messages must have been alloated.
     */
    if (book->languages == NULL) {
	eb_error = EB_ERR_NO_LANG;
	goto failed;
    }

    /*
     * If the language to set is the current languages, there is nothing
     * to be done.
     */
    if (book->lang_current != NULL && book->lang_current->code == code)
	return book->lang_current->msg_count;

    /*
     * Search a subbook which has language code `code'.
     */
    for (i = 0, lng = book->languages; i < book->lang_count; i++, lng++) {
	if (lng->code == code)
	    break;
    }
    if (book->lang_count <= i) {
	eb_error = EB_ERR_NO_SUCH_LANG;
	goto failed;
    }

    /*
     * Allocate memories for message buffer.
     */
    if (book->messages == NULL && eb_initialize_messages(book) < 0)
	goto failed;

    /*
     * Open the language file.
     * Length of the book path and subdir must be checked by caller.
     */
    sprintf(language, "%s/%s", book->path, EB_FILENAME_LANGUAGE);
    eb_fix_filename(book, language);
    file = eb_zopen(&zip, language);
    if (file < 0) {
	eb_error = EB_ERR_FAIL_OPEN_LANG;
	goto failed;
    }

    /*
     * Read messages.
     * Appends '\0' to each message.
     */
    if (eb_zlseek(&zip, file, lng->offset, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_LANG;
	goto failed;
    }
    for (i = 0, msg = book->messages;
	 i < lng->msg_count; i++, msg += EB_MAXLEN_MESSAGE + 2) {
	if (eb_zread(&zip, file, msg, EB_MAXLEN_MESSAGE + 1)
	    != EB_MAXLEN_MESSAGE + 1) {
	    eb_error = EB_ERR_FAIL_READ_LANG;
	    goto failed;
	}
	*(msg + EB_MAXLEN_MESSAGE + 1) = '\0';
    }

    /*
     * Convert messages from SJIS to JIS when language is Japanese.
     */
    if (lng->code == EB_LANG_JAPANESE) {
        for (i = 0, msg = book->messages;
	     i < lng->msg_count; i++, msg += EB_MAXLEN_MESSAGE + 2)
	    eb_sjis_to_euc(msg + 1, msg + 1);
    }
    book->lang_current = lng;

    /*
     * Close the language file.
     */
    eb_zclose(&zip, file);

    return lng->msg_count;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	eb_zclose(&zip, file);
    book->lang_current = NULL;
    return -1;
}


/*
 * Unset the current language.
 */
void
eb_unset_language(book)
    EB_Book *book;
{
    book->lang_current = NULL;
}


