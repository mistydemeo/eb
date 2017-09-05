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
 *   AC_TYPE_MODE_T
 *   AC_HEADER_STDC
 *   AC_CHECK_HEADERS(string.h, memory.h, stdlib.h, limits.h)
 *   AC_CHECK_FUNCS(strchr)
 *   AC_HEADER_STAT
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

/*
 * The maximum length of a filename.
 */
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

#ifdef  STAT_MACROS_BROKEN
#ifdef  S_ISREG
#undef  S_ISREG
#endif
#ifdef  S_ISDIR
#undef  S_ISDIR
#endif
#endif  /* STAT_MACROS_BROKEN */

#ifndef S_ISREG
#define S_ISREG(m)   (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)   (((m) & S_IFMT) == S_IFDIR)
#endif

#ifdef USE_FAKELOG
#include "fakelog.h"
#endif


/*
 * Make a directory if the directory doesn't exist.
 *
 * If making the directory is succeeded or the directory has already 
 * created, 0 is returned.  Otherwise -1 is returned.
 */
int
make_missing_directory(path, mode)
    const char *path;
    mode_t mode;
{
    struct stat st;

    if (stat(path, &st) < 0) {
	if (mkdir(path, mode) < 0) {
	    syslog(LOG_ERR, "cannot make the directory, %m: %s", path);
	    return -1;
	}
	syslog(LOG_DEBUG, "debug: make the directory: %s", path);
    } else if (!S_ISDIR(st.st_mode)) {
	syslog(LOG_ERR, "already exists, but not a directory: %s", path);
	return -1;
    } else {
	syslog(LOG_DEBUG, "debug: the directory already exists: %s\n", path);
    }

    return 0;
}


/*
 * Make a directory if the directory doesn't exist.
 * Intermediate directories are also created if required.
 *
 * If making the directory is succeeded or the directory has already 
 * created, 0 is returned.  Otherwise -1 is returned.
 */
int
make_missing_directory_chain(path, mode)
    const char *path;
    mode_t mode;
{
    char tmppath[PATH_MAX + 1];
    char *p;

    if (PATH_MAX < strlen(path)) {
	strncpy(tmppath, path, PATH_MAX);
	*(tmppath + PATH_MAX) = '\0';
	syslog(LOG_ERR, "too long path: %s...", tmppath);
	return -1;
    }

    strcpy(tmppath, path);
    for (p = strchr(tmppath, '/'); p != NULL; p = strchr(p + 1, '/')) {
	*p = '\0';
	if (make_missing_directory(tmppath, mode) < 0)
	    return -1;
	*p = '/';
    }

    if (make_missing_directory(tmppath, mode) < 0)
	return -1;

    return 0;
}


