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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
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
 * Otherwise, -1 is returned.
 */
EB_Error_Code
eb_initialize_languages(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Language *language;
    EB_Zip zip;
    int file = -1;
    char language_file_name[PATH_MAX + 1];
    char buffer[EB_SIZE_PAGE];
    char *buffer_p;
    int i;

    /*
     * Open the language file.
     */
    sprintf(language_file_name, "%s/%s", book->path, EB_FILE_NAME_LANGUAGE);
    eb_fix_file_name(book, language_file_name);
    file = eb_zopen(&zip, language_file_name);
    if (file < 0) {
	error_code = EB_ERR_FAIL_OPEN_LANG;
	goto failed;
    }

    /*
     * Get a character code of the book, and get the number of langueages
     * in the file.
     */
    if (eb_zread(&zip, file, buffer, 16) != 16) {
	error_code = EB_ERR_FAIL_READ_LANG;
	goto failed;
    }

    book->character_code = eb_uint2(buffer);
    if (book->character_code != EB_CHARCODE_ISO8859_1
	&& book->character_code != EB_CHARCODE_JISX0208
	&& book->character_code != EB_CHARCODE_JISX0208_GB2312) {
	error_code = EB_ERR_UNEXP_LANG;
	goto failed;
    }

    book->language_count = eb_uint2(buffer + 2);
    if (EB_MAX_LANGUAGES < book->language_count)
	book->language_count = EB_MAX_LANGUAGES;
    if (book->language_count == 0) {
	error_code = EB_ERR_UNEXP_LANG;
	goto failed;
    }

    /*
     * Allocate memories for language entries.
     */
    book->languages = (EB_Language *) malloc(sizeof(EB_Language)
	* book->language_count);
    if (book->languages == NULL) {
	error_code = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }

    /*
     * Get languege names.
     */
    if (eb_zread(&zip, file, buffer, (EB_MAX_LANGUAGE_NAME_LENGTH + 1)
	* book->language_count)
	!= (EB_MAX_LANGUAGE_NAME_LENGTH + 1) * book->language_count) {
	error_code = EB_ERR_FAIL_READ_LANG;
	goto failed;
    }
    for (i = 0, buffer_p = buffer, language = book->languages;
	 i < book->language_count;
	 i++, buffer_p += EB_MAX_LANGUAGE_NAME_LENGTH + 1, language++) {
	language->code = eb_uint1(buffer_p);
	language->offset = 0;
	language->message_count = 0;
	memcpy(language->name, buffer_p + 1, EB_MAX_LANGUAGE_NAME_LENGTH);
	*(language->name + EB_MAX_LANGUAGE_NAME_LENGTH) = '\0';
    }

    /*
     * Close the language file.
     */
    eb_zclose(&zip, file);

    return EB_SUCCESS;

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
    return error_code;
}


/*
 * Make a list of languages the book has.
 */
