/*                                                            -*- C -*-
 * Copyright (c) 2001  
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

#ifndef EB_BINARY_H
#define EB_BINARY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifdef EB_BUILD_LIBRARY
#include "defs.h"
#else
#include <eb/defs.h>
#endif

/*
 * Function declarations.
 */

/* binary.c */
EB_Error_Code eb_set_binary_mono_graphic EB_P((EB_Book *, const EB_Position *,
   int, int));
EB_Error_Code eb_set_binary_gray_graphic EB_P((EB_Book *, const EB_Position *,
    int, int));
EB_Error_Code eb_set_binary_wave EB_P((EB_Book *, const EB_Position *,
    const EB_Position *));
EB_Error_Code eb_set_binary_color_graphic EB_P((EB_Book *,
    const EB_Position *));
EB_Error_Code eb_set_binary_mpeg EB_P((EB_Book *, const unsigned int *));
EB_Error_Code eb_read_binary EB_P((EB_Book *, size_t, char *, ssize_t *));
void eb_unset_binary EB_P((EB_Book *));

/* filename.c */
EB_Error_Code eb_compose_movie_file_name EB_P((const unsigned int *,
    char *));
EB_Error_Code eb_decompose_movie_file_name EB_P((unsigned int *,
    const char *));

#ifdef __cplusplus
}
#endif

#endif /* not EB_BINARY_H */
