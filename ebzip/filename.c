/*                                                            -*- C -*-
 * Copyright (c) 1999, 2000, 01  
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

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

/*
 * The maximum length of path name.
 */
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

/*
 * Trick for difference of path notation between UNIX and DOS.
 */
#ifndef DOS_FILE_PATH
#define F_(path1, path2) (path1)
#else
#define F_(path1, path2) (path2)
#endif

#include "eb/eb.h"

void
fix_path_name_suffix(path_name, suffix)
    char *path_name;
    const char *suffix;
{
    char *base_name;
    char *dot;
    char *semicolon;

    base_name = strrchr(path_name, F_('/', '\\'));
    if (base_name == NULL)
	base_name = path_name;
    else
	base_name++;

    dot = strchr(base_name, '.');
    semicolon = strchr(base_name, ';');

    if (*suffix == '\0') {
	/*
	 * Remove `.ebz' from `fixed_file_name':
	 *   foo.ebz    -->  foo
	 *   foo.ebz;1  -->  foo;1
	 *   foo.       -->  foo.     (unchanged)
	 *   foo.;1     -->  foo.;1   (unchanged)
	 */
	if (dot != NULL && *(dot + 1) != '\0' && *(dot + 1) != ';') {
	    if (semicolon != NULL)
		sprintf(dot, ";%c", *(semicolon + 1));
	    else
		*dot = '\0';
	}
    } else {
	/*
	 * Add `.ebz' to `fixed_file_name':
	 *   foo       -->  foo.ebz
	 *   foo.      -->  foo.ebz
	 *   foo;1     -->  foo.ebz;1
	 *   foo.;1    -->  foo.ebz;1
	 *   foo.ebz   -->  foo.ebz    (unchanged)
	 */
	if (dot != NULL) {
	    if (semicolon != NULL)
		sprintf(dot, "%s;%c", suffix, *(semicolon + 1));
	    else
		strcpy(dot, suffix);
	} else {
	    if (semicolon != NULL)
		sprintf(semicolon, "%s;%c", suffix, *(semicolon + 1));
	    else
		strcat(base_name, suffix);
	}
    }
}
