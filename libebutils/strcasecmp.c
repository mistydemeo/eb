/*
 * Copyright (c) 1997, 98, 2000, 01  
 *    Motoyuki Kasahara
 *
 * This programs is free software; you can redistribute it and/or modify
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

/*
 * Compare strings.
 * Cases in the strings are insensitive.
 */
int
strcasecmp(string1, string2)
    const char *string1;
    const char *string2;
{
    const unsigned char *string1_p = (const unsigned char *)string1;
    const unsigned char *string2_p = (const unsigned char *)string2;
    int c1, c2;

    while (*string1_p != '\0') {
	if ('a' <= *string1_p && *string1_p <= 'z')
	    c1 = *string1_p - ('a' - 'A');
	else
	    c1 = *string1_p;

	if ('a' <= *string2_p && *string2_p <= 'z')
	    c2 = *string2_p - ('a' - 'A');
	else
	    c2 = *string2_p;

	if (c1 != c2)
	    return c1 - c2;

	string1_p++;
	string2_p++;
    }

    return -(*string2_p);
}


/*
 * Compare strings within `n' characters.
 * Cases in the strings are insensitive.
 */
int
strncasecmp(string1, string2, n)
    const char *string1;
    const char *string2;
    size_t n;
{
    const unsigned char *string1_p = (const unsigned char *)string1;
    const unsigned char *string2_p = (const unsigned char *)string2;
    size_t i = n;
    int c1, c2;

    if (i <= 0)
	return 0;

    while (*string1_p != '\0') {
	if ('a' <= *string1_p && *string1_p <= 'z')
	    c1 = *string1_p - ('a' - 'A');
	else
	    c1 = *string1_p;

	if ('a' <= *string2_p && *string2_p <= 'z')
	    c2 = *string2_p - ('a' - 'A');
	else
	    c2 = *string2_p;

	if (c1 != c2)
	    return c1 - c2;

	string1_p++;
	string2_p++;
	i--;
	if (i <= 0)
	    return 0;
    }

    return -(*string2_p);
}
