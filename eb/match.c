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

/*
 * Compare `word' and `pattern' within `length' characters.
 * 
 * When the word is equal to the pattern, or equal to the beginning of
 * the pattern, 0 is returned.  Otherwise, an integer less than or 
 * greater than zero is returned.  The return value is depends on whether
 * the word is greater or less than the pattern in dictionary order.
 */
int
eb_match_word(word, pattern, length)
    const char *word;
    const char *pattern;
    size_t length;
{
    int i = 0;
    unsigned char *word_p = (unsigned char *)word;
    unsigned char *pattern_p = (unsigned char *)pattern;

    for (;;) {
	if (length <= i)
	    return *word_p;

	if (*word_p == '\0')
	    return 0;
	else if (*word_p != *pattern_p)
	    return *word_p - *pattern_p;
	else {
	    word_p++;
	    pattern_p++;
	}

	i++;
    }

    /* not rearched */
    return 0;
}


/*
 * Compare `word' and `pattern' within `length' characters.
 * 
 * When the word is equal to the pattern, 0 is returned.
 * Otherwise, an integer less than or greater than zero is returned.
 * The return value of the case is depends on whether the word is
 * greater or less than the pattern in dictionary order.
 */
int
eb_match_exactword(word, pattern, length)
    const char *word;
    const char *pattern;
    size_t length;
{
    int i = 0;
    unsigned char *word_p = (unsigned char *)word;
    unsigned char *pattern_p = (unsigned char *)pattern;

    for (;;) {
	if (length <= i)
	    return *word_p;

	if (*word_p == '\0') {
	    /* ignore spaces in the tail of the pattern */
	    while (i < length && (*pattern_p == ' ' || *pattern_p == '\0')) {
		pattern_p++;
		i++;
	    }
	    return (i - length);
	} else if (*word_p != *pattern_p) {
	    return *word_p - *pattern_p;
	} else {
	    word_p++;
	    pattern_p++;
	}

	i++;
    }

    /* not rearched */
    return 0;
}


