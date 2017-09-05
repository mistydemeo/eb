/*                                                            -*- C -*-
 * Copyright (c) 2003
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#ifdef HAVE_LIBCHARSET_H
#include <libcharset.h>
#else
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#endif /* HAVE_LIBCHARSET_H */

/*
 * Convert `string' from EUC-JP to the current locale encoding, and
 * then write it to `stream'.
 */
int
fputs_eucjp_to_locale(string, stream)
    const char *string;
    FILE *stream;
{
#if defined(HAVE_ICONV_OPEN) || defined(HAVE_LIBICONV_OPEN)
    size_t string_length;
    const char *locale_encoding;
    char *buffer = NULL;
    size_t buffer_size;
    iconv_t cd = (iconv_t)-1;
    const char *in_p;
    char *out_p;
    size_t in_left;
    size_t out_left;
    int fputs_result;

    string_length = strlen(string);

#ifdef HAVE_LOCALE_CHARSET
    locale_encoding = locale_charset();
#else
#if defined(HAVE_NL_LANGINFO) && defined(CODESET)
    locale_encoding = nl_langinfo(CODESET);
#else
    locale_encoding = NULL;
#endif
#endif /* HAVE_LOCALE_CHARSET */
    if (locale_encoding == NULL)
	goto failed;
    cd = iconv_open(locale_encoding, "eucJP");
    if (cd == (iconv_t)-1)
	cd = iconv_open(locale_encoding, "EUC-JP");
    if (cd == (iconv_t)-1)
	goto failed;

    buffer_size = (string_length + 1) * 2;

    for (;;) {
	buffer = malloc(buffer_size);
	if (buffer == NULL)
	    goto failed;

	in_p = string;
	in_left = string_length + 1;
	out_p = buffer;
	out_left = buffer_size;

	if (iconv(cd, &in_p, &in_left, &out_p, &out_left) != -1)
	    break;
	if (errno == E2BIG) {
	    /*
	     * Reset initial state.
	     * To avoid a bug of iconv() on Solaris 2.6, we set `in_left',
	     * `out_p' and `out_left' to non-NULL values.
	     */
	    in_left = 0;
	    out_p = buffer;
	    out_left = 0;
	    iconv(cd, NULL, &in_left, &out_p, &out_left);
	    
	    free(buffer);
	    buffer = NULL;
	    buffer_size += string_length + 1;

	    continue;
	} else {
	    goto failed;
	}
    }

    iconv_close(cd);
    fputs_result = fputs(buffer, stream);
    free(buffer);

    return fputs_result;

    /*
     * An error occurs...
     */
  failed:
    if (cd != (iconv_t)-1)
	iconv_close(cd);
    if (buffer != NULL)
	free(buffer);
    return fputs(string, stream);

#else /* not HAVE_ICONV_OPEN && not HAVE_LIBICONV_OPEN */
    return fputs(string, stream);
#endif /* not HAVE_ICONV_OPEN && not HAVE_LIBICONV_OPEN */
}


/*
 * Convert `string' from EUC-JP to the current locale encoding, and
 * then write it and a newline to `stdout'.  
 */
int
puts_eucjp_to_locale(string)
    const char *string;
{
    if (fputs_eucjp_to_locale(string, stdout) == EOF)
	return EOF;
    if (fputs_eucjp_to_locale("\n", stdout) == EOF)
	return EOF;

    return 0;
}
