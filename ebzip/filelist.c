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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>
#endif

/*
 * Tricks for gettext.
 */
#define _(string) gettext(string)
#ifdef gettext_noop
#define N_(string) gettext_noop(string)
#else
#define N_(string) (string)
#endif

extern const char *invoked_name;

/*
 * Initialize a file name list.
 */
void
initialize_file_name_list(file_name_list, length)
    const char **file_name_list;
    int length;
{
    const char **list_p;
    int i;

    for (i = 0, list_p = file_name_list; i < length; i++, list_p++)
	*list_p = NULL;
    *list_p = NULL;
}


/*
 * Clear a file name list.
 */
void
clear_file_name_list(file_name_list)
    const char **file_name_list;
{
    const char **list_p;

    for (list_p = file_name_list; *list_p != NULL; list_p++) {
	free((char *)*list_p);
	*list_p = NULL;
    }
}


/*
 * Add a file name to a list.
 */
void
add_file_name_list(file_name_list, file_name)
    const char **file_name_list;
    char *file_name;
{
    const char **list_p;
    char *new_entry;

    for (list_p = file_name_list; *list_p != NULL; list_p++)
	;

    new_entry = (char *) malloc(strlen(file_name) + 1);
    if (new_entry == NULL) {
	fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	exit(1);
    }
    strcpy(new_entry, file_name);

    *list_p = new_entry;
}

