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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"

#ifndef ENABLE_PTHREAD
#define pthread_mutex_lock(m)
#define pthread_mutex_unlock(m)
#endif

/*
 * Mutex for gettext function call.
 */
#if defined(ENABLE_NLS) && defined(ENABLE_PTHREAD)
static pthread_mutex_t gettext_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Error messages.
 */
static const char *error_messages[] = {
    /* 0 -- 4 */
    N_("no error"),
    N_("memory exhausted"),
    N_("an empty file name"),
    N_("too long file name"),
    N_("too long word"),

    /* 5 -- 9 */
    N_("a word contains bad character"),
    N_("an empty word"),
    N_("failed to get the current working directory"),
    N_("failed to open CATALOG or CATALOGS file"),
    N_("failed to open CATALOG or CATALOGS file for appendix"),

    /* 10 -- 14 */
    N_("failed to open LANGUAGE file"),
    N_("failed to open START or HONMON file"),
    N_("failed to open a font file"),
    N_("failed to open APPENDIX or FUROKU file"),
    N_("failed to read CATALOG or CATALOGS file"),

    /* 15 -- 19 */
    N_("failed to read CATALOG or CATALOGS file for appendix"),
    N_("failed to read LANGUAGE file"),
    N_("failed to read START or HONMON file"),
    N_("failed to read a font file"),
    N_("failed to seek APPENDIX or FUROKU file"),

    /* 20 -- 24 */
    N_("failed to seek CATALOG or CATALOGS file"),
    N_("failed to seek CATALOG or CATALOGS file for appendix"),
    N_("failed to seek LANGUAGE file"),
    N_("failed to seek START or HONMON file"),
    N_("failed to seek a font file"),

    /* 25 -- 29 */
    N_("failed to seek APPENDIX or FUROKU file"),
    N_("unexpected format in CATALOG or CATALOGS file"),
    N_("unexpected format in CATALOG or CATALOGS file for appendix"),
    N_("unexpected format in LANGUAGE file"),
    N_("unexpected format in START or HONMON file"),

    /* 30 -- 34 */
    N_("unexpected format in a font file"),
    N_("unexpected format in APPENDIX or FUROKU file"),
    N_("book not bound"),
    N_("appendix not bound"),
    N_("no language"),

    /* 35 -- 39 */
    N_("no subbook"),
    N_("no appendix"),
    N_("no message"),
    N_("no font"),
    N_("no START or HONMON file"),

    /* 40 -- 44 */
    N_("no current language"),
    N_("no current subbook"),
    N_("no current appendix subbook"),
    N_("no current font"),
    N_("no such language"),

    /* 45 -- 49 */
    N_("no such subbook"),
    N_("no such appendix subbook"),
    N_("no such message"),
    N_("no such font"),
    N_("no such character bitmap"),

    /* 50 -- 54 */
    N_("no such character text"),
    N_("no such search method"),
    N_("no such hook"),
    N_("invalid hook workspace usage"),
    N_("different content type"),

    /* 55 -- 59 */
    N_("no previous search"),
    N_("no such multi search"),
    N_("no such multi search entry"),
    N_("too many words specified"),
    N_("no word specified"),

    /* 60 -- 64 */
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
    pthread_mutex_lock(&gettext_mutex);
    message = dgettext(EB_TEXT_DOMAIN_NAME, message);
    pthread_mutex_unlock(&gettext_mutex);
#endif /* ENABLE_NLS */

    return message;
}
