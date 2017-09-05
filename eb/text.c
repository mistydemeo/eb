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

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "text.h"

/*
 * Local macros.
 */
/* method */
#define CONTENT_TEXT		0
#define CONTENT_HEADING		1
#define CONTENT_RAWTEXT		2

/*
 * Unexported variables.
 */
/* How many times call an eb_read_ function. */
static int callcount;

/* Book-code of the book in which you want to get content. */
static EB_Book_Code bookcode = -1;

/* Current subbook-code of the book. */
static EB_Subbook_Code subcode = -1;

/* Current method; one of CONTENT_*. */
static int method = -1;

/* Current offset pointer of the START or HONMON file. */
static off_t location;

/* Cached page. */
static char pagebuf[EB_SIZE_PAGE];

/* Current pointer to the cached page */
static char *pagebufp;

/* Rest data size of the cached page. */
static size_t pagerest;

/* Work buffer passed to hook functions. */
static char workbuf[EB_MAXLEN_TEXT_WORK + 1];

/* Length of a string in `workbuf'. */
static size_t worklen;

/* Hook to process the current modifier */
static const EB_Hook *modhook;

/* Hook to process the current reference */
static const EB_Hook *refhook;

/* Narrow character region flag. */
static int narwflag;

/* Whether a printable character is appeared in the current text content. */
static int charapp;

/* EOF flag of the current subbook. */
static int bookeof;

/* Whether the current text content ends or not. */
static int textend;

/* Skip until `skipcoe' appears, if set to 0 or positive integer. */
static int skipcode;

/* Null hook. */
static const EB_Hook nullhook = {EB_HOOK_NULL, NULL};

/* default hookset */
static EB_Hookset default_hookset;

/* Whether `default_hookset' is initialized or not. */
static int default_hookset_initialized = 0;

/*
 * The maximum number of arguments for an escape sequence.
 */
#define EB_MAX_ARGV 4

/*
 * Read next page when the length of rest data is less than the value.
 */
#define SIZE_FEW_REST		16

/*
 * Unexported functions.
 */
static int eb_read_internal EB_P((EB_Book *, EB_Appendix *,
    const EB_Hookset *, char *, size_t));


/*
 * Initialize text processing status.
 */
void
eb_initialize_text()
{
    bookcode = -1;
    subcode = -1;
    method = -1;
    callcount = 0;
}


/*
 * Initialize a hookset.
 */
void
eb_initialize_hookset(hookset)
    EB_Hookset *hookset;
{
    int i;

    for (i = 0; i < EB_NUM_HOOKS; i++) {
	hookset->hooks[i].code = i;
	hookset->hooks[i].function = NULL;
    }
    hookset->hooks[EB_HOOK_STOPCODE].function = eb_hook_stopcode;
    hookset->hooks[EB_HOOK_NARROW_JISX0208].function = eb_hook_euc_to_ascii;
    hookset->hooks[EB_HOOK_NARROW_FONT].function
	= eb_hook_narrow_character_text;
    hookset->hooks[EB_HOOK_WIDE_FONT].function
	= eb_hook_wide_character_text;
}


/*
 * Set a hook.
 */
int
eb_set_hook(hookset, hook)
    EB_Hookset *hookset;
    const EB_Hook *hook;
{
    if (hook->code < 0 || EB_NUM_HOOKS <= hook->code) {
	eb_error = EB_ERR_NO_SUCH_HOOK;
	return -1;
    }

    if (hook->code != EB_HOOK_NULL)
	hookset->hooks[hook->code].function = hook->function;

    return 0;
}


/*
 * Set a list of hooks.
 */
int
eb_set_hooks(hookset, hook)
    EB_Hookset *hookset;
    const EB_Hook *hook;
{
    const EB_Hook *hk;
    int i;

    for (i = 0, hk = hook;
	 i < EB_NUM_HOOKS && hk->code != EB_HOOK_NULL; i++, hk++) {
	if (hook->code < 0 || EB_NUM_HOOKS <= hook->code) {
	    eb_error = EB_ERR_NO_SUCH_HOOK;
	    return -1;
	}
	hookset->hooks[hk->code].function = hk->function;
    }

    return 0;
}


/*
 * Reposition the offset of the subbook file.
 */
