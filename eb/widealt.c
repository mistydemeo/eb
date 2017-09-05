/*
 * Copyright (c) 1997, 98, 99, 2000, 01  
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
#include "appendix.h"
#include "internal.h"

/*
 * Unexported functions.
 */
static EB_Error_Code eb_wide_character_text_jis EB_P((EB_Appendix *, int,
    char *));
static EB_Error_Code eb_wide_character_text_latin EB_P((EB_Appendix *, int,
    char *));

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
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL)
	goto failed;

    if (appendix->subbook_current->wide_page == 0)
	goto failed;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&appendix->lock);
    return 0;
}


/*
 * Look up the character number of the start of the wide font alternation
 * of the current subbook in `book'.
 */
EB_Error_Code
eb_wide_alt_start(appendix, start)
    EB_Appendix *appendix;
    int *start;
{
    EB_Error_Code error_code;

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    if (appendix->subbook_current->wide_page == 0) {
	error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	goto failed;
    }

    *start = appendix->subbook_current->wide_start;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *start = -1;
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Return the character number of the end of the wide font alternation
 * of the current subbook in `book'.
 */
EB_Error_Code
eb_wide_alt_end(appendix, end)
    EB_Appendix *appendix;
    int *end;
{
    EB_Error_Code error_code;

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    if (appendix->subbook_current->wide_page == 0) {
	error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	goto failed;
    }

    *end = appendix->subbook_current->wide_end;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *end = -1;
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Get the alternation text of the character number `character_number'.
 */
EB_Error_Code
eb_wide_alt_character_text(appendix, character_number, text)
    EB_Appendix *appendix;
    int character_number;
    char *text;
{
    EB_Error_Code error_code;

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    /*
     * The wide font must be exist in the current subbook.
     */
    if (appendix->subbook_current->wide_page == 0) {
	error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	goto failed;
    }

    if (appendix->subbook_current->character_code == EB_CHARCODE_ISO8859_1) {
	error_code = eb_wide_character_text_latin(appendix,
	    character_number, text);
    } else {
	error_code = eb_wide_character_text_jis(appendix, character_number,
	    text);
    }
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *text = '\0';
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Get the alternation text of the character number `character_number'.
 */
static EB_Error_Code
eb_wide_character_text_jis(appendix, character_number, text)
    EB_Appendix *appendix;
    int character_number;
    char *text;
{
    EB_Error_Code error_code;
    int start;
    int end;
    off_t location;
    EB_Alternation_Cache *cachep;

    start = appendix->subbook_current->wide_start;
    end = appendix->subbook_current->wide_end;

    /*
     * Check for `character_number'.  Is it in a font?
     * This test works correctly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (character_number < start
	|| end < character_number
	|| (character_number & 0xff) < 0x21
	|| 0x7e < (character_number & 0xff)) {
	error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	goto failed;
    }

    /*
     * Calculate the location of alternation data.
     */
    location
	= (off_t)(appendix->subbook_current->wide_page - 1) * EB_SIZE_PAGE
	+ (((character_number >> 8) - (start >> 8)) * 0x5e
	    + (character_number & 0xff) - (start & 0xff))
	* (EB_MAX_ALTERNATION_TEXT_LENGTH + 1);

    /*
     * Check for the cache data.
     */
    cachep = appendix->wide_cache + EB_HASH_ALT_CACHE(character_number);
    if (cachep->character_number == character_number) {
	memcpy(text, cachep->text, EB_MAX_ALTERNATION_TEXT_LENGTH + 1);
	goto succeeded;
    }

    /*
     * Read the alternation data.
     */
    if (zio_lseek(&appendix->subbook_current->zio, location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_APP;
	goto failed;
    }
    cachep->character_number = -1;
    if (zio_read(&appendix->subbook_current->zio, cachep->text, 
	EB_MAX_ALTERNATION_TEXT_LENGTH + 1)
	!= EB_MAX_ALTERNATION_TEXT_LENGTH + 1) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }

    /*
     * Update cache data.
     */
    memcpy(text, cachep->text, EB_MAX_ALTERNATION_TEXT_LENGTH + 1);
    cachep->text[EB_MAX_ALTERNATION_TEXT_LENGTH] = '\0';
    cachep->character_number = character_number;

  succeeded:
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *text = '\0';
    return error_code;
}


/*
 * Get the alternation text of the character number `character_number'.
 */
static EB_Error_Code
eb_wide_character_text_latin(appendix, character_number, text)
    EB_Appendix *appendix;
    int character_number;
    char *text;
{
    EB_Error_Code error_code;
    int start;
    int end;
    off_t location;
    EB_Alternation_Cache *cache_p;

    start = appendix->subbook_current->wide_start;
    end = appendix->subbook_current->wide_end;

    /*
     * Check for `character_number'.  Is it in a font?
     * This test works correctly even when the font doesn't exist in
     * the current subbook because `start' and `end' have set to -1
     * in the case.
     */
    if (character_number < start
	|| end < character_number
	|| (character_number & 0xff) < 0x01
	|| 0xfe < (character_number & 0xff)) {
	error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	goto failed;
    }

    /*
     * Calculate the location of alternation data.
     */
    location
	= (off_t)(appendix->subbook_current->wide_page - 1) * EB_SIZE_PAGE
	+ (((character_number >> 8) - (start >> 8)) * 0xfe
	    + (character_number & 0xff) - (start & 0xff))
	* (EB_MAX_ALTERNATION_TEXT_LENGTH + 1);

    /*
     * Check for the cache data.
     */
    cache_p = appendix->wide_cache + EB_HASH_ALT_CACHE(character_number);
    if (cache_p->character_number == character_number) {
	memcpy(text, cache_p->text, EB_MAX_ALTERNATION_TEXT_LENGTH + 1);
	goto succeeded;
    }

    /*
     * Read the alternation data.
     */
    if (zio_lseek(&appendix->subbook_current->zio, location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_APP;
	goto failed;
    }
    cache_p->character_number = -1;
    if (zio_read(&appendix->subbook_current->zio, cache_p->text, 
	EB_MAX_ALTERNATION_TEXT_LENGTH + 1)
	!= EB_MAX_ALTERNATION_TEXT_LENGTH + 1) {
	error_code = EB_ERR_FAIL_READ_APP;
	goto failed;
    }

    /*
     * Update cache data.
     */
    memcpy(text, cache_p->text, EB_MAX_ALTERNATION_TEXT_LENGTH + 1);
    cache_p->text[EB_MAX_ALTERNATION_TEXT_LENGTH] = '\0';
    cache_p->character_number = character_number;

  succeeded:
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *text = '\0';
    return error_code;
}


/*
 * Return next `n'th character number from `*character_number'.
 */
EB_Error_Code
eb_forward_wide_alt_character(appendix, n, character_number)
    EB_Appendix *appendix;
    int n;
    int *character_number;
{
    EB_Error_Code error_code;
    int start;
    int end;
    int i;

    if (n < 0) {
	return eb_backward_wide_alt_character(appendix, -n,
	    character_number);
    }

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    /*
     * The wide font must be exist in the current subbook.
     */
    if (appendix->subbook_current->wide_page == 0) {
	error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	goto failed;
    }

    start = appendix->subbook_current->wide_start;
    end = appendix->subbook_current->wide_end;

    if (appendix->subbook_current->character_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `*character_number'. (ISO 8859 1)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x01
	    || 0xfe < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	    goto failed;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if (0xfe <= (*character_number & 0xff))
		*character_number += 3;
	    else
		*character_number += 1;
	    if (end < *character_number) {
		error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
		goto failed;
	    }
	}
    } else {
	/*
	 * Check for `*character_number'. (JIS X 0208)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x21
	    || 0x7e < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	    goto failed;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if (0x7e <= (*character_number & 0xff))
		*character_number += 0xa3;
	    else
		*character_number += 1;
	    if (end < *character_number) {
		error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
		goto failed;
	    }
	}
    }

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *character_number = -1;
    eb_unlock(&appendix->lock);
    return error_code;
}


/*
 * Return previous `n'th character number from `*character_number'.
 */
EB_Error_Code
eb_backward_wide_alt_character(appendix, n, character_number)
    EB_Appendix *appendix;
    int n;
    int *character_number;
{
    EB_Error_Code error_code;
    int start;
    int end;
    int i;

    if (n < 0) {
	return eb_forward_wide_alt_character(appendix, -n, character_number);
    }

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    /*
     * The wide font must be exist in the current subbook.
     */
    if (appendix->subbook_current->wide_page == 0) {
	error_code = EB_ERR_NO_CUR_FONT;
	goto failed;
    }

    start = appendix->subbook_current->wide_start;
    end = appendix->subbook_current->wide_end;

    if (appendix->subbook_current->character_code == EB_CHARCODE_ISO8859_1) {
	/*
	 * Check for `*character_number'. (ISO 8859 1)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x01
	    || 0xfe < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	    goto failed;
	}

	/*
	 * Get character number. (ISO 8859 1)
	 */
	for (i = 0; i < n; i++) {
	    if ((*character_number & 0xff) <= 0x01)
		*character_number -= 3;
	    else
		*character_number -= 1;
	    if (*character_number < start) {
		error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
		goto failed;
	    }
	}
    } else {
	/*
	 * Check for `*character_number'. (JIS X 0208)
	 */
	if (*character_number < start
	    || end < *character_number
	    || (*character_number & 0xff) < 0x21
	    || 0x7e < (*character_number & 0xff)) {
	    error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
	    goto failed;
	}

	/*
	 * Get character number. (JIS X 0208)
	 */
	for (i = 0; i < n; i++) {
	    if ((*character_number & 0xff) <= 0x21)
		*character_number -= 0xa3;
	    else
		*character_number -= 1;
	    if (*character_number < start) {
		error_code = EB_ERR_NO_SUCH_CHAR_TEXT;
		goto failed;
	    }
	}
    }

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *character_number = -1;
    eb_unlock(&appendix->lock);
    return error_code;
}


