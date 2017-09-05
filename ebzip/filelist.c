/*                                                            -*- C -*-
 * Copyright (c) 1999  Motoyuki Kasahara
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

extern const char *invoked_name;

/*
 * Initialize a filename list.
 */
void
initialize_filename_list(filename_list, length)
    const char **filename_list;
    int length;
{
    const char **f;
    int i;

    for (i = 0, f = filename_list; i < length; i++, f++)
	*f = NULL;
    *f = NULL;
}


/*
 * Clear a filename list.
 */
void
clear_filename_list(filename_list)
    const char **filename_list;
{
    const char **f;

    for (f = filename_list; *f != NULL; f++) {
	free((char *)*f);
	*f = NULL;
    }
}


/*
 * Add a filename to a list.
 */
void
add_filename_list(filename_list, filename)
    const char **filename_list;
    char *filename;
{
    const char **f;
    char *mem;

    for (f = filename_list; *f != NULL; f++)
	;

    mem = (char *) malloc(strlen(filename) + 1);
    if (mem == NULL) {
	fprintf(stderr, "%s: memory exhausted\n", invoked_name);
	exit(1);
    }
    strcpy(mem, filename);

    *f = mem;
}

