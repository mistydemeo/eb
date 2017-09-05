/*
 * Copyright (c) 2001, 02
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

#include "build-pre.h"
#include "eb.h"
#include "build-post.h"

#if defined(__STDC__) || defined(MSVC)
#include <stdarg.h>
#else
#include <varargs.h>
#endif


/*
 * Mutex.
 */
#ifdef ENABLE_PTHREAD
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Initialization flag.
 */
int eb_log_initialized = 0;

/*
 * Debug log flag.
 */
int eb_log_flag = 0;

/*
 * Pointer to log function.
 */
static void (*eb_log_function) EB_P((const char *, va_list)) = eb_log_stderr;


/*
 * Initialize logging sub-system.
 */
void
eb_initialize_log()
{
    if (eb_log_initialized)
	return;

    eb_log_flag = (getenv(EB_DEBUG_ENVIRONMENT_VARIABLE) != NULL);
    eb_log_function = eb_log_stderr;
    eb_log_initialized = 1;
}

/*
 * Set log function.
 */
void
eb_set_log_function(function)
    void (*function) EB_P((const char *, va_list));
{
    if (!eb_log_initialized)
	eb_initialize_log();
    eb_log_function = function;
}

/*
 * Enable logging.
 */
void
eb_enable_log()
{
    if (!eb_log_initialized)
	eb_initialize_log();
    eb_log_flag = 1;
}

/*
 * Disable logging.
 */
void
eb_disable_log()
{
    if (!eb_log_initialized)
	eb_initialize_log();
    eb_log_flag = 0;
}

/*
 * Log a message.
 */
#if defined(__STDC__) || defined(MSVC)
void
eb_log(const char *message, ...)
#else
void
eb_log(message, va_alist)
    const char *message;
    va_dcl 
#endif
{
    va_list ap;

#if defined(__STDC__) || defined(MSVC)
    va_start(ap, message);
#else
    va_start(ap);
#endif

    if (eb_log_flag && eb_log_function != NULL)
	eb_log_function(message, ap);

    va_end(ap);
}

/*
 * Output a log message to standard error.
 * This is the default log handler.
 *
 * Currently, this function doesn't work if the system lacks vprintf()
 * and dopront().
 */
void
eb_log_stderr(message, ap)
    const char *message;
    va_list ap;
{
    pthread_mutex_lock(&log_mutex);

    fputs("[EB] ", stderr);

#if defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)
    vfprintf(stderr, message, ap);
#else
    fprintf(stderr, "eb_log_stderr() doesn't work on the system.");
#endif
    fputc('\n', stderr);
    fflush(stderr);

    pthread_mutex_unlock(&log_mutex);
}

#define MAX_QUOTED_STREAM_LENGTH	100

/*
 * Return Quoted printable string of `stream'.
 */
const char *
eb_quoted_stream(stream, stream_length)
    const char *stream;
    size_t stream_length;
{
    static char quoted_streams[EB_MAX_KEYWORDS][MAX_QUOTED_STREAM_LENGTH + 3];
    static int current_index = 0;
    unsigned char *quoted_p;
    const unsigned char *stream_p;
    size_t quoted_length = 0;
    int i;

    current_index = (current_index + 1) % EB_MAX_KEYWORDS;
    quoted_p = (unsigned char *)quoted_streams[current_index];
    stream_p = (const unsigned char *)stream;

    if (stream == NULL)
	return "";

    for (i = 0; i < stream_length && *stream_p != '\0'; i++) {
	if (0x20 <= *stream_p && *stream_p <= 0x7f && *stream_p != '=') {
	    if (MAX_QUOTED_STREAM_LENGTH < quoted_length + 1) {
		*quoted_p++ = '.';
		*quoted_p++ = '.';
		break;
	    }
	    *quoted_p++ = *stream_p;
	    quoted_length++;
	} else {
	    if (MAX_QUOTED_STREAM_LENGTH < quoted_length + 3) {
		*quoted_p++ = '.';
		*quoted_p++ = '.';
		break;
	    }
	    *quoted_p++ = '=';
	    *quoted_p++ = "0123456789ABCDEF" [*stream_p / 0x10];
	    *quoted_p++ = "0123456789ABCDEF" [*stream_p % 0x10];
	    quoted_length += 3;
	}
	stream_p++;
    }

    *quoted_p = '\0';
    return quoted_streams[current_index];
}


/*
 * Return Quoted printable string.
 */
const char *
eb_quoted_string(string)
    const char *string;
{
    return eb_quoted_stream(string, strlen(string));
}
