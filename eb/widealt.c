/*
 * Copyright (c) 1997, 1998, 1999  Motoyuki Kasahara
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "eb.h"
#include "error.h"
#include "appendix.h"
#include "internal.h"

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef __STDC__
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ */
#endif

/*
 * Unexported functions.
 */
static int eb_wide_character_text_jis EB_P((EB_Appendix *, int, char *));
static int eb_wide_character_text_latin EB_P((EB_Appendix *, int, char *));

/*
 * Hash macro for cache data.
 */
#define EB_HASH_ALT_CACHE(c)	((c) & 0x0f)


/*
 * Examine whether the current subbook in `book' has a wide font
 * alternation or not.
 */
int
eb_have_wide_alt(appendix)
    EB_Appendix *appendix;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return 0;
    }

    if (appendix->sub_current->wide_page == 0) {
	eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	return 0;
    }

    return 1;
}


/*
 * Look up the character number of the start of the wide font alternation
 * of the current subbook in `book'.
 */
int
eb_wide_alt_start(appendix)
    EB_Appendix *appendix;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return 0;
    }

    if (appendix->sub_current->wide_page == 0) {
	eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	return 0;
    }

    return appendix->sub_current->wide_start;
}


/*
 * Return the character number of the end of the wide font alternation
 * of the current subbook in `book'.
 */
int
eb_wide_alt_end(appendix)
    EB_Appendix *appendix;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return 0;
    }

    if (appendix->sub_current->wide_page == 0) {
	eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	return 0;
    }

    return appendix->sub_current->wide_end;
}


/*
 * Get the alternation text of the character number `ch'.
 */
int
eb_wide_alt_character_text(appendix, ch, text)
    EB_Appendix *appendix;
    int ch;
    char *text;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return -1;
    }

    /*
     * The wide font must be exist in the current subbook.
     */
    if (appendix->sub_current->wide_page == 0) {
	eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	return -1;
    }

    if (appendix->sub_current->char_code == EB_CHARCODE_ISO8859_1)
	return eb_wide_character_text_latin(appendix, ch, text);
    else
	return eb_wide_character_text_jis(appendix, ch, text);

    /* not reached */
    return 0;
}


/*
 * Get the alternation text of the character number `ch'.
 */
