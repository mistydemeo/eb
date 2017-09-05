/*                                                            -*- C -*-
 * Copyright (c) 1998, 2000  Motoyuki Kasahara
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

/*
 * This program requires the following Autoconf macros:
 *   AC_C_CONST
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h)
 *   AC_CHECK_FUNCS(strchr)
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

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#define YESNO_RESPONSE_SIZE	80

/*
 * Get y/n response to `message'.
 * If `y' is selected, 1 is returned.  Otherwise 0 is returned.
 */
int
query_y_or_n(message)
    const char *message;
{
    char response[YESNO_RESPONSE_SIZE];
    char *head, *tail;

    for (;;) {
	/*
	 * Output `message' to standard error.
	 */
	fputs(message, stderr);
	fflush(stderr);

	/*
	 * Get a line from standard in.
	 */
	if (fgets(response, YESNO_RESPONSE_SIZE, stdin) == NULL)
	    return 0;
	if (strchr(response, '\n') == NULL)
	    continue;

	/*
	 * Delete spaces and tabs in the beginning and end of the
	 * line.
	 */
        for (head = response; *head == ' ' || *head == '\t'; head++)
            ;
	for (tail = head + strlen(head) - 1;
	     head <= tail && (*tail == ' ' || *tail == '\t' || *tail == '\n');
	     tail--)
            *tail = '\0';

	/*
	 * Return if the line is `y' or `n'.
	 */
	if ((*head == 'Y' || *head == 'y') && *(head + 1) == '\0')
	    return 1;
	if ((*head == 'N' || *head == 'n') && *(head + 1) == '\0')
	    return 0;
    }

    /* not reached */
    return 0;
}
