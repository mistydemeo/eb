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
#include "internal.h"
#include "text.h"

/*
 * The maximum number of arguments for an escape sequence.
 */
#define EB_MAX_ARGV 	6

/*
 * Read next when the length of cached data is shorter than this value.
 */
#define SIZE_FEW_REST	48

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
    EB_Hookset *, void *, size_t, char *, ssize_t *));

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
    book->text_context.is_candidate = 0;

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
    if (zio_file(&book->subbook_current->text_zio) < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    if (position->page <= 0
	|| position->offset < 0 || EB_SIZE_PAGE <= position->offset) {
	error_code = EB_ERR_FAIL_SEEK_TEXT;
	goto failed;
    }

    /*
     * Initialize text-context variables.
     */
    book->text_context.location = (off_t)(position->page - 1) * EB_SIZE_PAGE
	+ position->offset;
    book->text_context.code = EB_TEXT_NONE;
    if (book->text_context.unprocessed != NULL) {
	free(book->text_context.unprocessed);
	book->text_context.unprocessed = NULL;
	book->text_context.unprocessed_size = 0;
    }
    book->text_context.in_step = 0;
    book->text_context.narrow_flag = 0;
    book->text_context.printable_count = 0;
    book->text_context.file_end_flag = 0;
    book->text_context.text_end_flag = 0;
    book->text_context.skip_code = SKIP_CODE_NONE;
    book->text_context.auto_stop_code = -1;
    book->text_context.is_candidate = 0;

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
    if (zio_file(&book->subbook_current->text_zio) < 0) {
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
eb_read_text(book, appendix, hookset, container, text_max_length, text,
    text_length)
    EB_Book *book;
    EB_Appendix *appendix;
    EB_Hookset *hookset;
    VOID *container;
    size_t text_max_length;
    char *text;
    ssize_t *text_length;
{
    EB_Error_Code error_code;
    const EB_Hook *hook;
    EB_Position position;

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
    if (zio_file(&book->subbook_current->text_zio) < 0) {
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
	if (book->text_context.code != EB_TEXT_TEXT
	    && book->text_context.code != EB_TEXT_OPTIONAL_TEXT) {
	    error_code = EB_ERR_DIFF_CONTENT;
	    goto failed;
	}
    } else {
	eb_tell_text(book, &position);

	if (book->subbook_current->menu.start_page <= position.page
	    && position.page <= book->subbook_current->menu.end_page)
	    book->text_context.code = EB_TEXT_OPTIONAL_TEXT;
	else if (book->subbook_current->copyright.start_page <= position.page
	    && position.page <= book->subbook_current->copyright.end_page)
	    book->text_context.code = EB_TEXT_OPTIONAL_TEXT;
	else
	    book->text_context.code = EB_TEXT_TEXT;
	    
	if (book->text_context.unprocessed != NULL) {
	    free(book->text_context.unprocessed);
	    book->text_context.unprocessed = NULL;
	    book->text_context.unprocessed_size = 0;
	}
	book->text_context.in_step = 0;
	book->text_context.narrow_flag = 0;
	book->text_context.printable_count = 0;
	book->text_context.file_end_flag = 0;
	book->text_context.text_end_flag = 0;
	book->text_context.skip_code = SKIP_CODE_NONE;
	book->text_context.auto_stop_code = -1;
	book->text_context.is_candidate = 0;

	/*
	 * Call the function bound to `EB_HOOK_INITIALIZE'.
	 */
	hook = hookset->hooks + EB_HOOK_INITIALIZE;
	if (hook->function != NULL) {
	    error_code = hook->function(book, appendix, container,
		EB_HOOK_INITIALIZE, 0, NULL);
	    if (error_code != EB_SUCCESS)
		goto failed;
	}
    }

    error_code = eb_read_text_internal(book, appendix, hookset, container,
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
eb_read_heading(book, appendix, hookset, container, text_max_length, text,
    text_length)
    EB_Book *book;
    EB_Appendix *appendix;
    EB_Hookset *hookset;
    VOID *container;
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
    if (zio_file(&book->subbook_current->text_zio) < 0) {
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
	if (book->text_context.unprocessed != NULL) {
	    free(book->text_context.unprocessed);
	    book->text_context.unprocessed = NULL;
	    book->text_context.unprocessed_size = 0;
	}
	book->text_context.in_step = 0;
	book->text_context.narrow_flag = 0;
	book->text_context.printable_count = 0;
	book->text_context.file_end_flag = 0;
	book->text_context.text_end_flag = 0;
	book->text_context.skip_code = SKIP_CODE_NONE;
	book->text_context.auto_stop_code = -1;
	book->text_context.is_candidate = 0;

	/*
	 * Call the function bound to `EB_HOOK_INITIALIZE'.
	 */
	hook = hookset->hooks + EB_HOOK_INITIALIZE;
	if (hook->function != NULL) {
	    error_code = hook->function(book, appendix, container,
		EB_HOOK_INITIALIZE, 0, NULL);
	    if (error_code != EB_SUCCESS)
		goto failed;
	}
    }

    error_code = eb_read_text_internal(book, appendix, hookset, container,
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
    if (zio_file(&book->subbook_current->text_zio) < 0) {
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
    if (zio_lseek(&book->subbook_current->text_zio, 
	book->text_context.location, SEEK_SET) == -1) {
	error_code = EB_ERR_FAIL_SEEK_TEXT;
	goto failed;
    }
    *text_length = zio_read(&book->subbook_current->text_zio, text,
	text_max_length);
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
eb_read_text_internal(book, appendix, hookset, container, text_max_length,
    text, text_length)
    EB_Book *book;
    EB_Appendix *appendix;
    EB_Hookset *hookset;
    VOID *container;
    size_t text_max_length;
    char *text;
    ssize_t *text_length;
{
    EB_Error_Code error_code;
    EB_Text_Context *context;
    unsigned char c1, c2;
    char *cache_p;
    const EB_Hook *hook;
    unsigned char *candidate_p;
    size_t candidate_length;
    size_t cache_rest_length;
    unsigned int argv[EB_MAX_ARGV];
    int argc;

    /*
     * Lock cache data.
     */
    pthread_mutex_lock(&cache_mutex);

    /*
     * Initialize variables.
     */
    context = &book->text_context;
    context->out = text;
    context->out_rest_length = text_max_length;
    if (context->is_candidate) {
	candidate_length = strlen(context->candidate);
	candidate_p = (unsigned char *)context->candidate + candidate_length;
    } else {
	candidate_length = 0;
	candidate_p = NULL;
    }

    /*
     * Do nothing if the text-end flag has been set.
     */
    if (context->text_end_flag)
	goto succeeded;

    /*
     * Check for cache data.
     * If cache data is not what we need, discard it.
     */
    if (book->code == cache_book_code
	&& cache_location <= context->location
	&& context->location < cache_location + cache_length) {
	cache_p = cache_buffer + (context->location - cache_location);
	cache_rest_length = cache_length
	    - (context->location - cache_location);
    } else {
	cache_book_code = EB_BOOK_NONE;
	cache_p = cache_buffer;
	cache_length = 0;
	cache_rest_length = 0;
    }

    /*
     * If unprocessed string are rest in `context->unprocessed',
     * copy them to `context->out'.
     */
    if (context->unprocessed != NULL) {
	if (context->out_rest_length < context->unprocessed_size)
	    goto succeeded;

	/*
	 * Update cache controll variables.
	 * We have to discard cache data if text corresponding to
	 * `context->unprocessed' is not remained in the cache
	 * buffer.
	 */
	if (context->unprocessed_size <= cache_rest_length) {
	    cache_p += context->in_step;
	    cache_rest_length -= context->in_step;
	} else {
	    cache_book_code = EB_BOOK_NONE;
	    cache_p = cache_buffer;
	    cache_length = 0;
	    cache_rest_length = 0;
	}

	/*
	 * Append the unprocessed string to `text'.
	 */
	memcpy(context->out, context->unprocessed, context->unprocessed_size);
	context->out += context->unprocessed_size;
	context->out_rest_length -= context->unprocessed_size;
	context->location += context->in_step;
	free(context->unprocessed);
	context->unprocessed = NULL;
	context->unprocessed_size = 0;
	context->in_step = 0;
    }

    for (;;) {
	context->in_step = 0;
	context->out_step = 0;
	argc = 1;

	/*
	 * If it reaches to the near of the end of the cache buffer,
	 * then moves remaind cache text to the beginning of the cache
	 * buffer, and reads a next chunk from a file.
	 */
	if (cache_rest_length < SIZE_FEW_REST && !context->file_end_flag) {
	    ssize_t read_result;

	    if (0 < cache_rest_length)
		memmove(cache_buffer, cache_p, cache_rest_length);
	    if (zio_lseek(&book->subbook_current->text_zio, 
		context->location + cache_rest_length, SEEK_SET) == -1) {
		error_code = EB_ERR_FAIL_SEEK_TEXT;
		goto failed;
	    }

	    read_result = zio_read(&book->subbook_current->text_zio, 
		cache_buffer + cache_rest_length,
		EB_SIZE_PAGE - cache_rest_length);
	    if (read_result < 0) {
		error_code = EB_ERR_FAIL_READ_TEXT;
		goto failed;
	    } else if (read_result != EB_SIZE_PAGE - cache_rest_length)
		context->file_end_flag = 1;

	    cache_book_code = book->code;
	    cache_location = context->location;
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
	c1 = eb_uint1(cache_p);

	if (c1 == 0x1f) {
	    hook = &null_hook;

	    /*
	     * This is escape sequences.
	     */
	    if (cache_rest_length < 2) {
		error_code = EB_ERR_UNEXP_TEXT;
		goto failed;
	    }
	    argv[0] = eb_uint2(cache_p);
	    c2 = eb_uint1(cache_p + 1);

	    switch (c2) {
	    case 0x02:
		/* beginning of text */
		context->in_step = 2;
		break;

	    case 0x03:
		/* end of text (don't set `in_step') */
		context->text_end_flag = 1;
		goto succeeded;

	    case 0x04:
		/* beginning of NARROW */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_NARROW;
		context->narrow_flag = 1;
		break;

	    case 0x05:
		/* end of NARROW */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_END_NARROW;
		context->narrow_flag = 0;
		break;

	    case 0x06:
		/* beginning of subscript */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_SUBSCRIPT;
		break;

	    case 0x07:
		/* end of subscript */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_END_SUBSCRIPT;
		break;

	    case 0x09:
		/* set indent */
		context->in_step = 4;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 2;
		argv[1] = eb_uint2(cache_p + 2);
		hook = hookset->hooks + EB_HOOK_STOP_CODE;

		if (0 < context->printable_count
		    && context->code == EB_TEXT_TEXT
		    && hook->function != NULL) {
		    error_code = hook->function(book, appendix, container,
			EB_HOOK_STOP_CODE, argc, argv);
		    if (error_code == EB_ERR_STOP_CODE) {
			context->text_end_flag = 1;
			goto succeeded;
		    } else if (error_code != EB_SUCCESS)
			goto failed;
		}

		hook = hookset->hooks + EB_HOOK_SET_INDENT;
		break;

	    case 0x0a:
		/* newline */
		context->in_step = 2;
		if (context->code == EB_TEXT_HEADING) {
		    context->text_end_flag = 1;
		    context->location += context->in_step;
		    goto succeeded;
		}
		hook = hookset->hooks + EB_HOOK_NEWLINE;
		break;

	    case 0x0e:
		/* beginning of superscript */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_SUPERSCRIPT;
		break;

	    case 0x0f:
		/* end of superscript */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_END_SUPERSCRIPT;
		break;

	    case 0x10:
		/* beginning of newline prohibition */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_NO_NEWLINE;
		break;

	    case 0x11:
		/* end of newline prohibition */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_END_NO_NEWLINE;
		break;

	    case 0x12:
		/* beginning of emphasis */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_EMPHASIS;
		break;

	    case 0x13:
		/* end of emphasis */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_END_EMPHASIS;
		break;

	    case 0x14:
		context->in_step = 4;
		context->skip_code = 0x15;
		break;

	    case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
	    case 0xe0:
		/* emphasis; described in JIS X 4081-1996 */
		context->in_step = 4;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		/* Some old EB books don't take an argument. */
		if (book->disc_code != EB_DISC_EPWING
		    && eb_uint1(cache_p + 2) >= 0x1f)
		    context->in_step = 2;
		break;

	    case 0x32:
		/* beginning of reference to monochrome graphic */
		context->in_step = 2;
		argc = 4;
		argv[1] = 0;
		argv[2] = 0;
		argv[3] = 0;
		hook = hookset->hooks + EB_HOOK_BEGIN_MONO_GRAPHIC;
		break;

	    case 0x39:
		/* beginning of MPEG movie */
		context->in_step = 46;
		argc = 6;
		argv[1] = eb_uint4(cache_p + 2);
		argv[2] = eb_uint4(cache_p + 22);
		argv[3] = eb_uint4(cache_p + 26);
		argv[4] = eb_uint4(cache_p + 30);
		argv[5] = eb_uint4(cache_p + 34);
		hook = hookset->hooks + EB_HOOK_BEGIN_MPEG;
		break;

	    case 0x35: case 0x36: case 0x37: case 0x38: case 0x3a:
	    case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		context->in_step = 2;
		context->skip_code = eb_uint1(cache_p + 1) + 0x20;
		break;

	    case 0x41:
		/* beginning of keyword */
		context->in_step = 4;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 2;
		argv[1] = eb_uint2(cache_p + 2);
		hook = hookset->hooks + EB_HOOK_STOP_CODE;

		if (0 < context->printable_count
		    && context->code == EB_TEXT_TEXT
		    && hook->function != NULL) {
		    error_code = hook->function(book, appendix, container,
			EB_HOOK_STOP_CODE, argc, argv);
		    if (error_code == EB_ERR_STOP_CODE) {
			context->text_end_flag = 1;
			goto succeeded;
		    } else if (error_code != EB_SUCCESS)
			goto failed;
		}
		if (context->auto_stop_code < 0)
		    context->auto_stop_code = eb_uint2(cache_p + 2);

		hook = hookset->hooks + EB_HOOK_BEGIN_KEYWORD;
		break;

	    case 0x42:
		/* beginning of reference */
		context->in_step = 4;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		if (eb_uint1(cache_p + 2) != 0x00)
		    context->in_step -= 2;
		hook = hookset->hooks + EB_HOOK_BEGIN_REFERENCE;
		break;

	    case 0x43:
		/* beginning of an entry of a candidate */
		context->in_step = 2;
		if (context->skip_code == SKIP_CODE_NONE) {
		    context->candidate[0] = '\0';
		    context->is_candidate = 1;
		    candidate_length = 0;
		    candidate_p = (unsigned char *)context->candidate;
		}
		hook = hookset->hooks + EB_HOOK_BEGIN_CANDIDATE;
		break;

	    case 0x44:
		/* beginning of monochrome graphic */
		context->in_step = 12;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 4;
		argv[1] = eb_uint2(cache_p + 2);
		argv[2] = eb_bcd4(cache_p + 4);
		argv[3] = eb_bcd4(cache_p + 8);
		if (0 < argv[2] && 0 < argv[3])
		    hook = hookset->hooks + EB_HOOK_BEGIN_MONO_GRAPHIC;
		break;

	    case 0x45:
		/* prefix of sound or picuture */
		context->in_step = 4;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		if (eb_uint1(cache_p + 2) != 0x1f) {
		    argc = 2;
		    argv[1] = eb_bcd4(cache_p + 2);
		}
		break;

	    case 0x4a:
		/* beginning of WAVE sound */
		context->in_step = 18;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 6;
		argv[1] = eb_uint4(cache_p + 2);
		argv[2] = eb_bcd4(cache_p + 6);
		argv[3] = eb_bcd2(cache_p + 10);
		argv[4] = eb_bcd4(cache_p + 12);
		argv[5] = eb_bcd2(cache_p + 16);
		hook = hookset->hooks + EB_HOOK_BEGIN_WAVE;
		break;

	    case 0x4d:
		/* beginning of color graphic (BMP or JPEG) */
		context->in_step = 20;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 4;
		argv[1] = eb_uint2(cache_p + 2);
		argv[2] = eb_bcd4(cache_p + 14);
		argv[3] = eb_bcd2(cache_p + 18);
		if (argv[1] >> 8 == 0x00)
		    hook = hookset->hooks + EB_HOOK_BEGIN_COLOR_BMP;
		else
		    hook = hookset->hooks + EB_HOOK_BEGIN_COLOR_JPEG;
		break;

	    case 0x49: case 0x4b: case 0x4c: case 0x4e: case 0x4f:
		context->in_step = 2;
		context->skip_code = eb_uint1(cache_p + 1) + 0x20;
		break;

	    case 0x52:
		/* end of reference to monochrome graphic */
		context->in_step = 8;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		hook = hookset->hooks + EB_HOOK_END_MONO_GRAPHIC;
		break;

	    case 0x59:
		/* end of MPEG movie */
		context->in_step = 2;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_MPEG;
		break;

	    case 0x61:
		/* end of keyword */
		context->in_step = 2;
		hook = hookset->hooks + EB_HOOK_END_KEYWORD;
		break;

	    case 0x62:
		/* end of reference */
		context->in_step = 8;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		hook = hookset->hooks + EB_HOOK_END_REFERENCE;
		break;

	    case 0x63:
		/* end of an entry of a candidate */
		context->in_step = 8;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		if (argv[1] == 0 && argv[2] == 0)
		    hook = hookset->hooks + EB_HOOK_END_CANDIDATE_LEAF;
		else
		    hook = hookset->hooks + EB_HOOK_END_CANDIDATE_GROUP;
		break;

	    case 0x64:
		/* end of monochrome graphic */
		context->in_step = 8;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		argc = 3;
		argv[1] = eb_bcd4(cache_p + 2);
		argv[2] = eb_bcd2(cache_p + 6);
		if (0 < argv[1] && 0 < argv[2])
		    hook = hookset->hooks + EB_HOOK_END_MONO_GRAPHIC;
		break;

	    case 0x6a:
		/* end of WAVE sound */
		context->in_step = 2;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_WAVE;
		break;

	    case 0x6d:
		/* end of color graphic (BMP or JPEG) */
		context->in_step = 2;
		if (cache_rest_length < context->in_step) {
		    error_code = EB_ERR_UNEXP_TEXT;
		    goto failed;
		}
		hook = hookset->hooks + EB_HOOK_END_COLOR_GRAPHIC;
		break;

	    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
	    case 0x76: case 0x77: case 0x78: case 0x79: case 0x7a: case 0x7b:
	    case 0x7c: case 0x7d: case 0x7e: case 0x7f:
	    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
	    case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b:
	    case 0x8c: case 0x8d: case 0x8e: case 0x8f:
		context->in_step = 2;
		context->skip_code = eb_uint1(cache_p + 1) + 0x20;
		break;

	    case 0xe1:
		context->in_step = 2;
		break;

	    case 0xe4: case 0xe6: case 0xe8: case 0xea: case 0xec: case 0xee:
	    case 0xf0: case 0xf2: case 0xf4: case 0xf6: case 0xf8: case 0xfa:
	    case 0xfc: case 0xfe:
		context->in_step = 2;
		context->skip_code = eb_uint1(cache_p + 1) + 0x01;
		break;

	    default:
		context->in_step = 2;
		if (context->skip_code == eb_uint1(cache_p + 1))
		    context->skip_code = SKIP_CODE_NONE;
		break;
	    }

	    if (context->skip_code == SKIP_CODE_NONE
		&& hook->function != NULL) {
		error_code = hook->function(book, appendix, container,
		    hook->code, argc, argv);
		if (error_code != EB_SUCCESS)
		    goto failed;
	    }

	    /*
	     * Post process.  Clean a candidate.
	     */
	    if (c2 == 0x63) {
		/* end of an entry of candidate */
		context->is_candidate = 0;
	    }

	} else if (book->character_code == EB_CHARCODE_ISO8859_1) {
	    /*
	     * The book is mainly written in ISO 8859 1.
	     */
	    context->printable_count++;

	    if ((0x20 <= c1 && c1 < 0x7f) || (0xa0 <= c1 && c1 <= 0xff)) {
		/*
		 * This is an ISO 8859 1 character.
		 */
		context->in_step = 1;
		argv[0] = eb_uint1(cache_p);

		if (context->skip_code == SKIP_CODE_NONE) {
		    if (context->is_candidate
			&& candidate_length < EB_MAX_WORD_LENGTH) {
			*candidate_p++ = c1 | 0x80;
			*candidate_p = '\0';
			candidate_length++;
		    }

		    hook = hookset->hooks + EB_HOOK_ISO8859_1;
		    if (hook->function == NULL) {
			error_code = eb_write_text_byte1(book, c1);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    } else {
			error_code = hook->function(book, appendix, container,
			    EB_HOOK_ISO8859_1, argc, argv);
			if (error_code != EB_SUCCESS)
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

		context->in_step = 2;
		argv[0] = eb_uint2(cache_p);
		if (context->skip_code == SKIP_CODE_NONE) {
		    hook = hookset->hooks + EB_HOOK_NARROW_FONT;
		    if (hook->function == NULL) {
			error_code = eb_write_text_byte1(book, c1);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    } else {
			error_code = hook->function(book, appendix, container,
			    EB_HOOK_NARROW_FONT, argc, argv);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    }
		}
	    }

	} else {
	    /*
	     * The book is written in JIS X 0208 or JIS X 0208 + GB 2312.
	     */
	    context->printable_count++;
	    context->in_step = 2;

	    if (cache_rest_length < 2) {
		error_code = EB_ERR_UNEXP_TEXT;
		goto failed;
	    }

	    c2 = eb_uint1(cache_p + 1);

	    if (context->skip_code != SKIP_CODE_NONE) {
		/* nothing to be done. */
	    } else if (0x20 < c1 && c1 < 0x7f && 0x20 < c2 && c2 < 0x7f) {
		/*
		 * This is a JIS X 0208 KANJI character.
		 */
		argv[0] = eb_uint2(cache_p) | 0x8080;

		if (context->is_candidate
		    && candidate_length < EB_MAX_WORD_LENGTH - 1) {
		    *candidate_p++ = c1 | 0x80;
		    *candidate_p++ = c2 | 0x80;
		    *candidate_p = '\0';
		    candidate_length += 2;
		}

		if (context->narrow_flag) {
		    hook = hookset->hooks + EB_HOOK_NARROW_JISX0208;
		    if (hook->function == NULL) {
			error_code = eb_write_text_byte2(book, c1 | 0x80,
			    c2 | 0x80);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    } else {
			error_code = hook->function(book, appendix, container,
			    EB_HOOK_NARROW_JISX0208, 0, argv);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    }
		} else {
		    hook = hookset->hooks + EB_HOOK_WIDE_JISX0208;
		    if (hook->function == NULL) {
			error_code = eb_write_text_byte2(book, c1 | 0x80,
			    c2 | 0x80);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    } else {
			error_code = hook->function(book, appendix, container,
			    EB_HOOK_WIDE_JISX0208, argc, argv);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    }
		}
	    } else if (0x20 < c1 && c1 < 0x7f && 0xa0 < c2 && c2 < 0xff) {
		/*
		 * This is a GB 2312 HANJI character.
		 */
		argv[0] = eb_uint2(cache_p) | 0x0080;

		if (context->is_candidate
		    && candidate_length < EB_MAX_WORD_LENGTH - 1) {
		    *candidate_p++ = c1;
		    *candidate_p++ = c2 | 0x80;
		    *candidate_p   = '\0';
		    candidate_length += 2;
		}

		hook = hookset->hooks + EB_HOOK_GB2312;
		if (hook->function == NULL) {
		    error_code = eb_write_text_byte2(book, c1 | 0x80, c2);
		    if (error_code != EB_SUCCESS)
			goto failed;
		} else {
		    error_code = hook->function(book, appendix, container,
			EB_HOOK_GB2312, 0, argv);
		    if (error_code != EB_SUCCESS)
			goto failed;
		}
	    } else {
		/*
		 * This is a local character.
		 */
		argv[0] = eb_uint2(cache_p);

		if (context->narrow_flag) {
		    hook = hookset->hooks + EB_HOOK_NARROW_FONT;
		    if (hook->function == NULL) {
			error_code = eb_write_text_byte2(book, c1, c2);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    } else {
			error_code = hook->function(book, appendix, container,
			    EB_HOOK_NARROW_FONT, argc, argv);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    }
		} else {
		    hook = hookset->hooks + EB_HOOK_WIDE_FONT;
		    if (hook->function == NULL) {
			error_code = eb_write_text_byte2(book, c1, c2);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    } else {
			error_code = hook->function(book, appendix, container,
			    EB_HOOK_WIDE_FONT, argc, argv);
			if (error_code != EB_SUCCESS)
			    goto failed;
		    }
		}
	    }
	}

	/*
	 * Break if an unprocessed character is remained.
	 */
	if (context->unprocessed != NULL)
	    break;

	/*
	 * Update variables.
	 */
	cache_p += context->in_step;
	cache_rest_length -= context->in_step;
	context->location += context->in_step;
	context->in_step = 0;
    }

  succeeded:
    *text_length = (context->out - text);

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
    context->code = EB_TEXT_INVALID;
    pthread_mutex_unlock(&cache_mutex);
    return error_code;
}


/*
 * Write a byte to a text buffer.
 */
EB_Error_Code
eb_write_text_byte1(book, byte1)
    EB_Book *book;
    int byte1;
{
    char stream[1];

    /*
     * If the text buffer has enough space to write `byte1',
     * save the byte in `book->text_context.unprocessed'.
     */
    if (book->text_context.unprocessed != NULL
	|| book->text_context.out_rest_length < 1) {
	*(unsigned char *)stream = byte1;
	return eb_write_text(book, stream, 1);
    }

    /*
     * Write the byte.
     */
    *(book->text_context.out) = byte1;
    book->text_context.out++;
    book->text_context.out_rest_length--;
    book->text_context.out_step++;

    return EB_SUCCESS;
}


/*
 * Write two bytes to a text buffer for output.
 */
EB_Error_Code
eb_write_text_byte2(book, byte1, byte2)
    EB_Book *book;
    int byte1;
    int byte2;
{
    char stream[2];

    /*
     * If the text buffer has enough space to write `byte1' and `byte2',
     * save the bytes in `book->text_context.unprocessed'.
     */
    if (book->text_context.unprocessed != NULL
	|| book->text_context.out_rest_length < 2) {
	*(unsigned char *)stream = byte1;
	*(unsigned char *)(stream + 1) = byte1;
	return eb_write_text(book, stream, 2);
    }

    /*
     * Write the two bytes.
     */
    *(book->text_context.out) = byte1;
    book->text_context.out++;
    *(book->text_context.out) = byte2;
    book->text_context.out++;
    book->text_context.out_rest_length -= 2;
    book->text_context.out_step += 2;

    return EB_SUCCESS;
}


/*
 * Write a string to a text buffer.
 */
EB_Error_Code
eb_write_text_string(book, string)
    EB_Book *book;
    const char *string;
{
    size_t string_length;

    /*
     * If the text buffer has enough space to write `sting',
     * save the string in `book->text_context.unprocessed'.
     */
    string_length = strlen(string);
    if (book->text_context.unprocessed != NULL
	|| book->text_context.out_rest_length < string_length) {
	return eb_write_text(book, string, string_length);
    }

    /*
     * Write the string.
     */
    memcpy(book->text_context.out, string, string_length);
    book->text_context.out += string_length;
    book->text_context.out_rest_length -= string_length;
    book->text_context.out_step += string_length;

    return EB_SUCCESS;
}


/*
 * Write a stream with `length' bytes to a text buffer.
 */
EB_Error_Code
eb_write_text(book, stream, stream_length)
    EB_Book *book;
    const char *stream;
    size_t stream_length;
{
    char *reallocated;

    /*
     * If the text buffer has enough space to write `stream',
     * save the stream in `book->text_context.unprocessed'.
     */
    if (book->text_context.unprocessed != NULL) {
	reallocated = (char *)realloc(book->text_context.unprocessed, 
	    book->text_context.unprocessed_size + stream_length);
	if (reallocated == NULL) {
	    free(book->text_context.unprocessed);
	    book->text_context.unprocessed = NULL;
	    book->text_context.unprocessed_size = 0;
	    return EB_ERR_MEMORY_EXHAUSTED;
	}
	memcpy(reallocated + book->text_context.unprocessed_size, stream,
	    stream_length);
	book->text_context.unprocessed = reallocated;
	book->text_context.unprocessed_size += stream_length;
	return EB_SUCCESS;
	    
    } else if (book->text_context.out_rest_length < stream_length) {
	book->text_context.unprocessed
	    = (char *)malloc(book->text_context.out_step + stream_length);
	if (book->text_context.unprocessed == NULL)
	    return EB_ERR_MEMORY_EXHAUSTED;
	book->text_context.unprocessed_size
	    = book->text_context.out_step + stream_length;
	memcpy(book->text_context.unprocessed, 
	    book->text_context.out - book->text_context.out_step,
	    book->text_context.out_step);
	memcpy(book->text_context.unprocessed + book->text_context.out_step,
	    stream, stream_length);
	book->text_context.out -= book->text_context.out_step;
	book->text_context.out_step = 0;
	return EB_SUCCESS;
    }

    /*
     * Write the stream.
     */
    memcpy(book->text_context.out, stream, stream_length);
    book->text_context.out += stream_length;
    book->text_context.out_rest_length -= stream_length;
    book->text_context.out_step += stream_length;

    return EB_SUCCESS;
}


/*
 * Get the current candidate word for multi search.
 */
const char *
eb_current_candidate(book)
    EB_Book *book;
{
    if (!book->text_context.is_candidate)
	book->text_context.candidate[0] = '\0';

    return book->text_context.candidate;
}


/*
 * Forward text position to the next paragraph.
 */
EB_Error_Code
eb_forward_text(book, hookset)
    EB_Book *book;
    EB_Hookset *hookset;
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
    if (zio_file(&book->subbook_current->text_zio) < 0) {
	error_code = EB_ERR_NO_TEXT;
	goto failed;
    }

    /*
     * Clear work buffer in the text context.
     */
    if (book->text_context.unprocessed != NULL) {
	free(book->text_context.unprocessed);
	book->text_context.unprocessed = NULL;
	book->text_context.unprocessed_size = 0;
    }

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
	error_code = eb_read_text_internal(book, NULL, hookset, NULL,
	    EB_SIZE_PAGE, text, &text_length);
	if (error_code != EB_SUCCESS)
	    goto failed;
	if (text_length == 0)
	    break;
    }

    /*
     * Disable the text-end flag.
     */
    if (book->text_context.unprocessed != NULL) {
	free(book->text_context.unprocessed);
	book->text_context.unprocessed = NULL;
	book->text_context.unprocessed_size = 0;
    }
    book->text_context.in_step = 0;
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
	    NULL, EB_SIZE_PAGE, text, &text_length);
	if (error_code != EB_SUCCESS)
	    goto failed;
	if (text_length == 0)
	    break;
    }

    /*
     * Initialize the context flags.
     */
    if (book->text_context.unprocessed != NULL) {
	free(book->text_context.unprocessed);
	book->text_context.unprocessed = NULL;
	book->text_context.unprocessed_size = 0;
    }
    book->text_context.in_step = 0;
    book->text_context.narrow_flag = 0;
    book->text_context.printable_count = 0;
    book->text_context.text_end_flag = 0;
    book->text_context.skip_code = SKIP_CODE_NONE;

    /*
     * Unlock cache data.
     */
  succeeded:
    eb_unlock(&book->lock);
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


