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

#ifndef FAKELOG_H
#define FAKELOG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/*
 * Log modes.
 */
#define FAKELOG_TO_NOWHERE	0
#define FAKELOG_TO_SYSLOG	1
#define FAKELOG_TO_STDERR	2
#define FAKELOG_TO_BOTH		3

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
 * Maximum length of log-name.
 */
#define FAKELOG_MAX_LOG_NAME_LENGTH	64

/*
 * Fake syslog().
 */
#ifdef syslog
#undef syslog
#endif
#define syslog fakelog


/*
 * Function declarations.
 */
#ifdef __STDC__

void set_fakelog_name(const char *);
void set_fakelog_mode(int);
void set_fakelog_level(int);

#if (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && defined(HAVE_VSYSLOG)
void fakelog(int, const char *, ...);
#else /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */
void fakelog();
#endif /* not (defined(HAVE_VPRINTF) || defined(HAVE_DOPRNT)) && ... */

#else /* not __STDC__ */

void set_fakelog_name();
void set_fakelog_mode();
void set_fakelog_level();
void fakelog();

#endif  /* not __STDC__ */

#endif /* not FAKELOG_H */
