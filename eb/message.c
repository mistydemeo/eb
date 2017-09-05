/*
 * Copyright (c) 1997, 98, 2000  Motoyuki Kasahara
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
#include "internal.h"
#include "language.h"

/*
 * Read information from the `LANGUAGES' file in `book'.
 *
 * If succeeded, 0 is returned.  Otherwise, -1 is returned.
 */
EB_Error_Code
eb_initialize_messages(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Language *language;
    EB_Zip zip;
    int file = -1;
    ssize_t read_length;
    off_t location;
    int max_messages;
    char language_path_name[PATH_MAX + 1];
    char buffer[EB_SIZE_PAGE];
    char *buffer_p;
    int i;

    /*
     * Open the language file.
     */
    if (eb_compose_path_name(book->path, EB_FILE_NAME_LANGUAGE,
	EB_SUFFIX_NONE, language_path_name) == 0) {
	file = eb_zopen(&zip, language_path_name, EB_ZIP_NONE);
    } else if (eb_compose_path_name(book->path, EB_FILE_NAME_LANGUAGE,
	EB_SUFFIX_EBZ, language_path_name) == 0) {
	file = eb_zopen(&zip, language_path_name, EB_ZIP_EBZIP1);
    }
    if (file < 0) {
	error_code = EB_ERR_FAIL_OPEN_LANG;
	goto failed;
    }

    /*
     * Get a character code of the book, and get the number of langueages
     * in the file.
     */
    if (eb_zlseek(&zip, file, (off_t)(EB_MAX_LANGUAGE_NAME_LENGTH + 1)
	* book->language_count + 16, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_LANG;
	goto failed;
    }

    /*
     * Get offset and size of each language.
     */
    read_length = eb_zread(&zip, file, buffer, EB_SIZE_PAGE);
    if (read_length < EB_MAX_MESSAGE_LENGTH + 1) {
	error_code = EB_ERR_UNEXP_LANG;
	goto failed;
    }
    if (*buffer != book->languages->code || strncmp(buffer + 1,
	book->languages->name, EB_MAX_LANGUAGE_NAME_LENGTH + 1) != 0) {
	error_code = EB_ERR_UNEXP_LANG;
	goto failed;
    }

    i = 0;
    language = book->languages;
    buffer_p = buffer + EB_MAX_LANGUAGE_NAME_LENGTH + 1;
    location = (off_t)16
	+ (EB_MAX_LANGUAGE_NAME_LENGTH + 1) * book->language_count
	+ (EB_MAX_LANGUAGE_NAME_LENGTH + 1);
    language->location = location;

    for (;;) {
	int rest_length;
	EB_Language_Code language_code;

	/*
         * If `buffer_p' reached to a near of the end of the buffer,
	 * read next page.
         */
        rest_length = read_length - (buffer_p - buffer);
        if (rest_length < EB_MAX_MESSAGE_LENGTH + 1) {
	    int n;

            memmove(buffer, buffer_p, rest_length);
            n = eb_zread(&zip, file, buffer + rest_length,
		EB_SIZE_PAGE - rest_length);
            if (n < 0 || rest_length + n < EB_MAX_MESSAGE_LENGTH + 1)
                break;
	    read_length = n + rest_length;
	    buffer_p = buffer;
        }

	language_code = eb_uint1(buffer_p);
	if (i + 1 < book->language_count
	    && language_code == (language + 1)->code
	    && strncmp((language + 1)->name, buffer_p + 1,
		EB_MAX_LANGUAGE_NAME_LENGTH + 1) == 0) {
	    /*
	     * Next language name is found.  I
	     */
	    i++;
	    language++;
	    buffer_p += EB_MAX_LANGUAGE_NAME_LENGTH + 1;
	    location += EB_MAX_LANGUAGE_NAME_LENGTH + 1;
	    language->location = location;
	    language->message_count = 0;
	} else if (language_code == 0 && *(buffer_p + 1) == '\0') {
	    /*
	     * Break at an empty message.
	     */
	    break;
	} else {
	    /*
	     * A message.
	     */
	    buffer_p += EB_MAX_MESSAGE_LENGTH + 1;
	    location += EB_MAX_MESSAGE_LENGTH + 1;
	    language->message_count++;
	}
    }

    /*
     * Truncate the number of messages in a language if exceeds its limit.
     */
    for (i = 0, language = book->languages; i < book->language_count;
	 i++, language++) {
	if (EB_MAX_MESSAGES < language->message_count)
	    language->message_count = EB_MAX_MESSAGES;
    }

    /*
     * Decide the size for the message buffer, and allocate memories.
     */
    max_messages = 0;
    for (i = 0, language = book->languages; i < book->language_count;
	 i++, language++) {
	if (max_messages < language->message_count)
	    max_messages = language->message_count;
    }
    if (max_messages == 0)
	max_messages = 1;

    book->messages = (char *)malloc((EB_MAX_MESSAGE_LENGTH + 2)
	* max_messages);
    if (book->messages == NULL) {
	error_code = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
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

    if (book->messages != NULL) {
	free(book->messages);
	book->messages = NULL;
    }

    return error_code;
}


/*
 * Get a list of messages in the current language.
 */
EB_Error_Code
eb_message_list(book, message_list, message_count)
    EB_Book *book;
    EB_Message_Code *message_list;
    int *message_count;
{
    EB_Error_Code error_code;
    EB_Message_Code *list_p;
    char *message;
    int i;

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
     * Make messages list.
     */
    for (i = 0, message = book->messages, list_p = message_list;
	 i < book->language_current->message_count;
	 i++, message += EB_MAX_MESSAGE_LENGTH + 2, list_p++)
	*list_p = eb_uint1(message);

    *message_count = book->language_current->message_count;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *message_count = 0;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Test whether the message `message_code' is exists in the current
 * language.
 */
int
eb_have_message(book, message_code)
    EB_Book *book;
    EB_Message_Code message_code;
{
    char *message;
    int c;
    int i;
    
    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current language must have been set.
     */
    if (book->language_current == NULL)
	goto failed;
    
    for (i = 0, message = book->messages;
	 i < book->language_current->message_count;
	 i++, message += EB_MAX_MESSAGE_LENGTH + 2) {
	c = eb_uint1(message);
	if (c == message_code)
	    break;
    }

    if (book->language_current->message_count <= i)
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
 * Get the specified message in the current language.
 */
EB_Error_Code
eb_message(book, message_code, message)
    EB_Book *book;
    EB_Message_Code message_code;
    char *message;
{
    EB_Error_Code error_code;
    const char *m;
    int c;
    int i;
    
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
    
    for (i = 0, m = book->messages;
	 i < book->language_current->message_count;
	 i++, m += EB_MAX_MESSAGE_LENGTH + 2) {
	c = eb_uint1(m);
	if (c == message_code)
	    break;
    }

    if (book->language_current->message_count <= i) {
	error_code = EB_ERR_NO_SUCH_MSG;
	goto failed;
    }

    strcpy(message, m + 1);

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *message = '\0';
    eb_unlock(&book->lock);
    return error_code;
}