EB_Error_Code
eb_language_list(book, language_list, language_count)
    EB_Book *book;
    EB_Language_Code *language_list;
    int *language_count;
{
    EB_Error_Code error_code;
    EB_Language *language;
    EB_Language_Code *list_p;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Language information in the book must have been initalized.
     */
    if (book->languages == NULL) {
	error_code = EB_ERR_NO_LANG;
	goto failed;
    }

    /*
     * Make a language list.
     */
    for (i = 0, language = book->languages, list_p = language_list;
	 i < book->language_count; i++, language++, list_p++)
	*list_p = language->code;
    *language_count = book->language_count;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *language_count = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Test that the book supports a language `language_code'.
 */
int
eb_have_language(book, language_code)
    EB_Book *book;
    EB_Language_Code language_code;
{
    EB_Language *language;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Language information in the book must have been loaded.
     */
    if (book->languages == NULL)
	goto failed;

    /*
     * Find the language entry.
     */
    for (i = 0, language = book->languages; i < book->language_count;
	 i++, language++) {
	if (language->code == language_code)
	    break;
    }
    if (book->language_count <= i)
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
 * Return the current language code.
 */
EB_Error_Code
eb_language(book, language_code)
    EB_Book *book;
    EB_Language_Code *language_code;
{
    EB_Error_Code error_code;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current language must have been set.
     */
    if (book->language_current == NULL) {
	error_code = EB_ERR_NO_CUR_LANG;
	goto failed;
    }

    /*
     * Copy the language counter to `language_count'.
     */
    *language_code = book->language_current->code;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *language_code = EB_LANGUAGE_INVALID;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a name of the current language.
 */
EB_Error_Code
eb_language_name(book, language_name)
    EB_Book *book;
    char *language_name;
{
    EB_Error_Code error_code;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current language must have been set.
     */
    if (book->language_current == NULL) {
	error_code = EB_ERR_NO_CUR_LANG;
	goto failed;
    }

    /*
     * Copy the language name to `language_name'.
     */
    strcpy(language_name, book->language_current->name);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *language_name = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Return a language name of the number `language_code'.
 */
EB_Error_Code
eb_language_name2(book, language_code, language_name)
    EB_Book *book;
    EB_Language_Code language_code;
    char *language_name;
{
    EB_Error_Code error_code;
    EB_Language *language;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Language information in the book must have been initialized.
     */
    if (book->languages == NULL) {
	error_code = EB_ERR_NO_LANG;
	goto failed;
    }

    /*
     * Find the language entry.
     */
    for (i = 0, language = book->languages; i < book->language_count;
	 i++, language++) {
	if (language->code == language_code)
	    break;
    }
    if (book->language_count <= i) {
	error_code = EB_ERR_NO_SUCH_LANG;
	goto failed;
    }

    /*
     * Copy the language name to `language_name'.
     */
    strcpy(language_name, language->name);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *language_name = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Set a language `language_code' as the current language.
 */
EB_Error_Code
eb_set_language(book, language_code)
    EB_Book *book;
    EB_Language_Code language_code;
{
    EB_Error_Code error_code;
    EB_Zip zip;
    EB_Language *language;
    char language_file_name[PATH_MAX + 1];
    char *message;
    int file = -1;
    int i;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Language information in the book must have been initalized,
     * and memories for messages must have been alloated.
     */
    if (book->languages == NULL) {
	error_code = EB_ERR_NO_LANG;
	goto failed;
    }

    /*
     * If the language to set is the current languages, there is nothing
     * to be done.
     */
    if (book->language_current != NULL
	&& book->language_current->code == language_code) {
	language = book->language_current;
	goto succeeded;
    }

    /*
     * Search a subbook which has language code `language_code'.
     */
    for (i = 0, language = book->languages; i < book->language_count;
	 i++, language++) {
	if (language->code == language_code)
	    break;
    }
    if (book->language_count <= i) {
	error_code = EB_ERR_NO_SUCH_LANG;
	goto failed;
    }

    /*
     * Allocate memories for message buffer.
     */
    if (book->messages == NULL) {
	error_code = eb_initialize_messages(book);
	if (error_code != EB_SUCCESS)
	    goto failed;
    }

    /*
     * Open the language file.
     * Length of the book path and subdir must be checked by caller.
     */
    sprintf(language_file_name, "%s/%s", book->path, EB_FILE_NAME_LANGUAGE);
    eb_fix_file_name(book, language_file_name);
    file = eb_zopen(&zip, language_file_name);
    if (file < 0) {
	error_code = EB_ERR_FAIL_OPEN_LANG;
	goto failed;
    }

    /*
     * Read messages.
     * Appends '\0' to each message.
     */
    if (eb_zlseek(&zip, file, language->offset, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_LANG;
	goto failed;
    }
    for (i = 0, message = book->messages;
	 i < language->message_count;
	 i++, message += EB_MAX_MESSAGE_LENGTH + 2) {
	if (eb_zread(&zip, file, message, EB_MAX_MESSAGE_LENGTH + 1)
	    != EB_MAX_MESSAGE_LENGTH + 1) {
	    error_code = EB_ERR_FAIL_READ_LANG;
	    goto failed;
	}
	*(message + EB_MAX_MESSAGE_LENGTH + 1) = '\0';
    }

    /*
     * Convert messages from SJIS to JIS when language is Japanese.
     */
    if (language->code == EB_LANGUAGE_JAPANESE) {
        for (i = 0, message = book->messages; i < language->message_count;
	     i++, message += EB_MAX_MESSAGE_LENGTH + 2)
	    eb_sjis_to_euc(message + 1, message + 1);
    }
    book->language_current = language;

    /*
     * Close the language file.
     */
    eb_zclose(&zip, file);

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
    if (0 <= file)
	eb_zclose(&zip, file);
    book->language_current = NULL;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Unset the current language.
 */
void
eb_unset_language(book)
    EB_Book *book;
{
    eb_lock(&book->lock);
    book->language_current = NULL;
    eb_unlock(&book->lock);
}


