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

/*
 * This program requires the following Autoconf macros:
 *   AC_C_CONST
 *   AC_TYPE_SIZE_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, syslog.h)
 *   AC_CHECK_FUNCS(vsyslog, strerror, syslog)
 *   AC_FUNC_VPRINTF
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef __STDC__
#include <stdarg.h>
#else /* not __STDC__ */
#include <varargs.h>
#endif /* not __STDC__ */
#endif /* (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */

#ifndef HAVE_STRERROR
#ifdef __STDC__
char *strerror(int);
#else /* not __STDC__ */
char *strerror();
#endif /* not __STDC__ */
#endif /* HAVE_STRERROR */

#include "fakelog.h"

#undef syslog


/*
 * Log name, mode and level.
 */
static char *log_name = NULL;
static char log_name_buffer[FAKELOG_MAX_LOG_NAME_LENGTH + 1];
static int log_mode = FAKELOG_TO_SYSLOG;
static int log_level = FAKELOG_ERR;

/*
 * Set log name.
 */
void
set_fakelog_name(name)
    const char *name;
{
    if (name == NULL)
        log_name = NULL;
    else {
        strncpy(log_name_buffer, name, FAKELOG_MAX_LOG_NAME_LENGTH);
        log_name_buffer[FAKELOG_MAX_LOG_NAME_LENGTH] = '\0';
        log_name = log_name_buffer;
    }
}


/*
 * Set log level.
 */
void
set_fakelog_mode(mode)
    int mode;
{
    log_mode = mode;
}


/*
 * Set log level.
 */
void
set_fakelog_level(level)
    int level;
{
    /*
     * Convert a syslog priority to a fakelog priority.
     */
    switch (level) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
    case LOG_WARNING:
    case LOG_NOTICE:
    case LOG_INFO:
    case LOG_DEBUG:
	log_level = level;
	break;
    default:
	log_level = FAKELOG_QUIET;
	break;
    }
}


/*
 * The function fakes syslog(), and output a message to standard 
 * error, instead of syslog.
 *
 * To use the function, you can fake your sources by the following
 * trick:
 *
 *     #define syslog fakelog
 *
 * The mark `%m' in the message is expanded to a corresponding error
 * message, but the expanded message must not exceed  MAXLEN_EXPANDED
 * characters.
 */
#define BUFFER_SIZE    1024

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef __STDC__
void
fakelog(int priority, const char *message, ...)
#else /* not __STDC__ */
void
fakelog(priority, message, va_alist)
    int priority;
    char *message;
    va_dcl 
#endif /* not __STDC__ */
#else /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
void
fakelog(priority, message, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)
    int priority;
    char *message;
    char *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
#endif /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
{

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
    va_list ap;
#endif
    const char *message_p;
    char buffer[BUFFER_SIZE];
    char *buffer_p;
    size_t buffer_rest_length;
    const char *error_string;
    size_t error_string_length;
    int log_flag;

    /*
     * Convert `priority'.
     */
    switch (priority) {
#ifdef LOG_EMERG
    case LOG_EMERG:
	log_flag = (FAKELOG_EMERG <= log_level);
	break;
#endif
#ifdef LOG_ALERT
    case LOG_ALERT:
	log_flag = (FAKELOG_ALERT <= log_level);
	break;
#endif
#ifdef LOG_CRIT
    case LOG_CRIT:
	log_flag = (FAKELOG_CRIT <= log_level);
	break;
#endif
#ifdef LOG_ERR
    case LOG_ERR:
	log_flag = (FAKELOG_ERR <= log_level);
	break;
#endif
#ifdef LOG_WARNING
    case LOG_WARNING:
	log_flag = (FAKELOG_WARNING <= log_level);
	break;
#endif
#ifdef LOG_NOTICE
    case LOG_NOTICE:
	log_flag = (FAKELOG_NOTICE <= log_level);
	break;
#endif
#ifdef LOG_INFO
    case LOG_INFO:
	log_flag = (FAKELOG_INFO <= log_level);
	break;
#endif
#ifdef LOG_DEBUG
    case LOG_DEBUG:
	log_flag = (FAKELOG_DEBUG <= log_level);
	break;
#endif
    default:
	log_flag = 0;
    }

    /*
     * Start to use the variable length arguments.
     */
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef __STDC__
    va_start(ap, message);
#else /* not __STDC__ */
    va_start(ap);
#endif /* not __STDC__ */
#endif /* (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */

    /*
     * Output the message to syslog.
     */
    if (log_mode == FAKELOG_TO_SYSLOG || log_mode == FAKELOG_TO_BOTH) {
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
	vsyslog(priority, message, ap);
#else /* not (defined(HAVE_VPRINTF) || ... */
#ifdef HAVE_SYSLOG
	syslog(priority, message, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif /* HAVE_SYSLOG */
#endif /* not (defined(HAVE_VPRINTF) || ... */
    }

    /*
     * Output the message to standard error.
     */
    if ((log_mode == FAKELOG_TO_STDERR || log_mode == FAKELOG_TO_BOTH)
	&& log_flag) {
	/*
	 * Output a command name.
	 */
	if (log_name != NULL)
	    fprintf(stderr, "%s: ", log_name);

	/*
	 * Expand `%m' in `message', and copied into `buffer'.
	 */
	message_p = message;
	buffer_p = buffer;
	buffer_rest_length = BUFFER_SIZE - 1;
	error_string = strerror(errno);
	error_string_length = strlen(error_string);

	while (*message_p != '\0') {
	    if (*message_p == '%') {
		if (*(message_p + 1) == 'm') {
		    if (buffer_rest_length < error_string_length)
			break;
		    strcpy(buffer_p, error_string);
		    buffer_p += error_string_length;
		    message_p += 2;
		    buffer_rest_length -= error_string_length;
		} else  {
		    if (buffer_rest_length < 2 || *(message_p + 1) == '\0')
			break;
		    *buffer_p++ = *message_p++;
		    *buffer_p++ = *message_p++;
		}
	    } else {
		if (buffer_rest_length < 1)
		    break;
		*buffer_p++ = *message_p++;
	    }
	}
	*buffer_p = '\0';
	
	/*
	 * If `buffer' overflows, give up logging.
	 */
	if (*message_p != '\0')
	    return;

	/*
	 * Terminate `buffer'.
	 */
	*buffer_p = '\0';

	/*
	 * Print the message.
	 */
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef HAVE_VPRINTF
	vfprintf(stderr, buffer, ap);
#else /* not HAVE_VPRINTF */
#ifdef HAVE_DOPRNT
	_doprnt(buffer, &ap, stderr);
#endif /* not HAVE_DOPRNT */
#endif /* not HAVE_VPRINTF */
#else /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
	fprintf(stderr, buffer, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */

	fputc('\n', stderr);
	fflush(stderr);
    }

    /*
     * End to use the variable length arguments.
     */
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
    va_end(ap);
#endif /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
}


