/*
 * Copyright (c) 1997, 1998  Motoyuki Kasahara
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
 * Read information from the `LANGUAGES' file in `book'.
 *
 * If succeeded, the number of messages in the book is returned.
 * Otherwise, -1 is returned and `eb_error' is set.
 */
int
eb_initialize_messages(book)
    EB_Book *book;
{
    EB_Language *lng;
    EB_Zip zip;
    int file = -1;
    ssize_t len;
    off_t offset;
    int maxmsgs;
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
    if (eb_zlseek(&zip, file, 
	(EB_MAXLEN_LANGNAME + 1) * book->lang_count + 16, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_LANG;
	goto failed;
    }

    /*
     * Get offset and size of each language.
     */
    len = eb_zread(&zip, file, buf, EB_SIZE_PAGE);
    if (len < EB_MAXLEN_MESSAGE + 1) {
	eb_error = EB_ERR_UNEXP_LANG;
	goto failed;
    }
    if (*buf != book->languages->code || strncmp(buf + 1,
	book->languages->name, EB_MAXLEN_LANGNAME + 1) != 0) {
	eb_error = EB_ERR_UNEXP_LANG;
	goto failed;
    }

    i = 0;
    lng = book->languages;
    bufp = buf + EB_MAXLEN_LANGNAME + 1;
    offset = 16 + (EB_MAXLEN_LANGNAME + 1) * book->lang_count
	+ (EB_MAXLEN_LANGNAME + 1);
    lng->offset = offset;

    for (;;) {
	int rest;
	EB_Language_Code code;

	/*
         * If `bufp' reached to a near of the end of the buffer,
	 * read next page.
         */
        rest = len - (bufp - buf);
        if (rest < EB_MAXLEN_MESSAGE + 1) {
	    int n;

            memcpy(buf, bufp, rest);
            n = eb_zread(&zip, file, buf + rest, EB_SIZE_PAGE - rest);
            if (n < 0 || rest + n < EB_MAXLEN_MESSAGE + 1)
                break;
	    len = n + rest;
	    bufp = buf;
        }

	code = eb_uint1(bufp);
	if (i + 1 < book->lang_count && code == (lng + 1)->code
	    && strncmp((lng + 1)->name, bufp + 1, EB_MAXLEN_LANGNAME + 1)
	    == 0) {
	    /*
	     * Next language name is found.  I
	     */
	    i++;
	    lng++;
	    bufp += EB_MAXLEN_LANGNAME + 1;
	    offset += EB_MAXLEN_LANGNAME + 1;
	    lng->offset = offset;
	    lng->msg_count = 0;
	} else if (code == 0 && *(bufp + 1) == '\0') {
	    /*
	     * Break at an empty message.
	     */
	    break;
	} else {
	    /*
	     * A message.
	     */
	    bufp += EB_MAXLEN_MESSAGE + 1;
	    offset += EB_MAXLEN_MESSAGE + 1;
	    lng->msg_count++;
	}
    }

    /*
     * Truncate the number of messages in a language if exceeded its limit.
     */
    for (i = 0, lng = book->languages; i < book->lang_count; i++, lng++) {
	if (EB_MAX_MESSAGES < lng->msg_count)
	    lng->msg_count = EB_MAX_MESSAGES;
    }

    /*
     * Decide the size for the message buffer, and allocate memories.
     */
    maxmsgs = 0;
    for (i = 0, lng = book->languages; i < book->lang_count; i++, lng++) {
	if (maxmsgs < lng->msg_count)
	    maxmsgs = lng->msg_count;
    }
    if (maxmsgs == 0)
	maxmsgs = 1;

    book->messages = (char *)malloc((EB_MAXLEN_MESSAGE + 2) * maxmsgs);
    if (book->messages == NULL) {
	eb_error = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
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

    if (book->messages != NULL) {
	free(book->messages);
	book->messages = NULL;
    }

    return -1;
}


/*
 * Return the number of messaegs in the current language.
 */
int
eb_message_count(book)
    EB_Book *book;
{
    /*
     * Current language must have been set.
     */
    if (book->lang_current == NULL) {
	eb_error = EB_ERR_NO_CUR_LANG;
	return -1;
    }

    return book->lang_current->msg_count;
}


/*
 * Get a list of messages in the current language.
 */
int
eb_message_list(book, list)
    EB_Book *book;
    EB_Message_Code *list;
{
    char *msg;
    int i;

    /*
     * Current language must have been set.
     */
    if (book->lang_current == NULL) {
	eb_error = EB_ERR_NO_CUR_LANG;
	return -1;
    }

    /*
     * Make messages list.
     */
    for (i = 0, msg = book->messages; i < book->lang_current->msg_count;
	 i++, list++, msg += EB_MAXLEN_MESSAGE + 2)
	*list = eb_uint1(msg);

    return book->lang_current->msg_count;
}


/*
 * Test whether the message `code' is exists in the current language.
 */
int
eb_have_message(book, code)
    EB_Book *book;
    EB_Message_Code code;
{
    char *msg;
    int c;
    int i;
    
    /*
     * Current language must have been set.
     */
    if (book->lang_current == NULL) {
	eb_error = EB_ERR_NO_CUR_LANG;
	return 0;
    }
    
    for (i = 0, msg = book->messages; i < book->lang_current->msg_count;
	 i++, msg += EB_MAXLEN_MESSAGE + 2) {
	c = eb_uint1(msg);
	if (c == code)
	    return 1;
    }

    eb_error = EB_ERR_NO_SUCH_MSG;
    return 0;
}


/*
 * Get the specified message in the current language.
 */
const char *
eb_message(book, code)
    EB_Book *book;
    EB_Message_Code code;
{
    char *msg;
    int c;
    int i;
    
    /*
     * Current language must have been set.
     */
    if (book->lang_current == NULL) {
	eb_error = EB_ERR_NO_CUR_LANG;
	return NULL;
    }
    
    for (i = 0, msg = book->messages; i < book->lang_current->msg_count;
	 i++, msg += EB_MAXLEN_MESSAGE + 2) {
	c = eb_uint1(msg);
	if (c == code)
	    return msg + 1;
    }

    eb_error = EB_ERR_NO_SUCH_MSG;
    return NULL;
}