int
eb_seek(book, position)
    EB_Book *book;
    const EB_Position *position;
{
    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }
    if (book->sub_current->sub_file < 0) {
	eb_error = EB_ERR_NO_START;
	return -1;
    }

    /*
     * Initialize static variables.
     */
    callcount = 0;
    bookcode = book->code;
    subcode = book->sub_current->code;
    location = (position->page - 1) * EB_SIZE_PAGE + position->offset;
    pagebufp = pagebuf;
    pagerest = 0;
    worklen = 0;
    modhook = &nullhook;
    refhook = &nullhook;
    narwflag = 0;
    charapp = 0;
    bookeof = 0;
    textend = 0;
    skipcode = -1;

    return 0;
}


/*
 * Get text in the current subbook in `book'.
 */
int
eb_text(book, appendix, hookset, text, textsize)
    EB_Book *book;
    EB_Appendix *appendix;
    const EB_Hookset *hookset;
    char *text;
    size_t textsize;
{
    EB_Subbook *sub = book->sub_current;
    const EB_Hook *hook;

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (sub == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }
    if (sub->sub_file < 0) {
	eb_error = EB_ERR_NO_START;
	return -1;
    }

    /*
     * Use `default_hookset' when `hookset' is `NULL'.
     */
    if (hookset == NULL) {
	if (!default_hookset_initialized) {
	    eb_initialize_hookset(&default_hookset);
	    default_hookset_initialized = 1;
	}
	hookset = &default_hookset;
    }

    if (0 < callcount) {
	if (bookcode == -1) {
	    eb_error = EB_ERR_NO_PREV_CONTENT;
	    return -1;
	}
	if (bookcode != book->code) {
	    eb_error = EB_ERR_DIFF_BOOK;
	    return -1;
	}
	if (subcode != sub->code) {
	    eb_error = EB_ERR_DIFF_SUBBOOK;
	    return -1;
	}
	if (method != CONTENT_TEXT) {
	    eb_error = EB_ERR_DIFF_CONTENT;
	    return -1;
	}
	if (textend)
	    return 0;
    } else {
	method = CONTENT_TEXT;

	/*
	 * Call the function bound to `EB_HOOK_INITIALIZE'.
	 */
	hook = hookset->hooks + EB_HOOK_INITIALIZE;
	if (hook->function != NULL && hook->function(book, appendix, NULL,
	    EB_HOOK_INITIALIZE, 0, NULL) < 0) {
	    eb_error = EB_ERR_HOOK_WORKSPACE;
	    return -1;
	}
    }

    callcount++;
    return eb_read_internal(book, appendix, hookset, text, textsize);
}


/*
 * Get text in the current subbook in `book'.
 */
int
eb_heading(book, appendix, hookset, text, textsize)
    EB_Book *book;
    EB_Appendix *appendix;
    const EB_Hookset *hookset;
    char *text;
    size_t textsize;
{
    EB_Subbook *sub = book->sub_current;
    const EB_Hook *hook;

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (sub == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }
    if (sub->sub_file < 0) {
	eb_error = EB_ERR_NO_START;
	return -1;
    }

    /*
     * Use `default_hookset' when `hookset' is `NULL'.
     */
    if (hookset == NULL) {
	if (!default_hookset_initialized) {
	    eb_initialize_hookset(&default_hookset);
	    default_hookset_initialized = 1;
	}
	hookset = &default_hookset;
    }

    if (0 < callcount) {
	if (bookcode == -1) {
	    eb_error = EB_ERR_NO_PREV_CONTENT;
	    return -1;
	}
	if (bookcode != book->code) {
	    eb_error = EB_ERR_DIFF_BOOK;
	    return -1;
	}
	if (subcode != sub->code) {
	    eb_error = EB_ERR_DIFF_SUBBOOK;
	    return -1;
	}
	if (method != CONTENT_HEADING) {
	    eb_error = EB_ERR_DIFF_CONTENT;
	    return -1;
	}
	if (textend)
	    return 0;
    } else {
	method = CONTENT_HEADING;

	/*
	 * Call the function bound to `EB_HOOK_INITIALIZE'.
	 */
	hook = hookset->hooks + EB_HOOK_INITIALIZE;
	if (hook->function != NULL && hook->function(book, appendix, NULL,
	    EB_HOOK_INITIALIZE, 0, NULL) < 0) {
	    eb_error = EB_ERR_HOOK_WORKSPACE;
	    return -1;
	}
    }

    callcount++;
    return eb_read_internal(book, appendix, hookset, text, textsize);
}


/*
 * Read data from the subbook file directly.
 */
