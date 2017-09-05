/*                                                            -*- C -*-
 * Copyright (c) 2000, 01  
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

#ifndef EB_LIBINTL_R_H
#define EB_LIBINTL_R_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/* intl_r.c */
#if defined(__STDC__) || defined(__cplusplus)
char *eb_gettext_r(const char *, char *, size_t);
char *eb_dgettext_r(const char *, const char *, char *, size_t);
char *eb_dcgettext_r(const char *, const char *, int, char *, size_t);
char *eb_textdomain_r(const char *);
char *eb_bindtextdomain_r(const char *, const char *);
#else
char *eb_gettext_r();
char *eb_dgettext_r();
char *eb_dcgettext_r();
char *eb_textdomain_r();
char *eb_bindtextdomain_r();
#endif

#ifdef __cplusplus
}
#endif

#endif /* not EB_LIBINTL_R_H */
