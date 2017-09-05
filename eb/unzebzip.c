/*                                                            -*- C -*-
 * Copyright (c) 1998, 99, 2000  Motoyuki Kasahara
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

#include "eb.h"
#include "error.h"
#include "internal.h"

/*
 * Uncompress an ebzip'ped slice.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
int
eb_unzip_slice_ebzip1(out_buffer, out_byte_length, in_buffer, in_byte_length)
    char *out_buffer;
    size_t out_byte_length;
    char *in_buffer;
    size_t in_byte_length;
{
    z_stream stream;

    stream.zalloc = NULL;
    stream.zfree = NULL;
    stream.opaque = NULL;

    if (inflateInit(&stream) != Z_OK)
	return -1;
    
    stream.next_in = (Bytef *) in_buffer;
    stream.avail_in = in_byte_length;
    stream.next_out = (Bytef *) out_buffer;
    stream.avail_out = out_byte_length;

    if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
	return -1;

    if (inflateEnd(&stream) != Z_OK)
	return -1;

    if (stream.total_out != out_byte_length)
	return -1;

    return 0;
}


