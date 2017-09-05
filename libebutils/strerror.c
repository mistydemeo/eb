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

/*
 * Requirements for Autoconf:
 *   AC_C_CONST
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

extern char *sys_errlist[];
extern int sys_nerr;


/*
 * strerror() described in ISO 9899: 1990.
 */
char *
strerror(errnum)
    int errnum;
{
    if (0 < errnum && errnum <= sys_nerr)
        return sys_errlist[errnum];

    return "Unknown error";
}


/*
 * perror() described in ISO 9899: 1990.
 */
void
perror(message)
    const char *message;
{
    if (message != NULL)
	fprintf(stderr, "%s: ", message);

    if (0 < errno && errno <= sys_nerr) {
        fputs(sys_errlist[errno], stderr);
	fputc('\n', stderr);
    } else
	fputs("Unknown error\n", stderr);
}
