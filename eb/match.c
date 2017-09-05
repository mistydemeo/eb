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

/*
 * Compare `word' and `pattern'.
 * `word' must be terminated by `\0' and `pattern' is assumed to be
 * `length' characters long.
 * 
 * When `word' is equal to `pattern', or equal to the beginning of
 * `pattern', 0 is returned.  A positive or negateive integer is
 * returned according as `pattern' is greater or less than `word'.
 */
int
eb_match_canonicalized_word(canonicalized_word, pattern, length)
    const char *canonicalized_word;
    const char *pattern;
    size_t length;
{
    int i = 0;
    unsigned char *word_p = (unsigned char *)canonicalized_word;
    unsigned char *pattern_p = (unsigned char *)pattern;

    for (;;) {
	if (length <= i)
	    return *word_p;
	if (*word_p == '\0')
	    return 0;

	if (*word_p != *pattern_p)
	    return *word_p - *pattern_p;
	
	word_p++;
	pattern_p++;
	i++;
    }

    /* not rearched */
    return 0;
}


/*
 * Compare `word' and `pattern'.
 * `word' must be terminated by `\0' and `pattern' is assumed to be
 * `length' characters long.
 * 
 * When the word is equal to the pattern, 0 is returned.  A positive or
 * negateive integer is returned according as `pattern' is greater or
 * less than `word'.
 */
int
eb_exact_match_canonicalized_word(canonicalized_word, pattern, length)
    const char *canonicalized_word;
    const char *pattern;
    size_t length;
{
    int i = 0;
    unsigned char *word_p = (unsigned char *)canonicalized_word;
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
	}
	if (*word_p != *pattern_p)
	    return *word_p - *pattern_p;

	word_p++;
	pattern_p++;
	i++;
    }

    /* not rearched */
    return 0;
}


/*
 * Compare `word' and `pattern'.
 *
 * This function is equivalent to eb_match_canonicalized_word() except
 * that this function ignores difference of case.  The word and pattern
 * are assumed to be written in ISO 8859-1.
 */
int
eb_match_word_latin(word, pattern, length)
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
	}
	if (isalpha(*word_p)
	    || 0xc0 <= *word_p && *word_p <= 0xd6
	    || 0xd8 <= *word_p && *word_p <= 0xde
	    || 0xe0 <= *word_p && *word_p <= 0xf6
	    || 0xf8 <= *word_p && *word_p <= 0xfe) {
	    if ((*word_p | 0x20) != (*pattern_p | 0x20))
		return *word_p - *pattern_p;
	} else {
	    if (*word_p != *pattern_p)
		return *word_p - *pattern_p;
	}
	word_p++;
	pattern_p++;
	i++;
    }

    /* not rearched */
    return 0;
}


/*
 * Compare `word' and `pattern'.
 *
 * This function is equivalent to eb_match_canonicalized_word() except
 * that this function ignores differences of case and kana (katakana and
 * hiragana).  The word and pattern are assumed to be written in JIS X
 * 0208.
 */
int
eb_match_word_jis(word, pattern, length)
    const char *word;
    const char *pattern;
    size_t length;
{
    int i = 0;
    unsigned char *word_p = (unsigned char *)word;
    unsigned char *pattern_p = (unsigned char *)pattern;
    unsigned char wc0, wc1, pc0, pc1;

    for (;;) {
	if (length <= i)
	    return *word_p;
	if (*word_p == '\0')
	    return 0;
	if (length <= i + 1 || *(word_p + 1) == '\0')
	    return *word_p - *pattern_p;

	wc0 = *word_p;
	wc1 = *(word_p + 1);
	pc0 = *pattern_p;
	pc1 = *(pattern_p + 1);

	if ((wc0 == 0x24 || wc0 == 0x25) && (pc0 == 0x24 || pc0 == 0x25)) {
	    if (wc1 != pc1)
		return wc1 - pc1;
	} else if (wc0 == 0x23 && pc0 == 0x23 && isalpha(wc1)) {
	    if ((wc1 | 0x20) != (pc1 | 0x20))
		return wc1 - pc1;
	} else {
	    if (wc0 != pc0)
		return wc0 - pc0;
	    if (wc1 != pc1)
		return wc1 - pc1;
	}
	word_p += 2;
	pattern_p += 2;
	i += 2;
    }

    /* not rearched */
    return 0;
}


/*
 * Compare `word' and `pattern'.
 *
 * This function is equivalent to eb_exact_match_canonicalized_word()
 * except that this function ignores difference of case.  The word and
 * pattern are assumed to be written in ISO 8859-1.
 */
int
eb_exact_match_word_latin(word, pattern, length)
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

	if (isalpha(*word_p)
	    || 0xc0 <= *word_p && *word_p <= 0xd6
	    || 0xd8 <= *word_p && *word_p <= 0xde
	    || 0xe0 <= *word_p && *word_p <= 0xf6
	    || 0xf8 <= *word_p && *word_p <= 0xfe) {
	    if ((*word_p | 0x20) != (*pattern_p | 0x20))
		return *word_p - *pattern_p;
	} else {
	    if (*word_p != *pattern_p)
		return *word_p - *pattern_p;
	}
	word_p++;
	pattern_p++;
	i++;
    }

    /* not rearched */
    return 0;
}


/*
 * Compare `word' and `pattern'.
 *
 * This function is equivalent to eb_exact_match_canonicalized_word()
 * except that this function ignores differences of case and kana
 * (katakana and hiragana).  The word and pattern are assumed to be
 * written in JIS X 02028.
 */
int
eb_exact_match_word_jis(word, pattern, length)
    const char *word;
    const char *pattern;
    size_t length;
{
    int i = 0;
    unsigned char *word_p = (unsigned char *)word;
    unsigned char *pattern_p = (unsigned char *)pattern;
    unsigned char wc0, wc1, pc0, pc1;

    for (;;) {
	if (length <= i)
	    return *word_p;
	if (*word_p == '\0')
	    return - *pattern_p;
	if (length <= i + 1 || *(word_p + 1) == '\0')
	    return *word_p - *pattern_p;

	wc0 = *word_p;
	wc1 = *(word_p + 1);
	pc0 = *pattern_p;
	pc1 = *(pattern_p + 1);

	if ((wc0 == 0x24 || wc0 == 0x25) && (pc0 == 0x24 || pc0 == 0x25)) {
	    if (wc1 != pc1)
		return wc1 - pc1;
	} else if (wc0 == 0x23 && pc0 == 0x23 && isalpha(wc1)) {
	    if ((wc1 | 0x20) != (pc1 | 0x20))
		return wc1 - pc1;
	} else {
	    if (wc0 != pc0)
		return wc0 - pc0;
	    if (wc1 != pc1)
		return wc1 - pc1;
	}
	word_p += 2;
	pattern_p += 2;
	i += 2;
    }

    /* not rearched */
    return 0;
}