ssize_t
eb_rawtext(book, buffer, length)
    EB_Book *book;
    char *buffer;
    size_t length;
{
    ssize_t ssize;

    /*
     * Current subbook must have been set and START file must be exist.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }
    if (book->sub_current->sub_file < 0) {
	eb_error = EB_ERR_NO_START;
	return -1;
    }

    if (0 < callcount) {
	if (bookcode == -1) {
	    eb_error = EB_ERR_NO_PREV_CONTENT;
	    return -1;
	}
	if (bookcode != book->code) {
	    eb_error = EB_ERR_DIFF_BOOK;
	    return -1;
	}
	if (subcode != book->sub_current->code) {
	    eb_error = EB_ERR_DIFF_SUBBOOK;
	    return -1;
	}
	if (method != CONTENT_RAWTEXT) {
	    eb_error = EB_ERR_DIFF_CONTENT;
	    return -1;
	}
    } else
	method = CONTENT_RAWTEXT;

    callcount++;

    /*
     * Seek START file and read data.
     */
    if (eb_zlseek(&(book->sub_current->zip), 
	book->sub_current->sub_file, location, SEEK_SET) == -1) {
	eb_error = EB_ERR_FAIL_SEEK_START;
	return -1;
    }
    ssize = eb_zread(&(book->sub_current->zip)
	, book->sub_current->sub_file, buffer, length);
    if (ssize != length)
	eb_error = EB_ERR_FAIL_SEEK_START;

    return ssize;
}


/*
 * Get text or heading.
 */
