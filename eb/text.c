/*
 * Copyright (c) 1997, 98, 99, 2000  Motoyuki Kasahara
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

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

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

#ifndef ENABLE_PTHREAD
#define pthread_mutex_lock(m)
#define pthread_mutex_unlock(m)
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "text.h"

/*
 * The maximum number of arguments for an escape sequence.
 */
#define EB_MAX_ARGV 	4

/*
 * Read next when the length of cached data is shorter than this value.
 */
#define SIZE_FEW_REST	16

/*
 * Special skip-code that represents `no skip-code is set'.
 */
#define SKIP_CODE_NONE  -1

/*
 * Cache data buffer.
 */
static char cache_buffer[EB_SIZE_PAGE];

/*
 * Book code of which `cache_buffer' records data.
 */
static EB_Book_Code cache_book_code = EB_BOOK_NONE;

/*
 * Location of cache data loaded in `cache_buffer'.
 */
static off_t cache_location;

/*
 * Length of cache data loaded in `cache_buffer'.
 */
static size_t cache_length;

/*
 * Null hook.
 */
static const EB_Hook null_hook = {EB_HOOK_NULL, NULL};

/*
 * Mutex for cache variables.
 */
#ifdef ENABLE_PTHREAD
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Unexported functions.
 */
static EB_Error_Code eb_read_text_internal EB_P((EB_Book *, EB_Appendix *,
    const EB_Hookset *, size_t, char *, ssize_t *));

/*
 * Initialize text processing status.
 */
void
eb_initialize_text(book)
    EB_Book *book;
{
    pthread_mutex_lock(&cache_mutex);

    if (book->code == cache_book_code)
	cache_book_code = EB_BOOK_NONE;
    book->text_context.code = EB_TEXT_NONE;

    pthread_mutex_unlock(&cache_mutex);
}


/*
 * Reposition the offset of the subbook file.
 */
EB_Error_Code
eb_seek_text(book, position)
    EB_Book *book;
    const EB_Position *position;
{
    EB_Error_Code error_code;

    /*
     * Lock cache data and the book.
     */
    pthread_mutex_lock(&cache_mutex);
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }
    if (book->subbook_current->text_file < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    /*
     * Initialize text-context variables.
     */
    book->text_context.location = (position->page - 1) * EB_SIZE_PAGE
	+ position->offset;
    book->text_context.code = EB_TEXT_NONE;
    book->text_context.work_length = 0;
    book->text_context.work_step = 0;
    book->text_context.narrow_flag = 0;
    book->text_context.printable_count = 0;
    book->text_context.file_end_flag = 0;
    book->text_context.text_end_flag = 0;
    book->text_context.skip_code = SKIP_CODE_NONE;
    book->text_context.auto_stop_code = -1;

    /*
     * Unlock cache data and the book.
     */
    eb_unlock(&book->lock);
    pthread_mutex_unlock(&cache_mutex);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&book->lock);
    pthread_mutex_unlock(&cache_mutex);
    return error_code;
}


/*
 * Get the current text position of the subbook file.
 */
