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

/*
 * This program requires the following Autoconf macros:
 *   AC_C_CONST
 *   AC_TYPE_SIZE_T
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else

/* Define to empty if the keyword `const' does not work.  */
/* #define const */

/* Define if `size_t' is not defined.  */
/* #define size_t unsigned */

#endif /* not HAVE_CONFIG_H */

#include <sys/types.h>

#ifndef VOID
#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif
#endif


/*
 * memmove() described in ISO 9899: 1990.
 */
VOID *
eb_memmove(destination, source, length)
    VOID *destination;
    const VOID *source;
    size_t length;
{
    char *d = (char *)destination;
    const char *s = (const char *)source;
    size_t i;
    
    if (s < d) {
	s += length - 1;
	d += length - 1;
	for (i = 0; i < length; i++)
	    *d-- = *s--;
    } else if (s != d) {
	for (i = 0; i < length; i++)
	    *d++ = *s++;
    }

    return destination;
}
