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

#include "eb.h"
#include "error.h"

/*
 * Error code.
 */
EB_Error_Code eb_error = EB_NO_ERR;

/*
 * Error messages.
 */
static char *messages[] = {
    /* 0 -- 4 */
    "no error",
    "memory exhausted",
    "an empty filename",
    "too long filename",
    "too long word",

    /* 5 -- 9 */
    "a word contains bad character",
    "an empty word",
    "cannot get the current working directory",
    "failed to open CATALOG or CATALOGS file",
    "failed to open CATALOG or CATALOGS file for appendix",

    /* 10 -- 14 */
    "failed to open LANGUAGE file",
    "failed to open START or HONMON file",
    "failed to open a font file",
    "failed to open APPENDIX or FUROKU file",
    "failed to read CATALOG or CATALOGS file",

    /* 15 -- 19 */
    "failed to read CATALOG or CATALOGS file for appendix",
    "failed to read LANGUAGE file",
    "failed to read START or HONMON file",
    "failed to read a font file",
    "failed to seek APPENDIX or FUROKU file",

    /* 20 -- 24 */
    "failed to seek CATALOG or CATALOGS file",
    "failed to seek CATALOG or CATALOGS file for appendix",
    "failed to seek LANGUAGE file",
    "failed to seek START or HONMON file",
    "failed to seek a font file",

    /* 25 -- 29 */
    "failed to seek APPENDIX or FUROKU file",
    "unexpected format in CATALOG or CATALOGS file",
    "unexpected format in CATALOG or CATALOGS file for appendix",
    "unexpected format in LANGUAGE file",
    "unexpected format in START or HONMON file",

    /* 30 -- 34 */
    "unexpected format in a font file",
    "unexpected format in APPENDIX or FUROKU file",
    "book not bound",
    "appendix not bound",
    "no language",

    /* 35 -- 39 */
    "no subbook",
    "no appendix",
    "no message",
    "no font",
    "no start file",

    /* 40 -- 44 */
    "no current language",
    "no current subbook",
    "no current appendix subbook",
    "no current font",
    "no such language",

    /* 45 -- 49 */
    "no such subbook",
    "no such appendix subbook",
    "no such message",
    "no such font",
    "no such character bitmap",

    /* 50 -- 54 */
    "no such character text",
    "no such search method",
    "no such hook",
    "invalid hook workspace usage",
    "different content type",

    /* 55 -- 59 */
    "different subbook",
    "different book",
    "no previous search",
    "no previous content",
    "no such multi-id",

    /* 60 -- 64 */
    "no such entry-id",
    NULL
};

/*
 * "Unknown error", the special error message.
 */
static const char *unknown = "unknown error";


/*
 * Look up the error message corresponding to the current error code.
 */
const char *
eb_error_message()
{
    if (eb_error < 0 || EB_NUM_ERRORS <= eb_error)
	return unknown;

    return messages[eb_error];
}


/*
 * Look up the error message corresponding to the error code `code'.
 */
const char *
eb_error_message2(code)
    EB_Error_Code code;
{
    if (code < 0 || EB_NUM_ERRORS <= code)
	return unknown;

    return messages[code];
}