static int
eb_wide_character_text_jis(appendix, ch, text)
    EB_Appendix *appendix;
    int ch;
    char *text;
{
    int start = appendix->sub_current->wide_start;
    int end = appendix->sub_current->wide_end;
    int location;
    EB_Alternation_Cache *cachep;

    start = appendix->sub_current->wide_start;
    end = appendix->sub_current->wide_end;

    /*
     * Check for `ch'.  Is it in a font?
     * This test works correctly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (ch < start || end < ch || (ch & 0xff) < 0x21 || 0x7e < (ch & 0xff)) {
	eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	return -1;
    }

    /*
     * Calculate the location of alternation data.
     */
    location = (appendix->sub_current->wide_page - 1) * EB_SIZE_PAGE
	+ (((ch >> 8) - (start >> 8)) * 0x5e + (ch & 0xff) - (start & 0xff))
	* (EB_MAXLEN_ALTERNATION_TEXT + 1);

    /*
     * Check for the cache data.
     */
    cachep = appendix->wide_cache + EB_HASH_ALT_CACHE(ch);
    if (cachep->charno == ch) {
	memcpy(text, cachep->text, EB_MAXLEN_ALTERNATION_TEXT + 1);
	return 0;
    }

    /*
     * Read the alternation data.
     */
    if (eb_zlseek(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, location, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_APP;
	return -1;
    }
    cachep->charno = -1;
    if (eb_zread(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, cachep->text, 
	EB_MAXLEN_ALTERNATION_TEXT + 1) != EB_MAXLEN_ALTERNATION_TEXT + 1) {
	eb_error = EB_ERR_FAIL_READ_APP;
	return -1;
    }

    /*
     * Update cache data.
     */
    memcpy(text, cachep->text, EB_MAXLEN_ALTERNATION_TEXT + 1);
    cachep->text[EB_MAXLEN_ALTERNATION_TEXT] = '\0';
    cachep->charno = ch;

    return 0;
}


/*
 * Get the alternation text of the character number `ch'.
 */
static int
eb_wide_character_text_latin(appendix, ch, text)
    EB_Appendix *appendix;
    int ch;
    char *text;
{
    int start = appendix->sub_current->wide_start;
    int end = appendix->sub_current->wide_end;
    int location;
    EB_Alternation_Cache *cachep;

    /*
     * Check for `ch'.  Is it in a font?
     * This test works correctly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (ch < start || end < ch || (ch & 0xff) < 0x01 || 0xfe < (ch & 0xff)) {
	eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	return -1;
    }

    /*
     * Calculate the location of alternation data.
     */
    location = (appendix->sub_current->wide_page - 1) * EB_SIZE_PAGE
	+ (((ch >> 8) - (start >> 8)) * 0xfe + (ch & 0xff) - (start & 0xff))
	* (EB_MAXLEN_ALTERNATION_TEXT + 1);

    /*
     * Check for the cache data.
     */
    cachep = appendix->wide_cache + EB_HASH_ALT_CACHE(ch);
    if (cachep->charno == ch) {
	memcpy(text, cachep->text, EB_MAXLEN_ALTERNATION_TEXT + 1);
	return 0;
    }

    /*
     * Read the alternation data.
     */
    if (eb_zlseek(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, location, SEEK_SET) < 0) {
	eb_error = EB_ERR_FAIL_SEEK_APP;
	return -1;
    }
    cachep->charno = -1;
    if (eb_zread(&(appendix->sub_current->zip), 
	appendix->sub_current->sub_file, cachep->text, 
	EB_MAXLEN_ALTERNATION_TEXT + 1) != EB_MAXLEN_ALTERNATION_TEXT + 1) {
	eb_error = EB_ERR_FAIL_READ_APP;
	return -1;
    }

    /*
     * Update cache data.
     */
    memcpy(text, cachep->text, EB_MAXLEN_ALTERNATION_TEXT + 1);
    cachep->text[EB_MAXLEN_ALTERNATION_TEXT] = '\0';
    cachep->charno = ch;

    return 0;
}


/*
 * Return next `n'th character number from `ch'.
 */
int
eb_forward_wide_alt_character(appendix, ch, n)
    EB_Appendix *appendix;
    int ch;
    int n;
{
    int start;
    int end;
    int i;

    if (n < 0)
	return eb_backward_wide_alt_character(appendix, ch, -n);

    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return -1;
    }

    /*
     * The wide font must be exist in the current subbook.
     */
    if (appendix->sub_current->wide_page == 0) {
	eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	return -1;
    }

    start = appendix->sub_current->wide_start;
    end = appendix->sub_current->wide_end;

    if (appendix->sub_current->char_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `ch'. (ISO 8859 1)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x01
	    || 0xfe < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	    return -1;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if (0xfe <= (ch & 0xff))
		ch += 3;
	    else
		ch++;
	    if (end < ch) {
		eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
		return -1;
	    }
	}
    } else {
	/*
	 * Check for `ch'. (JIS X 0208)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x21
	    || 0x7e < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	    return -1;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if (0x7e <= (ch & 0xff))
		ch += 0xa3;
	    else
		ch++;
	    if (end < ch) {
		eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
		return -1;
	    }
	}
    }

    return ch;
}


/*
 * Return previous `n'th character number from `ch'.
 */
int
eb_backward_wide_alt_character(appendix, ch, n)
    EB_Appendix *appendix;
    int ch;
    int n;
{
    int start;
    int end;
    int i;

    if (n < 0)
	return eb_forward_wide_alt_character(appendix, ch, -n);

    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return -1;
    }

    /*
     * The wide font must be exist in the current subbook.
     */
    if (appendix->sub_current->wide_page == 0) {
	eb_error = EB_ERR_NO_CUR_FONT;
	return -1;
    }

    start = appendix->sub_current->wide_start;
    end = appendix->sub_current->wide_end;

    if (appendix->sub_current->char_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `ch'. (ISO 8859 1)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x01
	    || 0xfe < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	    return -1;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if ((ch & 0xff) <= 0x01)
		ch -= 3;
	    else
		ch--;
	    if (ch < start) {
		eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
		return -1;
	    }
	}
    } else {
	/*
	 * Check for `ch'. (JIS X 0208)
	 */
	if (ch < start || end < ch || (ch & 0xff) < 0x21
	    || 0x7e < (ch & 0xff)) {
	    eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
	    return -1;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if ((ch & 0xff) <= 0x21)
		ch -= 0xa3;
	    else
		ch--;
	    if (ch < start) {
		eb_error = EB_ERR_NO_SUCH_CHAR_TEXT;
		return -1;
	    }
	}
    }

    return ch;
}


