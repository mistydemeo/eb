/*                                                            -*- C -*-
 * Copyright (c) 1999, 2000  Motoyuki Kasahara
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

#include "eb/eb.h"

void
compose_out_path_name(path_name, file_name, suffix, out_path_name)
    const char *path_name;
    const char *file_name;
    const char *suffix;
    char *out_path_name;
{
    char fixed_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char *dot;
    char *semicolon;

    strcpy(fixed_file_name, file_name);
    dot = strchr(fixed_file_name, '.');
    semicolon = strchr(fixed_file_name, ';');

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
		strcpy(dot, ";1");
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
	if (dot == NULL) {
	    if (semicolon != NULL)
		strcpy(semicolon, ".ebz;1");
	    else
		strcat(fixed_file_name, ".ebz");
	} else if (*(dot + 1) == '\0' || *(dot + 1) == ';') {
	    if (semicolon != NULL)
		strcpy(dot, ".ebz;1");
	    else
		strcpy(dot, ".ebz");
	}
    }

    sprintf(out_path_name, "%s/%s", path_name, fixed_file_name);
}

void
compose_out_path_name2(path_name, sub_directory_name, file_name, suffix,
    out_path_name)
    const char *path_name;
    const char *sub_directory_name;
    const char *file_name;
    const char *suffix;
    char *out_path_name;
{
    char sub_path_name[PATH_MAX + 1];
    sprintf(sub_path_name, "%s/%s", path_name, sub_directory_name);
    compose_out_path_name(sub_path_name, file_name, suffix, out_path_name);
}

void
compose_out_path_name3(path_name, sub_directory_name, sub2_directory_name,
    file_name, suffix, out_path_name)
    const char *path_name;
    const char *sub_directory_name;
    const char *sub2_directory_name;
    const char *file_name;
    const char *suffix;
    char *out_path_name;
{
    char sub2_path_name[PATH_MAX + 1];
    sprintf(sub2_path_name, "%s/%s/%s", path_name, sub_directory_name,
	sub2_directory_name);
    compose_out_path_name(sub2_path_name, file_name, suffix, out_path_name);
}

