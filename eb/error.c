/*
 * Copyright (c) 1997, 98, 2000, 01  
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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "internal.h"

/*
 * Mutex for gettext function call.
 */
#if defined(ENABLE_NLS) && defined(ENABLE_PTHREAD)
pthread_mutex_t gettext_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Error messages.
 */
static const char * const error_messages[] = {
    /* 0 -- 4 */
    N_("no error"),
    N_("memory exhausted"),
    N_("an empty file name"),
    N_("too long file name"),
    N_("bad file name"),

    /* 5 -- 9 */
    N_("bad directory name"),
    N_("too long word"),
    N_("a word contains bad character"),
    N_("an empty word"),
    N_("failed to get the current working directory"),

    /* 10 -- 14 */
    N_("failed to open a catalog file"),
    N_("failed to open an appendix catalog file"),
    N_("failed to open a language file"),
    N_("failed to open a text file"),
    N_("failed to open a font file"),

    /* 15 -- 19 */
    N_("failed to open an appendix file"),
    N_("failed to open a binary file"),
    N_("failed to read a catalog file"),
    N_("failed to read an appendix catalog file"),
    N_("failed to read a language file"),

    /* 20 -- 24 */
    N_("failed to read a text file"),
    N_("failed to read a font file"),
    N_("failed to read a binary file"),
    N_("failed to seek an appendix file"),
    N_("failed to seek a catalog file"),

    /* 25 -- 29 */
    N_("failed to seek an appendix catalog file"),
    N_("failed to seek a language file"),
    N_("failed to seek a text file"),
    N_("failed to seek a font file"),
    N_("failed to seek an appendix file"),

    /* 30 -- 34 */
    N_("failed to seek a binary file"),
    N_("unexpected format in a catalog file"),
    N_("unexpected format in an appendix catalog file"),
    N_("unexpected format in a language file"),
    N_("unexpected format in a text file"),

    /* 35 -- 39 */
    N_("unexpected format in a font file"),
    N_("unexpected format in an appendix file"),
    N_("unexpected format in a binary file"),
    N_("book not bound"),
    N_("appendix not bound"),

    /* 40 -- 44 */
    N_("no language"),
    N_("no subbook"),
    N_("no subbook in the appendix"),
    N_("no message"),
    N_("no font"),

    /* 45 -- 49 */
    N_("no text file"),
    N_("no current language"),
    N_("no current subbook"),
    N_("no current appendix subbook"),
    N_("no current font"),

    /* 50 -- 54 */
    N_("no current binary"),
    N_("no such language"),
    N_("no such subbook"),
    N_("no such appendix subbook"),
    N_("no such message"),

    /* 55 -- 59 */
    N_("no such font"),
    N_("no such character bitmap"),
    N_("no such character text"),
    N_("no such search method"),
    N_("no such hook"),

    /* 60 -- 64 */
    N_("no such binary"),
    N_("stop code found"),
    N_("different content type"),
    N_("no previous search"),
    N_("no such multi search"),

    /* 65 -- 69 */
    N_("no such multi search entry"),
    N_("too many words specified"),
    N_("no word specified"),
    N_("no candidates"),
    NULL
};

/*
 * "Unknown error", the special error message.
 */
static const char *unknown = N_("unknown error");


/*
 * Look up the error message corresponding to the error code `error_code'.
 */
const char *
eb_error_message(error_code)
    EB_Error_Code error_code;
{
    const char *message;

    if (0 <= error_code && error_code < EB_NUMBER_OF_ERRORS)
        message = error_messages[error_code];
    else
        message = unknown;

#ifdef ENABLE_NLS
    message = dgettext(EB_TEXT_DOMAIN_NAME, message);
#endif /* ENABLE_NLS */

    return message;
}


/*
 * Look up the error message corresponding to the error code `error_code',
 * and copy the message to `buffer'.  The maximum length of `buffer' is
 * EB_MAX_ERROR_MESSAGE_LENGTH.
 * This is thread-safe version of `eb_error_messge'.
 */
const char *
eb_error_message_r(error_code, buffer)
    EB_Error_Code error_code;
    char *buffer;
{
    const char *message;

    if (0 <= error_code && error_code < EB_NUMBER_OF_ERRORS)
        message = error_messages[error_code];
    else
        message = unknown;

    eb_dgettext_r(EB_TEXT_DOMAIN_NAME, message, buffer,
	EB_MAX_ERROR_MESSAGE_LENGTH + 1);

    return buffer;
}
