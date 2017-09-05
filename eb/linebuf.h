/*
 * Copyright (c) 1997, 2000, 01, 02
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

#ifndef LINEBUF_H
#define LINEBUF_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

/*
 * Buffer size of `Line_Buffer' struct.
 */
#define LINEBUF_BUFFER_SIZE	256

/*
 * Line buffer manager.
 */
typedef struct {
    int file;				/* file descriptor */
    int timeout;			/* idle timeout interval */
    size_t cache_length;		/* length of cache data */
    char buffer[LINEBUF_BUFFER_SIZE];	/* buffer */
} Line_Buffer;
 

/*
 * Function declarations.
 */
#ifdef __STDC__
void initialize_line_buffer(Line_Buffer *);
void finalize_line_buffer(Line_Buffer *);
void set_line_buffer_timeout(Line_Buffer *, int);
void bind_file_to_line_buffer(Line_Buffer *, int);
int file_bound_to_line_buffer(Line_Buffer *);
void discard_cache_in_line_buffer(Line_Buffer *);
size_t cache_length_in_line_buffer(Line_Buffer *);
ssize_t read_line_buffer(Line_Buffer *, char *, size_t);
ssize_t binary_read_line_buffer(Line_Buffer *, char *, size_t);
int skip_line_buffer(Line_Buffer *);
#else
void initialize_line_buffer();
void finalize_line_buffer();
void set_line_buffer_timeout();
void bind_file_to_line_buffer();
int file_bound_to_line_buffer();
void discard_cache_in_line_buffer();
ssize_t read_line_buffer();
ssize_t binary_read_line_buffer();
int skip_line_buffer();
#endif

#endif /* not LINEBUF_H */
