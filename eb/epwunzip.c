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

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef __STDC__
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ */
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"

/*
 * Uncompress an EPWING compressed slice.
 * The offset of `in_file' must points to the beginning of the compressed
 * slice.  Uncompressed data are put into `out_buffer'.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
int
eb_epwunzip_slice(out_buffer, in_file, huffman_tree)
    char *out_buffer;
    int in_file;
    EB_Huffman_Node *huffman_tree;
{
    EB_Huffman_Node *node_p;
    int bit;
    char in_buffer[EB_SIZE_PAGE];
    unsigned char *in_buffer_p;
    size_t in_read_length;
    int in_bit_index;
    unsigned char *out_buffer_p;
    size_t out_length;

    in_buffer_p = (unsigned char *)in_buffer;
    in_bit_index = 7;
    in_read_length = 0;
    out_buffer_p = (unsigned char *)out_buffer;
    out_length = 0;

    for (;;) {
	/*
	 * Descend the huffman tree until reached to the leaf node.
	 */
	node_p = huffman_tree;
	while (node_p->type == EB_HUFFMAN_NODE_INTERMEDIATE) {

	    /*
	     * If no data is left in the input buffer, read next chunk.
	     */
	    if ((unsigned char *)in_buffer + in_read_length <= in_buffer_p) {
		in_read_length = eb_read_all(in_file, in_buffer, EB_SIZE_PAGE);
		if (in_read_length <= 0)
		    return -1;
		in_buffer_p = (unsigned char *)in_buffer;
	    }

	    /*
	     * Step to a child.
	     */
	    bit = (*in_buffer_p >> in_bit_index) & 0x01;

	    if (bit == 1)
		node_p = node_p->left;
	    else
		node_p = node_p->right;
	    if (node_p == NULL)
		return -1;

	    if (0 < in_bit_index)
		in_bit_index--;
	    else {
		in_bit_index = 7;
		in_buffer_p++;
	    }
	}

	if (node_p->type == EB_HUFFMAN_NODE_EOF) {
	    /*
	     * Fill the rest of the output buffer with NUL,
             * when we meet an EOF mark before decode EB_SIZE_PAGE bytes.
	     */
	    if (out_length < EB_SIZE_PAGE) {
#ifdef HAVE_MEMCPY
		memset(out_buffer_p, EB_SIZE_PAGE - out_length, '\0');
#else
		bzero(out_buffer_p, EB_SIZE_PAGE - out_length);
#endif
	    }
	    return out_length;
	} else if (node_p->type == EB_HUFFMAN_NODE_LEAF16) {
	    /*
	     * The leaf is leaf16, decode 2 bytes character.
	     */
	    if (EB_SIZE_PAGE <= out_length)
		return -1;
	    else if (EB_SIZE_PAGE <= out_length + 1) {
		*out_buffer_p++ = (node_p->value >> 8) & 0xff;
		out_length++;
	    } else {
		*out_buffer_p++ = (node_p->value >> 8) & 0xff;
		*out_buffer_p++ = node_p->value & 0xff;
		out_length += 2;
	    }
	} else {
	    /*
	     * The leaf is leaf8, decode 1 byte character.
	     */
	    if (EB_SIZE_PAGE <= out_length)
		return -1;
	    *out_buffer_p++ = node_p->value;
	    out_length++;
	}
    }

    /* not reached */
    return -1;
}