static int
eb_read_internal(book, appendix, hookset, text, textsize)
    EB_Book *book;
    EB_Appendix *appendix;
    const EB_Hookset *hookset;
    char *text;
    size_t textsize;
{
    char *textp = text;
    unsigned char c;
    size_t textrest = textsize;
    const EB_Hook *hook;
    int step;
    int argv[EB_MAX_ARGV];
    int argc;

    /*
     * If unprocessed data are rest in `workbuf', copy them to `textp'.
     */
    if (0 < worklen) {
	if (textrest < worklen) {
	    *textp = '\0';
	    return 0;
	}
	strcpy(textp, workbuf);
	textp += worklen;
	textrest -= worklen;
	worklen = 0;
    }

    for (;;) {
	step = 0;
	*workbuf = '\0';
	argc = 1;

	/*
	 * If reached to the near of buffer's end, read next page.
	 */
	if (pagerest < SIZE_FEW_REST && !bookeof) {
	    ssize_t len;

	    memcpy(pagebuf, pagebufp, pagerest);
	    if (eb_zlseek(&(book->sub_current->zip), 
		book->sub_current->sub_file, location, SEEK_SET) == -1) {
		eb_error = EB_ERR_FAIL_SEEK_START;
		return -1;
	    }
	    len = eb_zread(&(book->sub_current->zip), 
		book->sub_current->sub_file, pagebuf + pagerest,
		EB_SIZE_PAGE - pagerest);
	    if (len < 0) {
		eb_error = EB_ERR_FAIL_READ_START;
		return -1;
	    } else if (len != EB_SIZE_PAGE - pagerest)
		bookeof = 1;
	    location += len;
	    pagebufp = pagebuf;
	    pagerest += len;
	}

	/*
	 * Get 1 byte from the buffer.
	 */
	if (pagerest < 1) {
	    eb_error = EB_ERR_UNEXP_START;
	    return -1;
	}
	c = eb_uint1(pagebufp);

	if (c == 0x1f) {
	    const EB_Hook *ctlhook = &nullhook;

	    if (pagerest < 2) {
		eb_error = EB_ERR_UNEXP_START;
		return -1;
	    }
	    step = 2;
	    argv[0] = eb_uint2(pagebufp);

	    /*
	     * Is is an escape sequences.
	     */
	    switch (eb_uint1(pagebufp + 1)) {
	    case 0x02:
		/* beginning of text */
		break;

	    case 0x03:
		/* end of text */
		goto at_end;

	    case 0x04:
		/* beginning of NARROW */
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_NARROW;
		narwflag = 1;
		break;

	    case 0x05:
		/* end of NARROW */
		ctlhook = hookset->hooks + EB_HOOK_END_NARROW;
		narwflag = 0;
		break;

	    case 0x06:
		/* beginning of subscript */
		modhook = hookset->hooks + EB_HOOK_SUBSCRIPT;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_SUBSCRIPT;
		break;

	    case 0x07:
		/* end of subscript */
		modhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_SUBSCRIPT;
		break;

	    case 0x09:
		/* set indent */
		if (pagerest < step + 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		ctlhook = hookset->hooks + EB_HOOK_SET_INDENT;
		step += 2;
		argc = 2;
		argv[1] = eb_uint2(pagebufp + 2);

		hook = hookset->hooks + EB_HOOK_STOPCODE;
		if (charapp && method == CONTENT_TEXT && hook->function != NULL
		    && hook->function(book, appendix, workbuf,
			EB_HOOK_STOPCODE, argc, argv) < 0)
		    goto at_end;
		break;

	    case 0x0a:
		/* newline */
		if (method == CONTENT_HEADING) {
		    textend = 1;
		    goto at_end;
		}
		if (skipcode < 0) {
		    *workbuf = '\n';
		    *(workbuf + 1) = '\0';
		}
		ctlhook = hookset->hooks + EB_HOOK_NEWLINE;
		break;

	    case 0x0e:
		/* beginning of superscript */
		modhook = hookset->hooks + EB_HOOK_SUPERSCRIPT;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_SUPERSCRIPT;
		break;

	    case 0x0f:
		/* end of superscript */
		modhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_SUPERSCRIPT;
		break;

	    case 0x10:
		/* beginning of newline prohibition */
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_NO_NEWLINE;
		break;

	    case 0x11:
		/* end of newline prohibition */
		ctlhook = hookset->hooks + EB_HOOK_END_NO_NEWLINE;
		break;

	    case 0x12:
		/* beginning of emphasis */
		modhook = hookset->hooks + EB_HOOK_EMPHASIS;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_EMPHASIS;
		break;

	    case 0x13:
		/* end of emphasis */
		modhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_EMPHASIS;
		break;

	    case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
	    case 0xe0:
		/* emphasis; described in JIS X 4081-1996 */
		if (pagerest < step + 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		/* Some old EB books don't take an argument. */
		if (book->disc_code == EB_DISC_EPWING
		    || eb_uint1(pagebufp + 2) < 0x1f)
		    step += 2;
		break;

	    case 0x32:
		/* beginning of an anchor of the picture */
		modhook = hookset->hooks + EB_HOOK_PICTURE;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_PICTURE;
		break;

	    case 0x33:
		/* beginning of an anchor of the sound */
		refhook = hookset->hooks + EB_HOOK_SOUND;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_PICTURE;
		break;

	    case 0x35: case 0x36: case 0x37: case 0x38: case 0x39: case 0x3a:
	    case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
		skipcode = eb_uint1(pagebufp + 1) + 0x20;
		break;

	    case 0x41:
		/* beginning of the keyword */
		if (pagerest < step + 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		refhook = hookset->hooks + EB_HOOK_KEYWORD;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_KEYWORD;
		step += 2;
		argc = 2;
		argv[1] = eb_uint2(pagebufp + 2);

		hook = hookset->hooks + EB_HOOK_STOPCODE;
		if (charapp && method == CONTENT_TEXT && hook->function != NULL
		    && hook->function(book, appendix, workbuf,
			EB_HOOK_STOPCODE, argc, argv) < 0)
		    goto at_end;
		break;

	    case 0x42:
		/* beginning of the reference */
		if (pagerest < step + 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		if (eb_uint1(pagebufp + 2) == 0x00)
		    step += 2;
		refhook = hookset->hooks + EB_HOOK_REFERENCE;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_REFERENCE;
		break;

	    case 0x43:
		/* beginning of an entry of the menu */
		refhook = hookset->hooks + EB_HOOK_MENU;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_MENU;
		break;

	    case 0x44:
		/* beginning of an anchor of the picture (old style?) */
		if (pagerest < step + 10) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		modhook = hookset->hooks + EB_HOOK_PICTURE;
		ctlhook = hookset->hooks + EB_HOOK_BEGIN_PICTURE;
		step += 10;
		break;

	    case 0x45:
		/* prefix of sound or picuture */
		if (pagerest < step + 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		if (eb_uint1(pagebufp + 2) != 0x1f) {
		    argc = 2;
		    argv[1] = eb_bcd4(pagebufp + 2);
		    step += 2;
		}
		break;

	    case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e:
	    case 0x4f:
		skipcode = eb_uint1(pagebufp + 1) + 0x20;
		break;

	    case 0x52:
		/* end of the picture */
		if (pagerest < step + 6) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		refhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_PICTURE;
		step += 6;
		argc = 3;
		argv[1] = eb_bcd4(pagebufp + 2);
		argv[2] = eb_bcd2(pagebufp + 6);
		break;

	    case 0x53:
		/* end of the sound */
		if (pagerest < step + 8) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		refhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_SOUND;
		step += 8;
		break;

	    case 0x61:
		/* end of the keyword */
		refhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_KEYWORD;
		break;

	    case 0x62:
		/* end of the reference */
		if (pagerest < step + 6) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		refhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_REFERENCE;
		step += 6;
		argc = 3;
		argv[1] = eb_bcd4(pagebufp + 2);
		argv[2] = eb_bcd2(pagebufp + 6);
		break;

	    case 0x63:
		/* end of an entry of the menu */
		if (pagerest < step + 6) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		refhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_MENU;
		step += 6;
		argc = 3;
		argv[1] = eb_bcd4(pagebufp + 2);
		argv[2] = eb_bcd2(pagebufp + 6);
		break;

	    case 0x64:
		/* end of the picture (old style?) */
		if (pagerest < step + 6) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}
		refhook = &nullhook;
		ctlhook = hookset->hooks + EB_HOOK_END_PICTURE;
		step += 6;
		argc = 3;
		argv[1] = eb_bcd4(pagebufp + 2);
		argv[2] = eb_bcd2(pagebufp + 6);
		break;

	    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
	    case 0x76: case 0x77: case 0x78: case 0x79: case 0x7a: case 0x7b:
	    case 0x7c: case 0x7d: case 0x7e: case 0x7f:
	    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
	    case 0x86: case 0x87: case 0x88: case 0x89: case 0x8a: case 0x8b:
	    case 0x8c: case 0x8d: case 0x8e: case 0x8f:
		skipcode = eb_uint1(pagebufp + 1) + 0x20;
		break;

	    case 0xe1:
		break;

	    case 0xe4: case 0xe6: case 0xe8: case 0xea: case 0xec: case 0xee:
	    case 0xf0: case 0xf2: case 0xf4: case 0xf6: case 0xf8: case 0xfa:
	    case 0xfc: case 0xfe:
		skipcode = eb_uint1(pagebufp + 1) + 0x01;
		break;

	    default:
		if (skipcode == eb_uint1(pagebufp + 1))
		    skipcode = -1;
		break;
	    }

	    if (skipcode < 0 && ctlhook->function != NULL
		&& ctlhook->function(book, appendix, workbuf, ctlhook->code,
		    argc, argv) < 0) {
		eb_error = EB_ERR_HOOK_WORKSPACE;
		return -1;
	    }
	} else if (book->char_code == EB_CHARCODE_ISO8859_1) {
	    /*
	     * The book is mainly written in ISO 8859 1.
	     */
	    charapp = 1;

	    if ((0x20 <= c && c < 0x7f) || (0xa0 <= c && c < 0xff)) {
		/*
		 * This is an ISO 8859 1 character.
		 */
		step = 1;
		argv[0] = eb_uint1(pagebufp);
		if (skipcode < 0) {
		    *workbuf = c;
		    *(workbuf + 1) = '\0';
		    hook = hookset->hooks + EB_HOOK_ISO8859_1;
		    if (hook->function != NULL
			&& hook->function(book, appendix, workbuf,
			    EB_HOOK_ISO8859_1, argc, argv) < 0) {
			eb_error = EB_ERR_HOOK_WORKSPACE;
			return -1;
		    }
		}
	    } else {
		/*
		 * This is a local character.
		 */
		if (pagerest < 2) {
		    eb_error = EB_ERR_UNEXP_START;
		    return -1;
		}

		step = 2;
		argv[0] = eb_uint2(pagebufp);
		if (skipcode < 0) {
		    *workbuf = (unsigned char) c;
		    *(workbuf + 1) = (unsigned char) eb_uint1(pagebufp + 1);
		    *(workbuf + 2) = '\0';
		    hook = hookset->hooks + EB_HOOK_NARROW_FONT;
		    if (hook->function != NULL
			&& hook->function(book, appendix, workbuf,
			    EB_HOOK_NARROW_FONT, argc, argv) < 0) {
			eb_error = EB_ERR_HOOK_WORKSPACE;
			return -1;
		    }
		}
	    }

	    if (skipcode < 0 && modhook->function != NULL
		&& modhook->function(book, appendix, workbuf, modhook->code,
		    0, argv) < 0) {
		eb_error = EB_ERR_HOOK_WORKSPACE;
		return -1;
	    }
	    if (skipcode < 0 && refhook->function != NULL
		&& refhook->function(book, appendix, workbuf, refhook->code,
		    0, argv) < 0) {
		eb_error = EB_ERR_HOOK_WORKSPACE;
		return -1;
	    }
	} else {
	    /*
	     * The book is written in JIS X 0208 (EB, EBXA, EPWING)
	     * or JIS X 0208 & GB 2312 (EBXA-C).
	     */
	    int c2;

	    charapp = 1;
	    step = 2;
	    argv[0] = eb_uint2(pagebufp);

	    if (pagerest < 2) {
		eb_error = EB_ERR_UNEXP_START;
		return -1;
	    }

	    c2 = eb_uint1(pagebufp + 1);

	    if (0 <= skipcode) {
		/* nothing to be done. */
	    } else if (0x20 < c && c < 0x7f && 0x20 < c2 && c2 < 0x7f) {
		/*
		 * This is a JIS X 0208 KANJI character.
		 */
		*workbuf = (unsigned char) (c | 0x80);
		*(workbuf + 1) = (unsigned char) (c2 | 0x80);
		*(workbuf + 2) = '\0';
		if (narwflag) {
		    hook = hookset->hooks + EB_HOOK_NARROW_JISX0208;
		    if (hook->function != NULL && hook->function(book,
			appendix, workbuf, EB_HOOK_NARROW_JISX0208, 0, argv)
			< 0) {
			eb_error = EB_ERR_HOOK_WORKSPACE;
			return -1;
		    }
		} else {
		    hook = hookset->hooks + EB_HOOK_WIDE_JISX0208;
		    if (hook->function != NULL
			&& hook->function(book, appendix, workbuf,
			    EB_HOOK_WIDE_JISX0208, argc, argv) < 0) {
			eb_error = EB_ERR_HOOK_WORKSPACE;
			return -1;
		    }
		}
	    } else if (0x20 < c && c < 0x7f && 0xa0 < c2 && c2 < 0xff) {
		/*
		 * This is a GB 2312 HANJI character.
		 */
		*workbuf = (unsigned char) (c | 0x80);
		*(workbuf + 1) = (unsigned char) c2;
		*(workbuf + 2) = '\0';
		hook = hookset->hooks + EB_HOOK_GB2312;
		if (hook->function != NULL && hook->function(book, appendix,
		    workbuf, EB_HOOK_GB2312, 0, argv) < 0) {
		    eb_error = EB_ERR_HOOK_WORKSPACE;
		    return -1;
		}
	    } else {
		/*
		 * This is a local character.
		 */
		*workbuf = (unsigned char) c;
		*(workbuf + 1) = (unsigned char) c2;
		*(workbuf + 2) = '\0';
		if (narwflag) {
		    hook = hookset->hooks + EB_HOOK_NARROW_FONT;
		    if (hook->function != NULL && hook->function(book, 
			appendix, workbuf, EB_HOOK_NARROW_FONT, argc, argv)
			< 0) {
			eb_error = EB_ERR_HOOK_WORKSPACE;
			return -1;
		    }
		} else {
		    hook = hookset->hooks + EB_HOOK_WIDE_FONT;
		    if (hook->function != NULL && hook->function(book,
			appendix, workbuf, EB_HOOK_WIDE_FONT, argc, argv)
			< 0) {
			eb_error = EB_ERR_HOOK_WORKSPACE;
			return -1;
		    }
		}
	    }

	    if (skipcode < 0 && modhook->function != NULL
		&& modhook->function(book, appendix, workbuf, modhook->code,
		    0, argv) < 0) {
		eb_error = EB_ERR_HOOK_WORKSPACE;
		return -1;
	    }
	    if (skipcode < 0 && refhook->function != NULL
		&& refhook->function(book, appendix, workbuf, refhook->code,
		    0, argv) < 0) {
		eb_error = EB_ERR_HOOK_WORKSPACE;
		return -1;
	    }
	}
	pagebufp += step;
	pagerest -= step;

	worklen = strlen(workbuf);
	if (textrest < worklen)
	    goto at_end;

	strcpy(textp, workbuf);
	textp += worklen;
	textrest -= worklen;
	worklen = 0;
    }

    /*
     * At end.
     */
  at_end:
    *textp = '\0';
    return (textp - text);
}


