/*                                                            -*- C -*-
 * Copyright (c) 1998, 2000  Motoyuki Kasahara
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

#include <zlib.h>

#include "eb/eb.h"

/*
 * Compress a slice with the ebzip compression format.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
int
ebzip1_slice(out_buffer, out_byte_length, in_buffer, in_byte_length)
    char *out_buffer;
    size_t *out_byte_length;
    char *in_buffer;
    size_t in_byte_length;
{
    z_stream stream;

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK)
	return -1;
    
    stream.next_in = (Bytef *) in_buffer;
    stream.avail_in = in_byte_length;
    stream.next_out = (Bytef *) out_buffer;
    stream.avail_out = in_byte_length + EB_SIZE_EBZIP_MARGIN;

    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
	*out_byte_length = in_byte_length;
	return 0;
    }

    if (deflateEnd(&stream) != Z_OK)
	return -1;

    *out_byte_length = stream.total_out;
    return 0;
}