EB_Error_Code
eb_tell_text(book, position)
    EB_Book *book;
    EB_Position *position;
{
    EB_Error_Code error_code;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }
    if (book->subbook_current->text_file < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    position->page = book->text_context.location / EB_SIZE_PAGE + 1;
    position->offset = book->text_context.location % EB_SIZE_PAGE;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Get text in the current subbook in `book'.
 */
EB_Error_Code
eb_read_text(book, appendix, hookset, text_max_length, text, text_length)
    EB_Book *book;
    EB_Appendix *appendix;
    const EB_Hookset *hookset;
    size_t text_max_length;
    char *text;
    ssize_t *text_length;
{
    EB_Error_Code error_code;
    const EB_Hook *hook;

    /*
     * Lock the book, appendix and hookset.
     */
    eb_lock(&book->lock);
    if (appendix != NULL)
	eb_lock(&appendix->lock);
    if (hookset != NULL)
	eb_lock(&hookset->lock);

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }
    if (book->subbook_current->text_file < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    /*
     * Use `eb_default_hookset' when `hookset' is `NULL'.
     */
    if (hookset == NULL)
	hookset = &eb_default_hookset;

    /*
     * Set text mode to `text'.
     */
    if (book->text_context.code != EB_TEXT_NONE) {
	if (book->text_context.code != EB_TEXT_TEXT) {
	    error_code = EB_ERR_DIFF_CONTENT;
	    goto failed;
	}
    } else {
	book->text_context.code = EB_TEXT_TEXT;

	/*
	 * Call the function bound to `EB_HOOK_INITIALIZE'.
	 */
	hook = hookset->hooks + EB_HOOK_INITIALIZE;
	if (hook->function != NULL
	    && hook->function(book, appendix, NULL, EB_HOOK_INITIALIZE, 0,
		NULL) < 0) {
	    error_code = EB_ERR_HOOK_WORKSPACE;
	    goto failed;
	}
    }

    error_code = eb_read_text_internal(book, appendix, hookset,
	text_max_length, text, text_length);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Unlock the book, appendix and hookset.
     */
    if (hookset != &eb_default_hookset)
	eb_unlock(&hookset->lock);
    if (appendix != NULL)
	eb_unlock(&appendix->lock);
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *text = '\0';
    *text_length = -1;
    book->text_context.code = EB_TEXT_INVALID;
    if (hookset != &eb_default_hookset)
	eb_unlock(&hookset->lock);
    if (appendix != NULL)
	eb_unlock(&appendix->lock);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Get text in the current subbook in `book'.
 */
EB_Error_Code
eb_read_heading(book, appendix, hookset, text_max_length, text, text_length)
    EB_Book *book;
    EB_Appendix *appendix;
    const EB_Hookset *hookset;
    size_t text_max_length;
    char *text;
    ssize_t *text_length;
{
    EB_Error_Code error_code;
    const EB_Hook *hook;

    /*
     * Lock the book, appendix and hookset.
     */
    eb_lock(&book->lock);
    if (appendix != NULL)
	eb_lock(&appendix->lock);
    if (hookset != NULL)
	eb_lock(&hookset->lock);

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }
    if (book->subbook_current->text_file < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    /*
     * Use `eb_default_hookset' when `hookset' is `NULL'.
     */
    if (hookset == NULL)
	hookset = &eb_default_hookset;

    /*
     * Set text mode to `heading'.
     */
    if (book->text_context.code != EB_TEXT_NONE) {
	if (book->text_context.code != EB_TEXT_HEADING) {
	    error_code = EB_ERR_DIFF_CONTENT;
	    goto failed;
	}
    } else {
	book->text_context.code = EB_TEXT_HEADING;

	/*
	 * Call the function bound to `EB_HOOK_INITIALIZE'.
	 */
	hook = hookset->hooks + EB_HOOK_INITIALIZE;
	if (hook->function != NULL
	    && hook->function(book, appendix, NULL, EB_HOOK_INITIALIZE, 0,
		NULL) < 0) {
	    error_code = EB_ERR_HOOK_WORKSPACE;
	    goto failed;
	}
    }

    error_code = eb_read_text_internal(book, appendix, hookset,
	text_max_length, text, text_length);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Unlock the book, appendix and hookset.
     */
    if (hookset != &eb_default_hookset)
	eb_unlock(&hookset->lock);
    if (appendix != NULL)
	eb_unlock(&appendix->lock);
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *text = '\0';
    *text_length = -1;
    book->text_context.code = EB_TEXT_INVALID;
    if (hookset != &eb_default_hookset)
	eb_unlock(&hookset->lock);
    if (appendix != NULL)
	eb_unlock(&appendix->lock);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Read data from the subbook file directly.
 */
EB_Error_Code
eb_read_rawtext(book, text_max_length, text, text_length)
    EB_Book *book;
    size_t text_max_length;
    char *text;
    ssize_t *text_length;
{
    EB_Error_Code error_code;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }
    if (book->subbook_current->text_file < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    /*
     * Set text mode to `rawtext'.
     */
    if (book->text_context.code != EB_TEXT_NONE) {
	if (book->text_context.code != EB_TEXT_RAWTEXT) {
	    error_code = EB_ERR_DIFF_CONTENT;
	    goto failed;
	}
    } else {
	book->text_context.code = EB_TEXT_RAWTEXT;
    }

    /*
     * Seek START file and read data.
     */
    if (eb_zlseek(&book->subbook_current->text_zip, 
	book->subbook_current->text_file, book->text_context.location,
	SEEK_SET) == -1) {
	error_code = EB_ERR_FAIL_SEEK_TEXT;
	goto failed;
    }
    *text_length = eb_zread(&book->subbook_current->text_zip,
	book->subbook_current->text_file, text, text_max_length);
    book->text_context.location += *text_length;
    if (*text_length < 0) {
	error_code = EB_ERR_FAIL_READ_TEXT;
	goto failed;
    }

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *text_length = -1;
    book->text_context.code = EB_TEXT_INVALID;
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Get text or heading.
 */
static EB_Error_Code
eb_read_text_internal(book, appendix, hookset, text_max_length, text,
    text_length)
    EB_Book *book;
    EB_Appendix *appendix;
    const EB_Hookset *hookset;
    size_t text_max_length;
    char *text;
    ssize_t *text_length;
{
    EB_Error_Code error_code;
    char *text_p = text;
    unsigned char c;
    size_t text_rest_length = text_max_length;
    char *cache_p;
    const EB_Hook *hook;
    size_t cache_rest_length;
    int argv[EB_MAX_ARGV];
    int argc;

    /*
     * Lock cache data.
     */
    pthread_mutex_lock(&cache_mutex);

    /*
     * If unprocessed data are rest in `book->text_context.work_buffer',
     * copy them to `cache_p'.
     */
    if (0 < book->text_context.work_length) {
	if (text_rest_length < book->text_context.work_length)
	    goto succeeded;
	strcpy(text_p, book->text_context.work_buffer);

	cache_p += book->text_context.work_step;
	cache_rest_length -= book->text_context.work_step;
	book->text_context.location += book->text_context.work_step;

	text_p += book->text_context.work_length;
	text_rest_length -= book->text_context.work_length;

	book->text_context.work_length = 0;
	book->text_context.work_step = 0;
    }

    /*
     * Do nothing if the text-end flag has been set.
     */
    if (book->text_context.text_end_flag)
	goto succeeded;

    /*
     * Check for cache data.
     * If cache data is not what we need, discard it.
     */
    if (book->code == cache_book_code
	&& cache_location <= book->text_context.location
	&& book->text_context.location < cache_location + cache_length) {
	cache_p = cache_buffer
	    + (book->text_context.location - cache_location);
	cache_rest_length = cache_length
	    - (book->text_context.location - cache_location);
    } else {
	cache_book_code = EB_BOOK_NONE;
	cache_p = cache_buffer;
	cache_length = 0;
	cache_rest_length = 0;
    }

    for (;;) {
	book->text_context.work_step = 0;
	*book->text_context.work_buffer = '\0';
	argc = 1;

	/*
	 * If it reaches to the near of the end of the cache buffer,
	 * then moves remaind cache text to the beginning of the cache
	 * buffer, and reads next from a file.
	 */
	if (cache_rest_length < SIZE_FEW_REST
	    && !book->text_context.file_end_flag) {
	    ssize_t read_result;

	    if (0 < cache_rest_length)
		memmove(cache_buffer, cache_p, cache_rest_length);
	    if (eb_zlseek(&book->subbook_current->text_zip, 
		book->subbook_current->text_file, book->text_context.location,
		SEEK_SET) == -1) {
		error_code = EB_ERR_FAIL_SEEK_TEXT;
		goto failed;
	    }

	    read_result = eb_zread(&book->subbook_current->text_zip, 
		book->subbook_current->text_file,
		cache_buffer + cache_rest_length,
		EB_SIZE_PAGE - cache_rest_length);
	    if (read_result < 0) {
		error_code = EB_ERR_FAIL_READ_TEXT;
		goto failed;
	    } else if (read_result != EB_SIZE_PAGE - cache_rest_length)
		book->text_context.file_end_flag = 1;

	    cache_book_code = book->code;
	    cache_location = book->text_context.location - cache_rest_length;
	    cache_length = cache_rest_length + read_result;
	    cache_p = cache_buffer;
	    cache_rest_length = cache_length;
	}

	/*
	 * Get 1 byte from the buffer.
	 */
	if (cache_rest_length < 1) {
	    error_code = EB_ERR_UNEXP_TEXT;
	    goto failed;
	}
	c = eb_uint1(cache_p);

	if (c == 0x1f) {
	    hook = &null_hook;

	    /*
	     * This is escape sequences.
	     */
	    if (cache_rest_length < 2) {
		error_code = EB_ERR_UNEXP_TEXT;
		goto failed;
	    }
	    argv[0] = eb_uint2(cache_p);

	    switch (eb_uint1(cache_p + 1)) {
	    case 0x02:
		/* beginning of text */
		book->text_context.work_step = 2;
		break;

	    case 0x03:
		/* end of text (don't set `work_step') */
		book->text_context.text_end_flag = 1;
		goto succeeded;

	    case 0x04:
		/* beginning of NARROW */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_NARROW;
		book->text_context.narrow_flag = 1;
		break;

	    case 0x05:
		/* end of NARROW */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_END_NARROW;
		book->text_context.narrow_flag = 0;
		break;

	    case 0x06:
		/* beginning of subscript */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_SUBSCRIPT;
		break;

	    case 0x07:
		/* end of subscript */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_END_SUBSCRIPT;
		break;

	    case 0x09:
		/* set indent */
		book->text_context.work_step = 4;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_STOP_CODE;
		argc = 2;
		argv[1] = eb_uint2(cache_p + 2);
		if (0 < book->text_context.printable_count
		    && book->text_context.code == EB_TEXT_TEXT
		    && hook->function != NULL
		    && hook->function(book, appendix,
			book->text_context.work_buffer, EB_HOOK_STOP_CODE,
			argc, argv) < 0) {
		    book->text_context.text_end_flag = 1;
		    goto succeeded;
		}

		*(book->text_context.work_buffer) = '\0';
		hook = hookset->hooks + EB_HOOK_SET_INDENT;
		argc = 2;
		argv[1] = eb_uint2(cache_p + 2);
		break;

	    case 0x0a:
		/* newline */
		book->text_context.work_step = 2;
		if (book->text_context.code == EB_TEXT_HEADING) {
		    book->text_context.text_end_flag = 1;
		    book->text_context.location
			+= book->text_context.work_step;
		    goto succeeded;
		}
		if (book->text_context.skip_code == SKIP_CODE_NONE) {
		    *book->text_context.work_buffer = '\n';
		    *(book->text_context.work_buffer + 1) = '\0';
		}
		hook = hookset->hooks + EB_HOOK_NEWLINE;
		break;

	    case 0x0e:
		/* beginning of superscript */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_SUPERSCRIPT;
		break;

	    case 0x0f:
		/* end of superscript */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_END_SUPERSCRIPT;
		break;

	    case 0x10:
		/* beginning of newline prohibition */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_NO_NEWLINE;
		break;

	    case 0x11:
		/* end of newline prohibition */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_END_NO_NEWLINE;
		break;

	    case 0x12:
		/* beginning of emphasis */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_EMPHASIS;
		break;

	    case 0x13:
		/* end of emphasis */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_END_EMPHASIS;
		break;

	    case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
	    case 0xe0:
		/* emphasis; described in JIS X 4081-1996 */
		book->text_context.work_step = 4;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		/* Some old EB books don't take an argument. */
		if (book->disc_code != EB_DISC_EPWING
		    && eb_uint1(cache_p + 2) >= 0x1f)
		    book->text_context.work_step = 2;
		break;

	    case 0x32:
		/* beginning of an anchor of the picture */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_PICTURE;
		break;

	    case 0x33:
		/* beginning of an anchor of the sound */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_PICTURE;
		break;

	    case 0x35: case 0x36: case 0x37: case 0x38: case 0x39: case 0x3a:
	    case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		book->text_context.work_step = 2;
		book->text_context.skip_code = eb_uint1(cache_p + 1) + 0x20;
		break;

	    case 0x41:
		/* beginning of the keyword */
		book->text_context.work_step = 4;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_STOP_CODE;
		argc = 2;
		argv[1] = eb_uint2(cache_p + 2);
		if (0 < book->text_context.printable_count
		    && book->text_context.code == EB_TEXT_TEXT
		    && hook->function != NULL
		    && hook->function(book, appendix,
			book->text_context.work_buffer, EB_HOOK_STOP_CODE,
			argc, argv) < 0) {
		    book->text_context.text_end_flag = 1;
		    goto succeeded;
		}
		if (book->text_context.auto_stop_code < 0)
		    book->text_context.auto_stop_code = eb_uint2(cache_p + 2);

		*(book->text_context.work_buffer) = '\0';
		hook = hookset->hooks + EB_HOOK_BEGIN_KEYWORD;
		argc = 2;
		argv[1] = eb_uint2(cache_p + 2);
		break;

	    case 0x42:
		/* beginning of the reference */
		book->text_context.work_step = 4;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		if (eb_uint1(cache_p + 2) != 0x00)
		    book->text_context.work_step -= 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_REFERENCE;
		break;

	    case 0x43:
		/* beginning of an entry of the menu */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_MENU;
		break;

	    case 0x44:
		/* beginning of an anchor of the picture (old style?) */
		book->text_context.work_step = 12;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_BEGIN_PICTURE;
		break;

	    case 0x45:
		/* prefix of sound or picuture */
		book->text_context.work_step = 4;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		if (eb_uint1(cache_p + 2) != 0x1f) {
		    argc = 2;
		    argv[1] = eb_bcd4(cache_p + 2);
		}
		break;

	    case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e:
	    case 0x4f:
		book->text_context.work_step = 2;
		book->text_context.skip_code = eb_uint1(cache_p + 1) + 0x20;
		break;

	    case 0x52:
		/* end of the picture */
		book->text_context.work_step = 8;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_PICTURE;
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		break;

	    case 0x53:
		/* end of the sound */
		book->text_context.work_step = 10;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_SOUND;
		break;

	    case 0x61:
		/* end of the keyword */
		book->text_context.work_step = 2;
		hook = hookset->hooks + EB_HOOK_END_KEYWORD;
		break;

	    case 0x62:
		/* end of the reference */
		book->text_context.work_step = 8;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_REFERENCE;
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		break;

	    case 0x63:
		/* end of an entry of the menu */
		book->text_context.work_step = 8;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_MENU;
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		break;

	    case 0x64:
		/* end of the picture (old style?) */
		book->text_context.work_step = 8;
		if (cache_rest_length < book->text_context.work_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_PICTURE;
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		break;

	    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
	    case 0x76: case 0x77: case 0x78: case 0x79: case 0x7a: case 0x7b:
	    case 0x7c: case 0x7d: case 0x7e: case 0x7f:
	    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
	    case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b:
	    case 0x8c: case 0x8d: case 0x8e: case 0x8f:
		book->text_context.work_step = 2;
		book->text_context.skip_code = eb_uint1(cache_p + 1) + 0x20;
		break;

	    case 0xe1:
		book->text_context.work_step = 2;
		break;

	    case 0xe4: case 0xe6: case 0xe8: case 0xea: case 0xec: case 0xee:
	    case 0xf0: case 0xf2: case 0xf4: case 0xf6: case 0xf8: case 0xfa:
	    case 0xfc: case 0xfe:
		book->text_context.work_step = 2;
		book->text_context.skip_code = eb_uint1(cache_p + 1) + 0x01;
		break;

	    default:
		book->text_context.work_step = 2;
		if (book->text_context.skip_code == eb_uint1(cache_p + 1))
		    book->text_context.skip_code = SKIP_CODE_NONE;
		break;
	    }

	    if (book->text_context.skip_code == SKIP_CODE_NONE
		&& hook->function != NULL
		&& hook->function(book, appendix,
		    book->text_context.work_buffer, hook->code, argc, argv)
		< 0) {
		error_code = EB_ERR_HOOK_WORKSPACE;
		goto failed;
	    }

	} else if (book->character_code == EB_CHARCODE_ISO8859_1) {
	    /*
	     * The book is mainly written in ISO 8859 1.
	     */
	    book->text_context.printable_count++;

	    if ((0x20 <= c && c < 0x7f) || (0xa0 <= c && c < 0xff)) {
		/*
		 * This is an ISO 8859 1 character.
		 */
		book->text_context.work_step = 1;
		argv[0] = eb_uint1(cache_p);
		if (book->text_context.skip_code == SKIP_CODE_NONE) {
		    *book->text_context.work_buffer = c;
		    *(book->text_context.work_buffer + 1) = '\0';
		    hook = hookset->hooks + EB_HOOK_ISO8859_1;
		    if (hook->function != NULL
			&& hook->function(book, appendix,
			    book->text_context.work_buffer, EB_HOOK_ISO8859_1,
			    argc, argv) < 0) {
			error_code = EB_ERR_HOOK_WORKSPACE;
			goto failed;
		    }
		}
	    } else {
		/*
		 * This is a local character.
		 */
		if (cache_rest_length < 2) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}

		book->text_context.work_step = 2;
		argv[0] = eb_uint2(cache_p);
		if (book->text_context.skip_code == SKIP_CODE_NONE) {
		    *book->text_context.work_buffer = (unsigned char) c;
		    *(book->text_context.work_buffer + 1)
			= (unsigned char) eb_uint1(cache_p + 1);
		    *(book->text_context.work_buffer + 2) = '\0';
		    hook = hookset->hooks + EB_HOOK_NARROW_FONT;
		    if (hook->function != NULL
			&& hook->function(book, appendix,
			    book->text_context.work_buffer,
			    EB_HOOK_NARROW_FONT, argc, argv) < 0) {
			error_code = EB_ERR_HOOK_WORKSPACE;
			goto failed;
		    }
		}
	    }

	} else {
	    /*
	     * The book is written in JIS X 0208 or JIS X 0208 + GB 2312.
	     */
	    int c2;

	    book->text_context.printable_count++;
	    book->text_context.work_step = 2;
	    argv[0] = eb_uint2(cache_p);

	    if (cache_rest_length < 2) {
		error_code = EB_ERR_UNEXP_TEXT;
		goto failed;
	    }

	    c2 = eb_uint1(cache_p + 1);

	    if (book->text_context.skip_code != SKIP_CODE_NONE) {
		/* nothing to be done. */
	    } else if (0x20 < c && c < 0x7f && 0x20 < c2 && c2 < 0x7f) {
		/*
		 * This is a JIS X 0208 KANJI character.
		 */
		*book->text_context.work_buffer
		    = (unsigned char) (c | 0x80);
		*(book->text_context.work_buffer + 1)
		    = (unsigned char) (c2 | 0x80);
		*(book->text_context.work_buffer + 2) = '\0';

		if (book->text_context.narrow_flag) {
		    hook = hookset->hooks + EB_HOOK_NARROW_JISX0208;
		    if (hook->function != NULL
			&& hook->function(book, appendix,
			    book->text_context.work_buffer,
			    EB_HOOK_NARROW_JISX0208, 0, argv) < 0) {
			error_code = EB_ERR_HOOK_WORKSPACE;
			goto failed;
		    }
		} else {
		    hook = hookset->hooks + EB_HOOK_WIDE_JISX0208;
		    if (hook->function != NULL
			&& hook->function(book, appendix,
			    book->text_context.work_buffer,
			    EB_HOOK_WIDE_JISX0208, argc, argv) < 0) {
			error_code = EB_ERR_HOOK_WORKSPACE;
			goto failed;
		    }
		}
	    } else if (0x20 < c && c < 0x7f && 0xa0 < c2 && c2 < 0xff) {
		/*
		 * This is a GB 2312 HANJI character.
		 */
		*book->text_context.work_buffer = (unsigned char) (c | 0x80);
		*(book->text_context.work_buffer + 1) = (unsigned char) c2;
		*(book->text_context.work_buffer + 2) = '\0';

		hook = hookset->hooks + EB_HOOK_GB2312;
		if (hook->function != NULL
		    && hook->function(book, appendix,
			book->text_context.work_buffer, EB_HOOK_GB2312, 0,
			argv) < 0) {
		    error_code = EB_ERR_HOOK_WORKSPACE;
		    goto failed;
		}
	    } else {
		/*
		 * This is a local character.
		 */
		*book->text_context.work_buffer = (unsigned char) c;
		*(book->text_context.work_buffer + 1) = (unsigned char) c2;
		*(book->text_context.work_buffer + 2) = '\0';

		if (book->text_context.narrow_flag) {
		    hook = hookset->hooks + EB_HOOK_NARROW_FONT;
		    if (hook->function != NULL
			&& hook->function(book, appendix,
			    book->text_context.work_buffer,
			    EB_HOOK_NARROW_FONT, argc, argv) < 0) {
			error_code = EB_ERR_HOOK_WORKSPACE;
			goto failed;
		    }
		} else {
		    hook = hookset->hooks + EB_HOOK_WIDE_FONT;
		    if (hook->function != NULL
			&& hook->function(book, appendix,
			    book->text_context.work_buffer, EB_HOOK_WIDE_FONT,
			    argc, argv) < 0) {
			error_code = EB_ERR_HOOK_WORKSPACE;
			goto failed;
		    }
		}
	    }
	}

	/*
	 * Add a string in the work buffer to the text buffer.
	 * If enough space is not left on the text buffer, it returns
	 * immediately.
	 */
	book->text_context.work_length
	    = strlen(book->text_context.work_buffer);
	if (text_rest_length < book->text_context.work_length)
	    break;
	strcpy(text_p, book->text_context.work_buffer);

	/*
	 * Update variables.
	 */
	cache_p += book->text_context.work_step;
	cache_rest_length -= book->text_context.work_step;
	book->text_context.location += book->text_context.work_step;

	text_p += book->text_context.work_length;
	text_rest_length -= book->text_context.work_length;

	book->text_context.work_length = 0;
	book->text_context.work_step = 0;
    }

  succeeded:
    *text_p = '\0';
    *text_length = (text_p - text);

    /*
     * Unlock cache data.
     */
    pthread_mutex_unlock(&cache_mutex);

    return EB_SUCCESS;

    /*
     * An error occurs...
     * Discard cache if read error occurs.
     */
  failed:
    if (error_code == EB_ERR_FAIL_READ_TEXT)
	cache_book_code = EB_BOOK_NONE;
    *text = '\0';
    *text_length = -1;
    book->text_context.code = EB_TEXT_INVALID;
    pthread_mutex_unlock(&cache_mutex);
    return error_code;
}


/*
 * Forward text position to the next paragraph.
 */
EB_Error_Code
eb_forward_text(book, hookset)
    EB_Book *book;
    const EB_Hookset *hookset;
{
    EB_Error_Code error_code;
    char text[EB_SIZE_PAGE];
    ssize_t text_length;

    /*
     * Lock the book and hookset.
     */
    eb_lock(&book->lock);
    if (hookset != NULL)
	eb_lock(&hookset->lock);

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }
    if (book->subbook_current->text_file < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    /*
     * Clear work buffer in the text context.
     */
    book->text_context.work_length = 0;

    /*
     * Use `eb_default_hookset' when `hookset' is `NULL'.
     */
    if (hookset == NULL)
	hookset = &eb_default_hookset;

    if (book->text_context.code != EB_TEXT_NONE) {
	if (book->text_context.code != EB_TEXT_TEXT) {
	    error_code = EB_ERR_DIFF_CONTENT;
	    goto failed;
	}
    } else {
	book->text_context.code = EB_TEXT_TEXT;
    }

    /*
     * if the text-end flag has been set, we simply unset the flag
     * and returns immediately.
     */
    if (book->text_context.text_end_flag) {
	book->text_context.text_end_flag = 0;
	goto succeeded;
    }

    for (;;) {
	error_code = eb_read_text_internal(book, NULL, hookset,
	    EB_SIZE_PAGE, text, &text_length);
	if (error_code != EB_SUCCESS)
	    goto failed;
	if (text_length == 0)
	    break;
    }

    /*
     * Disable the text-end flag.
     */
    book->text_context.work_length = 0;
    book->text_context.work_step = 0;
    book->text_context.narrow_flag = 0;
    book->text_context.printable_count = 0;
    book->text_context.text_end_flag = 0;
    book->text_context.skip_code = SKIP_CODE_NONE;

    /*
     * Unlock the book and hookset.
     */
  succeeded:
    if (hookset != &eb_default_hookset)
	eb_unlock(&hookset->lock);
    eb_unlock(&book->lock);
    pthread_mutex_unlock(&cache_mutex);
    return EB_SUCCESS;

    /*
     * An error occurs...
     * Discard cache if read error occurs.
     */
  failed:
    book->text_context.code = EB_TEXT_INVALID;
    if (hookset != &eb_default_hookset)
	eb_unlock(&hookset->lock);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Forward heading position to the next paragraph.
 * (for keyword search.)
 */
EB_Error_Code
eb_forward_heading(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    char text[EB_SIZE_PAGE];
    ssize_t text_length;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * if the text-end flag has been set, we simply unset the flag
     * and returns immediately.
     */
    if (book->text_context.text_end_flag) {
	book->text_context.text_end_flag = 0;
	goto succeeded;
    }

    /*
     * Force setting context mode as heading mode.
     */
    book->text_context.code = EB_TEXT_HEADING;

    /*
     * Go to the beginning of the next heading.
     */
    for (;;) {
	error_code = eb_read_text_internal(book, NULL, &eb_default_hookset,
	    EB_SIZE_PAGE, text, &text_length);
	if (error_code != EB_SUCCESS)
	    goto failed;
	if (text_length == 0)
	    break;
    }

    /*
     * Initialize the context flags.
     */
    book->text_context.work_length = 0;
    book->text_context.work_step = 0;
    book->text_context.narrow_flag = 0;
    book->text_context.printable_count = 0;
    book->text_context.text_end_flag = 0;
    book->text_context.skip_code = SKIP_CODE_NONE;

    /*
     * Unlock cache data.
     */
  succeeded:
    eb_unlock(&book->lock);
    pthread_mutex_unlock(&cache_mutex);
    return EB_SUCCESS;

    /*
     * An error occurs...
     * Discard cache if read error occurs.
     */
  failed:
    book->text_context.code = EB_TEXT_INVALID;
    eb_unlock(&book->lock);
    return error_code;
}


