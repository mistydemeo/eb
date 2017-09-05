/*                                                            -*- C -*-
 * Copyright (c) 1997, 98, 99, 2000, 01
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

#ifndef EB_BOOKLIST_H
#define EB_BOOKLIST_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EB_BUILD_LIBRARY
#include "eb.h"
#else
#include <eb/eb.h>
#endif

/*
 * Function declarations.
 */
/* booklist.c */
void eb_initialize_booklist EB_P((EB_BookList *));
void eb_finalize_booklist EB_P((EB_BookList *));
EB_Error_Code eb_bind_booklist EB_P((EB_BookList *, const char *));
EB_Error_Code eb_booklist_book_count EB_P((EB_BookList *, int *));
EB_Error_Code eb_booklist_book_name EB_P((EB_BookList *, int, char **));
EB_Error_Code eb_booklist_book_title EB_P((EB_BookList *, int, char **));


#ifdef __cplusplus
}
#endif

#endif /* not EB_BOOKLIST_H */
