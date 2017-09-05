/*
 * Copyright (c) 1997, 1998  Motoyuki Kasahara
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
 * Requirements for Autoconf:
 *   AC_C_CONST
 *   AC_TYPE_SIZE_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h)
 *   AC_CHECK_FUNCS(vsyslog, strerror)
 *   AC_FUNC_VPRINTF
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <syslog.h>

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
 * Log priorities.
 */
#define FAKELOG_QUIET		0
#define FAKELOG_EMERG		1
#define FAKELOG_ALERT		2
#define FAKELOG_CRIT		3
#define FAKELOG_ERR		4
#define FAKELOG_WARNING		5
#define FAKELOG_NOTICE		6
#define FAKELOG_INFO		7
#define FAKELOG_DEBUG		8
#define FAKELOG_UNKNOWN		9


/*
 * Log name, mode and level.
 */
static char *logname = NULL;
static char logname_buffer[FAKELOG_MAXLEN_LOGNAME + 1];
static int logmode = FAKELOG_TO_SYSLOG;
static int loglevel = FAKELOG_ERR;

/*
 * Set log name.
 */
void
set_fakelog_name(name)
    const char *name;
{
    if (name == NULL)
        logname = NULL;
    else {
        strncpy(logname_buffer, name, FAKELOG_MAXLEN_LOGNAME);
        logname_buffer[FAKELOG_MAXLEN_LOGNAME] = '\0';
        logname = logname_buffer;
    }
}


/*
 * Set log level.
 */
void
set_fakelog_mode(mode)
    int mode;
{
    logmode = mode;
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
#ifdef LOG_EMERG
    case LOG_EMERG:
	loglevel = FAKELOG_EMERG;
	break;
#endif
#ifdef LOG_ALERT
    case LOG_ALERT:
	loglevel = FAKELOG_ALERT;
	break;
#endif
#ifdef LOG_CRIT
    case LOG_CRIT:
	loglevel = FAKELOG_CRIT;
	break;
#endif
#ifdef LOG_ERR
    case LOG_ERR:
	loglevel = FAKELOG_ERR;
	break;
#endif
#ifdef LOG_WARNING
    case LOG_WARNING:
	loglevel = FAKELOG_WARNING;
	break;
#endif
#ifdef LOG_NOTICE
    case LOG_NOTICE:
	loglevel = FAKELOG_NOTICE;
	break;
#endif
#ifdef LOG_INFO
    case LOG_INFO:
	loglevel = FAKELOG_INFO;
	break;
#endif
#ifdef LOG_DEBUG
    case LOG_DEBUG:
	loglevel = FAKELOG_DEBUG;
	break;
#endif
    default:
	loglevel = FAKELOG_QUIET;
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
#define MAXLEN_EXPANDED    1023

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
    const char *msg;
    char expanded[MAXLEN_EXPANDED + 1];
    char *exp;
    size_t exprest;
    const char *errstr;
    size_t errlen;
    int logflag;

    /*
     * Convert `priority'.
     */
    switch (priority) {
#ifdef LOG_EMERG
    case LOG_EMERG:
	logflag = (FAKELOG_EMERG <= loglevel);
	break;
#endif
#ifdef LOG_ALERT
    case LOG_ALERT:
	logflag = (FAKELOG_ALERT <= loglevel);
	break;
#endif
#ifdef LOG_CRIT
    case LOG_CRIT:
	logflag = (FAKELOG_CRIT <= loglevel);
	break;
#endif
#ifdef LOG_ERR
    case LOG_ERR:
	logflag = (FAKELOG_ERR <= loglevel);
	break;
#endif
#ifdef LOG_WARNING
    case LOG_WARNING:
	logflag = (FAKELOG_WARNING <= loglevel);
	break;
#endif
#ifdef LOG_NOTICE
    case LOG_NOTICE:
	logflag = (FAKELOG_NOTICE <= loglevel);
	break;
#endif
#ifdef LOG_INFO
    case LOG_INFO:
	logflag = (FAKELOG_INFO <= loglevel);
	break;
#endif
#ifdef LOG_DEBUG
    case LOG_DEBUG:
	logflag = (FAKELOG_DEBUG <= loglevel);
	break;
#endif
    default:
	logflag = 0;
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
    if (logmode == FAKELOG_TO_SYSLOG || logmode == FAKELOG_TO_BOTH) {
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
	vsyslog(priority, message, ap);
#else
	syslog(priority, message, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
    }

    /*
     * Output the message to standard error.
     */
    if ((logmode == FAKELOG_TO_STDERR || logmode == FAKELOG_TO_BOTH)
	&& logflag) {
	/*
	 * Output a command name.
	 */
	if (logname != NULL)
	    fprintf(stderr, "%s: ", logname);

	/*
	 * Expand `%m' in `message', and copied into `expanded'.
	 */
	msg = message;
	exp = expanded;
	exprest = MAXLEN_EXPANDED;
	errstr = strerror(errno);
	errlen = strlen(errstr);

	while (*msg != '\0') {
	    if (*msg == '%') {
		if (*(msg + 1) == 'm') {
		    if (exprest < errlen)
			break;
		    strcpy(exp, errstr);
		    exp += errlen;
		    msg += 2;
		    exprest -= errlen;
		} else  {
		    if (exprest < 2 || *(msg + 1) == '\0')
			break;
		    *exp++ = *msg++;
		    *exp++ = *msg++;
		}
	    } else {
		if (exprest < 1)
		    break;
		*exp++ = *msg++;
	    }
	}
	*exp = '\0';
	
	/*
	 * If `expanded' overflows, give up logging.
	 */
	if (*msg != '\0')
	    return;

	/*
	 * Terminate `expanded'.
	 */
	*exp = '\0';

	/*
	 * Print the message.
	 */
#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
#ifdef HAVE_VPRINTF
	vfprintf(stderr, expanded, ap);
#else /* not HAVE_VPRINTF */
#ifdef HAVE_DOPRNT
	_doprnt(expanded, &ap, stderr);
#endif /* not HAVE_DOPRNT */
#endif /* not HAVE_VPRINTF */
#else /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
	fprintf(stderr, expanded, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
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


