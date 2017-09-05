/*                                                            -*- C -*-
 * Copyright (c) 1998, 99, 2000, 01  
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include <zlib.h>

#include "zio.h"

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

#ifndef O_BINARY
#define O_BINARY 0
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

/*
 * Whence parameter for lseek().
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif

#ifndef ENABLE_PTHREAD
#define pthread_mutex_lock(m)
#define pthread_mutex_unlock(m)
#endif

/*
 * Get an unsigned value from an octet stream buffer.
 */
#define zio_uint1(p) (*(const unsigned char *)(p))

#define zio_uint2(p) ((*(const unsigned char *)(p) << 8) \
        + (*(const unsigned char *)((p) + 1)))

#define zio_uint3(p) ((*(const unsigned char *)(p) << 16) \
        + (*(const unsigned char *)((p) + 1) << 8) \
        + (*(const unsigned char *)((p) + 2)))

#define zio_uint4(p) ((*(const unsigned char *)(p) << 24) \
        + (*(const unsigned char *)((p) + 1) << 16) \
        + (*(const unsigned char *)((p) + 2) << 8) \
        + (*(const unsigned char *)((p) + 3)))

/*
 * Size of a page (The term `page' means `block' in JIS X 4081).
 */
#define ZIO_SIZE_PAGE			2048

/*
 * Buffer for caching uncompressed data.
 */
static char *cache_buffer = NULL;

/*
 * File descriptor which caches data in `cache_buffer'.
 */
static int cache_file = -1;

/*
 * Offset of the beginning of the cached data `cache_buffer'.
 */
static off_t cache_location;

/*
 * Mutex for cache variables.
 */
#ifdef ENABLE_PTHREAD
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Unexported function.
 */
#ifdef __STDC__
static int zio_open_none(Zio *, const char *);
static int zio_open_ebzip(Zio *, const char *);
static int zio_open_epwing(Zio *, const char *);
static int zio_open_epwing6(Zio *, const char *);
static int zio_make_epwing_huffman_tree(Zio *, int);
static ssize_t zio_read_ebzip(Zio *, char *, size_t);
static ssize_t zio_read_epwing(Zio *, char *, size_t);
static ssize_t zio_read_raw(int, void *, size_t nbyte);
static int zio_unzip_slice_ebzip1(char *, size_t, char *, size_t);
static int zio_unzip_slice_epwing(char *, int, Zio_Huffman_Node *);
static int zio_unzip_slice_epwing6(char *, int, Zio_Huffman_Node *);
#else /* not __STDC__ */
static int zio_open_none();
static int zio_open_ebzip();
static int zio_open_epwing();
static int zio_open_epwing6();
static int zio_make_epwing_huffman_tree();
static ssize_t zio_read_ebzip();
static ssize_t zio_read_epwing();
static ssize_t zio_read_raw();
static int zio_unzip_slice_ebzip1();
static int zio_unzip_slice_epwing();
static int zio_unzip_slice_epwing6();
#endif /* not __STDC__ */

/*
 * Initialize cache buffer.
 */
int
zio_initialize_library()
{
    pthread_mutex_lock(&cache_mutex);

    /*
     * Allocate memory for cache buffer.
     */
    if (cache_buffer == NULL) {
	cache_buffer = (char *) malloc(ZIO_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL);
	if (cache_buffer == NULL)
	    goto failed;
    }

    pthread_mutex_unlock(&cache_mutex);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    pthread_mutex_unlock(&cache_mutex);
    return -1;
}


/*
 * Clear cache buffer.
 */
void
zio_finalize_library()
{
    pthread_mutex_lock(&cache_mutex);

    if (cache_buffer != NULL)
	free(cache_buffer);
    cache_buffer = NULL;
    cache_file = -1;

    pthread_mutex_unlock(&cache_mutex);
}


/*
 * Initialize `zio'.
 */
void
zio_initialize(zio)
    Zio *zio;
{
    zio->file = -1;
    zio->huffman_nodes = NULL;
    zio->huffman_root = NULL;
    zio->code = ZIO_INVALID;
    zio->file_size = 0;
}


/*
 * Finalize `zio'.
 */
void
zio_finalize(zio)
    Zio *zio;
{
    zio_close(zio);

    if (zio->huffman_nodes != NULL)
	free(zio->huffman_nodes);
    zio->huffman_nodes = NULL;
    zio->huffman_root = NULL;
    zio->code = ZIO_INVALID;
}


/*
 * Open `file'.
 */
int
zio_open(zio, file_name, zio_code)
    Zio *zio;
    const char *file_name;
    Zio_Code zio_code;
{
    if (0 <= zio->file)
	zio_close(zio);

    if (zio_code == ZIO_NONE)
	return zio_open_none(zio, file_name);
    else if (zio_code == ZIO_EBZIP1)
	return zio_open_ebzip(zio, file_name);
    else if (zio_code == ZIO_EPWING)
	return zio_open_epwing(zio, file_name);
    else if (zio_code == ZIO_EPWING6)
	return zio_open_epwing6(zio, file_name);
#if 0
    else if (zio_code == ZIO_SEBXA)
	return zio_open_sebxa(zio, file_name);
#endif

    return -1;
}


/*
 * Open an non-compressed file.
 */
static int
zio_open_none(zio, file_name)
    Zio *zio;
    const char *file_name;
{
    zio->file = open(file_name, O_RDONLY | O_BINARY);
    if (zio->file < 0)
	goto failed;

    zio->code = ZIO_NONE;
    zio->file_size = lseek(zio->file, 0, SEEK_END);
    if (zio->file_size < 0 || lseek(zio->file, 0, SEEK_SET) < 0)
	goto failed;

    return zio->file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= zio->file)
	close(zio->file);
    zio->file = -1;
    zio->code = ZIO_INVALID;
    return -1;
}


/*
 * Open an EBZIP compression file.
 */
static int
zio_open_ebzip(zio, file_name)
    Zio *zio;
    const char *file_name;
{
    char header[ZIO_SIZE_EBZIP_HEADER];

    /*
     * Try to open a `*.ebz' file.
     */
    zio->file = open(file_name, O_RDONLY | O_BINARY);
    if (zio->file < 0)
	goto failed;

    /*
     * Read header part of the ebzip'ped file.
     */
    if (zio_read_raw(zio->file, header, ZIO_SIZE_EBZIP_HEADER)
	!= ZIO_SIZE_EBZIP_HEADER)
	goto failed;
    zio->code = zio_uint1(header + 5) >> 4;
    zio->zip_level = zio_uint1(header + 5) & 0x0f;
    zio->slice_size = ZIO_SIZE_PAGE << zio->zip_level;
    zio->file_size = zio_uint4(header + 10);
    zio->crc = zio_uint4(header + 14);
    zio->mtime = zio_uint4(header + 18);
    zio->location = 0;

    if (zio->file_size < 1 << 16)
	zio->index_width = 2;
    else if (zio->file_size < 1 << 24)
	zio->index_width = 3;
    else
	zio->index_width = 4;

    /*
     * Check zio header information.
     */
    if (memcmp(header, "EBZip", 5) != 0
	|| zio->code != ZIO_EBZIP1
	|| ZIO_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL < zio->slice_size)
	goto failed;

    return zio->file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= zio->file)
	close(zio->file);
    zio->file = -1;
    zio->code = ZIO_INVALID;
    return -1;
}


/*
 * The buffer size must be 512 bytes, the number of 8 bit nodes.
 */
#define ZIO_EPWING_BUFFER_SIZE 512

/*
 * Open an EPWING compression file.
 */
static int
zio_open_epwing(zio, file_name)
    Zio *zio;
    const char *file_name;
{
    int leaf16_count;
    int leaf_count;
    char buffer[ZIO_EPWING_BUFFER_SIZE];
    char *buffer_p;
    size_t read_length;
    Zio_Huffman_Node *tail_node_p;
    int i;

    zio->code = ZIO_EPWING;
    zio->huffman_nodes = NULL;

    /*
     * Open `HONMON2'.
     */
    zio->file = open(file_name, O_RDONLY | O_BINARY);
    if (zio->file < 0)
	goto failed;

    /*
     * Read a header of `HONMON2' (32 bytes).
     * When `frequencies_length' is shorter than 512, we assumes the
     * file is broken.
     */
    if (zio_read_raw(zio->file, buffer, 32) != 32)
	goto failed;
    zio->location = 0;
    zio->slice_size = ZIO_SIZE_PAGE;
    zio->index_location = zio_uint4(buffer);
    zio->index_length = zio_uint4(buffer + 4);
    zio->frequencies_location = zio_uint4(buffer + 8);
    zio->frequencies_length = zio_uint4(buffer + 12);
    leaf16_count = (zio->frequencies_length - (256 * 2)) / 4;
    leaf_count = leaf16_count + 256 + 1;
    if (zio->index_length < 36 || zio->frequencies_length < 512)
	goto failed;

    /*
     * Check for the length of an uncompressed file.
     *
     * If the index of the non-first page in the last index group
     * is 0x0000, we assumes the data corresponding with the index
     * doesn't exist.
     */
    if (lseek(zio->file,
	zio->index_location + (zio->index_length - 36) / 36 * 36,
	SEEK_SET) < 0)
	goto failed;
    if (zio_read_raw(zio->file, buffer, 36) != 36)
	goto failed;
    zio->file_size = (size_t)(zio->index_length / 36) * (ZIO_SIZE_PAGE * 16);
    for (i = 1, buffer_p = buffer + 4 + 2; i < 16; i++, buffer_p += 2) {
	if (zio_uint2(buffer_p) == 0)
	    break;
    }
    zio->file_size -= ZIO_SIZE_PAGE * (16 - i);
    
    /*
     * Allocate memory for huffman nodes.
     */
    zio->huffman_nodes = (Zio_Huffman_Node *) malloc(sizeof(Zio_Huffman_Node)
	* leaf_count * 2);
    if (zio->huffman_nodes == NULL)
	goto failed;
    tail_node_p = zio->huffman_nodes;

    /*
     * Make leafs for 16bit character.
     */
    read_length = ZIO_EPWING_BUFFER_SIZE - (ZIO_EPWING_BUFFER_SIZE % 4);
    if (lseek(zio->file, zio->frequencies_location, SEEK_SET) < 0)
	goto failed;
    if (zio_read_raw(zio->file, buffer, read_length) != read_length)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < leaf16_count; i++) {
	if (buffer + read_length <= buffer_p) {
	    if (zio_read_raw(zio->file, buffer, read_length) != read_length)
		goto failed;
	    buffer_p = buffer;
	}
	tail_node_p->type = ZIO_HUFFMAN_NODE_LEAF16;
	tail_node_p->value = zio_uint2(buffer_p);
	tail_node_p->frequency = zio_uint2(buffer_p + 2);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 4;
	tail_node_p++;
    }

    /*
     * Make leafs for 8bit character.
     */
    if (lseek(zio->file, zio->frequencies_location + leaf16_count * 4,
	SEEK_SET) < 0)
	goto failed;
    if (zio_read_raw(zio->file, buffer, 512) != 512)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < 256; i++) {
	tail_node_p->type = ZIO_HUFFMAN_NODE_LEAF8;
	tail_node_p->value = i;
	tail_node_p->frequency = zio_uint2(buffer_p);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 2;
	tail_node_p++;
    }

    /*
     * Make a leaf for the end-of-page character.
     */
    tail_node_p->type = ZIO_HUFFMAN_NODE_EOF;
    tail_node_p->value = 256;
    tail_node_p->frequency = 1;
    tail_node_p++;
    
    /*
     * Make a huffman tree.
     */
    if (zio_make_epwing_huffman_tree(zio, leaf_count) < 0)
	goto failed;

    return zio->file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= zio->file)
	close(zio->file);
    if (zio->huffman_nodes != NULL)
	free(zio->huffman_nodes);
    zio->file = -1;
    zio->huffman_nodes = NULL;
    zio->huffman_root = NULL;
    zio->code = ZIO_INVALID;

    return -1;
}


/*
 * Open an EPWING compression file.
 */
static int
zio_open_epwing6(zio, file_name)
    Zio *zio;
    const char *file_name;
{
    int leaf32_count;
    int leaf16_count;
    int leaf_count;
    char buffer[ZIO_EPWING_BUFFER_SIZE];
    char *buffer_p;
    size_t read_length;
    Zio_Huffman_Node *tail_node_p;
    int i;

    zio->code = ZIO_EPWING6;
    zio->huffman_nodes = NULL;

    /*
     * Open `HONMON2'.
     */
    zio->file = open(file_name, O_RDONLY | O_BINARY);
    if (zio->file < 0)
	goto failed;

    /*
     * Read a header of `HONMON2' (48 bytes).
     * When `frequencies_length' is shorter than 512, we assumes the
     * file is broken.
     */
    if (zio_read_raw(zio->file, buffer, 48) != 48)
	goto failed;
    zio->location = 0;
    zio->slice_size = ZIO_SIZE_PAGE;
    zio->index_location = zio_uint4(buffer);
    zio->index_length = zio_uint4(buffer + 4);
    zio->frequencies_location = zio_uint4(buffer + 8);
    zio->frequencies_length = zio_uint4(buffer + 12);
    leaf16_count = 0x400;
    leaf32_count = (zio->frequencies_length - (leaf16_count * 4) - (256 * 2))
	/ 6;
    leaf_count = leaf32_count + leaf16_count + 256 + 1;
    if (zio->index_length < 36 || zio->frequencies_length < 512)
	goto failed;

    /*
     * Check for the length of an uncompressed file.
     *
     * If the index of the non-first page in the last index group
     * is 0x0000, we assumes the data corresponding with the index
     * doesn't exist.
     */
    if (lseek(zio->file,
	zio->index_location + (zio->index_length - 36) / 36 * 36,
	SEEK_SET) < 0)
	goto failed;
    if (zio_read_raw(zio->file, buffer, 36) != 36)
	goto failed;
    zio->file_size = (size_t)(zio->index_length / 36) * (ZIO_SIZE_PAGE * 16);
    for (i = 1, buffer_p = buffer + 4 + 2; i < 16; i++, buffer_p += 2) {
	if (zio_uint2(buffer_p) == 0)
	    break;
    }
    zio->file_size -= ZIO_SIZE_PAGE * (16 - i);
    
    /*
     * Allocate memory for huffman nodes.
     */
    zio->huffman_nodes = (Zio_Huffman_Node *) malloc(sizeof(Zio_Huffman_Node)
	* leaf_count * 2);
    if (zio->huffman_nodes == NULL)
	goto failed;
    tail_node_p = zio->huffman_nodes;

    /*
     * Make leafs for 32bit character.
     */
    read_length = ZIO_EPWING_BUFFER_SIZE - (ZIO_EPWING_BUFFER_SIZE % 6);
    if (lseek(zio->file, zio->frequencies_location, SEEK_SET) < 0)
	goto failed;
    if (zio_read_raw(zio->file, buffer, read_length) != read_length)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < leaf32_count; i++) {
	if (buffer + read_length <= buffer_p) {
	    if (zio_read_raw(zio->file, buffer, read_length) != read_length)
		goto failed;
	    buffer_p = buffer;
	}
	tail_node_p->type = ZIO_HUFFMAN_NODE_LEAF32;
	tail_node_p->value = zio_uint4(buffer_p);
	tail_node_p->frequency = zio_uint2(buffer_p + 4);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 6;
	tail_node_p++;
    }

    /*
     * Make leafs for 16bit character.
     */
    read_length = ZIO_EPWING_BUFFER_SIZE - (ZIO_EPWING_BUFFER_SIZE % 4);
    if (lseek(zio->file, zio->frequencies_location + leaf32_count * 6,
	SEEK_SET) < 0)
	goto failed;
    if (zio_read_raw(zio->file, buffer, read_length) != read_length)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < leaf16_count; i++) {
	if (buffer + read_length <= buffer_p) {
	    if (zio_read_raw(zio->file, buffer, read_length) != read_length)
		goto failed;
	    buffer_p = buffer;
	}
	tail_node_p->type = ZIO_HUFFMAN_NODE_LEAF16;
	tail_node_p->value = zio_uint2(buffer_p);
	tail_node_p->frequency = zio_uint2(buffer_p + 2);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 4;
	tail_node_p++;
    }

    /*
     * Make leafs for 8bit character.
     */
    if (lseek(zio->file,
	zio->frequencies_location + leaf32_count * 6 + leaf16_count * 4,
	SEEK_SET) < 0)
	goto failed;
    if (zio_read_raw(zio->file, buffer, 512) != 512)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < 256; i++) {
	tail_node_p->type = ZIO_HUFFMAN_NODE_LEAF8;
	tail_node_p->value = i;
	tail_node_p->frequency = zio_uint2(buffer_p);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 2;
	tail_node_p++;
    }

    /*
     * Make a leaf for the end-of-page character.
     */
    tail_node_p->type = ZIO_HUFFMAN_NODE_EOF;
    tail_node_p->value = 256;
    tail_node_p->frequency = 1;
    tail_node_p++;

    /*
     * Make a huffman tree.
     */
    if (zio_make_epwing_huffman_tree(zio, leaf_count) < 0)
	goto failed;

    return zio->file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= zio->file)
	close(zio->file);
    if (zio->huffman_nodes != NULL)
	free(zio->huffman_nodes);
    zio->file = -1;
    zio->huffman_nodes = NULL;
    zio->huffman_root = NULL;
    zio->code = ZIO_INVALID;

    return -1;
}


/*
 * Make a huffman tree for decompressing EPWING compression data.
 */
static int
zio_make_epwing_huffman_tree(zio, leaf_count)
    Zio *zio;
    int leaf_count;
{    
    Zio_Huffman_Node *target_node;
    Zio_Huffman_Node *most_node;
    Zio_Huffman_Node *node_p;
    Zio_Huffman_Node temporary_node;
    Zio_Huffman_Node *least_node_p;
    Zio_Huffman_Node *tail_node_p;
    int i;
    int j;

    tail_node_p = zio->huffman_nodes + leaf_count;

    /*
     * Sort the leaf nodes in frequency order.
     */
    for (i = 0; i < leaf_count - 1; i++) {
        target_node = zio->huffman_nodes + i;
        most_node = target_node;
        node_p = zio->huffman_nodes + i + 1;
  
        for (j = i + 1; j < leaf_count; j++) {
            if (most_node->frequency < node_p->frequency)
                most_node = node_p;
            node_p++;
        } 

        temporary_node.type = most_node->type;
        temporary_node.value = most_node->value;
        temporary_node.frequency = most_node->frequency;

        most_node->type = target_node->type;
        most_node->value = target_node->value;
        most_node->frequency = target_node->frequency;

        target_node->type = temporary_node.type;
        target_node->value = temporary_node.value;
        target_node->frequency = temporary_node.frequency;
    }

    /*
     * Make intermediate nodes of the huffman tree.
     * The number of intermediate nodes of the tree is <the number of
     * leaf nodes> - 1.
     */
    for (i = 1; i < leaf_count; i++) {
	/*
	 * Initialize a new intermediate node.
	 */
	tail_node_p->type = ZIO_HUFFMAN_NODE_INTERMEDIATE;
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;

	/*
	 * Find for a least frequent node.
	 * That node becomes a left child of the new intermediate node.
	 */
	least_node_p = NULL;
	for (node_p = zio->huffman_nodes; node_p < tail_node_p; node_p++) {
	    if (node_p->frequency == 0)
		continue;
	    if (least_node_p == NULL
		|| node_p->frequency <= least_node_p->frequency)
		least_node_p = node_p;
	}
	if (least_node_p == NULL)
	    return -1;
	tail_node_p->left = least_node_p;
	tail_node_p->frequency = least_node_p->frequency;
	least_node_p->frequency = 0;

	/*
	 * Find for a next least frequent node.
	 * That node becomes a right child of the new intermediate node.
	 */
	least_node_p = NULL;
	for (node_p = zio->huffman_nodes; node_p < tail_node_p; node_p++) {
	    if (node_p->frequency == 0)
		continue;
	    if (least_node_p == NULL
		|| node_p->frequency <= least_node_p->frequency)
		least_node_p = node_p;
	}
	if (least_node_p == NULL)
	    return -1;
	tail_node_p->right = least_node_p;
	tail_node_p->frequency += least_node_p->frequency;
	least_node_p->frequency = 0;

	tail_node_p++;
    }

    /*
     * Set a root node of the huffman tree.
     */ 
    zio->huffman_root = tail_node_p - 1;

    return 0;
}


/*
 * Reopen a file.
 */
int
zio_reopen(zio, file_name, zio_code)
    Zio *zio;
    const char *file_name;
    Zio_Code zio_code;
{
    if (zio->code == ZIO_INVALID)
	return zio_open(zio, file_name, zio_code);

    if (0 <= zio->file)
	zio_close(zio);

    zio->file = open(file_name, O_RDONLY | O_BINARY);
    if (zio->file < 0) {
	zio->code = ZIO_INVALID;
	return -1;
    }

    zio->location = 0;

    return zio->file;
}


/*
 * Close `zio'.
 */
void
zio_close(zio)
    Zio *zio;
{
    pthread_mutex_lock(&cache_mutex);

    /*
     * If contents of the file is cached, clear the cache.
     */
    if (cache_file == zio->file)
	cache_file = -1;

    pthread_mutex_unlock(&cache_mutex);

    if (0 <= zio->file)
	close(zio->file);
    zio->file = -1;
}


/*
 * Return file descriptor of `zio'.
 */
int
zio_file(zio)
    Zio *zio;
{
    return zio->file;
}


/*
 * Seek `zio'.
 */
off_t
zio_lseek(zio, location, whence)
    Zio *zio;
    off_t location;
    int whence;
{
    if (zio->file < 0)
	return -1;

    if (zio->code == ZIO_NONE) {
	/*
	 * If `zio' is not compressed, simply call lseek().
	 */
	return lseek(zio->file, location, whence);
    } else {
	/*
	 * Calculate new location according with `whence'.
	 */
	switch (whence) {
	case SEEK_SET:
	    zio->location = location;
	    break;
	case SEEK_CUR:
	    zio->location = zio->location + location;
	    break;
	case SEEK_END:
	    zio->location = zio->file_size - location;
	    break;
	default:
#ifdef EINVAL
	    errno = EINVAL;
#endif
	    return -1;
	}

	/*
	 * Adjust location.
	 */
	if (zio->location < 0)
	    zio->location = 0;
	if (zio->file_size < zio->location)
	    zio->location = zio->file_size;

	/*
	 * Update `zio->location'.
	 * (We don't actually seek the file.)
	 */
	return zio->location;
    }

    /* never reached */
    return -1;
}


/*
 * Read data from `zio' file.
 */
ssize_t
zio_read(zio, buffer, length)
    Zio *zio;
    char *buffer;
    size_t length;
{
    ssize_t read_length;

    if (zio->file < 0)
	return -1;

    /*
     * If the zio `file' is not compressed, call read() and return.
     */
    pthread_mutex_lock(&cache_mutex);

    if (zio->code == ZIO_NONE)
	read_length = zio_read_raw(zio->file, buffer, length);
    else if (zio->code == ZIO_EBZIP1)
	read_length = zio_read_ebzip(zio, buffer, length);
    else if (zio->code == ZIO_EPWING)
	read_length = zio_read_epwing(zio, buffer, length);
    else if (zio->code == ZIO_EPWING6)
	read_length = zio_read_epwing(zio, buffer, length);
#if 0
    else if (zio->code == ZIO_SEBXA)
	read_length = zio_read_sebxa(zio, buffer, length);
#endif
    else
	read_length = -1;

    pthread_mutex_unlock(&cache_mutex);

    return read_length;
}


/*
 * Read data from the `zio' file compressed with the ebzip compression
 * format.
 */
static ssize_t
zio_read_ebzip(zio, buffer, length)
    Zio *zio;
    char *buffer;
    size_t length;
{
    char temporary_buffer[ZIO_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL];
    size_t read_length = 0;
    size_t zipped_slice_size;
    off_t slice_location;
    off_t next_slice_location;
    int n;
    
    /*
     * Read data.
     */
    while (read_length < length) {
	if (zio->file_size <= zio->location)
	    return read_length;

	/*
	 * If data in `cache_buffer' is out of range, read data from
	 * `zio->file'.
	 */
	if (cache_file != zio->file
	    || zio->location < cache_location 
	    || cache_location + zio->slice_size <= zio->location) {

	    cache_file = -1;
	    cache_location = zio->location - (zio->location % zio->slice_size);

	    /*
	     * Get buffer location and size from index table in `zio->file'.
	     */
	    if (lseek(zio->file,
		zio->location / zio->slice_size * zio->index_width
		+ ZIO_SIZE_EBZIP_HEADER, SEEK_SET) < 0)
		goto failed;
	    if (zio_read_raw(zio->file, temporary_buffer, zio->index_width * 2)
		!= zio->index_width * 2)
		goto failed;

	    switch (zio->index_width) {
	    case 2:
		slice_location = zio_uint2(temporary_buffer);
		next_slice_location = zio_uint2(temporary_buffer + 2);
		break;
	    case 3:
		slice_location = zio_uint3(temporary_buffer);
		next_slice_location = zio_uint3(temporary_buffer + 3);
		break;
	    case 4:
		slice_location = zio_uint4(temporary_buffer);
		next_slice_location = zio_uint4(temporary_buffer + 4);
		break;
	    default:
		goto failed;
	    }
	    zipped_slice_size = next_slice_location - slice_location;

	    if (next_slice_location <= slice_location
		|| zio->slice_size < zipped_slice_size)
		goto failed;

	    /*
	     * Read a compressed slice from `zio->file' and uncompress it.
	     * The data is not compressed if its size is equals to
	     * slice size.
	     */
	    if (lseek(zio->file, slice_location, SEEK_SET) < 0)
		goto failed;
	    if (zio_read_raw(zio->file, temporary_buffer, zipped_slice_size)
		!= zipped_slice_size)
		goto failed;
	    if (zio->slice_size == zipped_slice_size)
		memcpy(cache_buffer, temporary_buffer, zio->slice_size);
	    else if (zio_unzip_slice_ebzip1(cache_buffer, zio->slice_size,
		temporary_buffer, zipped_slice_size) < 0)
		goto failed;

	    cache_file = zio->file;
	}

	/*
	 * Copy data from `cache_buffer' to `buffer'.
	 */
	n = zio->slice_size - (zio->location % zio->slice_size);
	if (length - read_length < n)
	    n = length - read_length;
	if (zio->file_size - zio->location < n)
	    n = zio->file_size - zio->location;
	memcpy(buffer + read_length,
	    cache_buffer + (zio->location - cache_location), n);
	read_length += n;
	zio->location += n;
    }

    return read_length;

    /*
     * An error occurs...
     */
  failed:
    return -1;
}


/*
 * Read data from the `zio' file compressed with the EPWING or EPWING V6 
 * compression format.
 */
static ssize_t
zio_read_epwing(zio, buffer, length)
    Zio *zio;
    char *buffer;
    size_t length;
{
    char temporary_buffer[36];
    size_t read_length = 0;
    off_t page_location;
    int n;
    
    /*
     * Read data.
     */
    while (read_length < length) {
	if (zio->file_size <= zio->location)
	    return read_length;

	/*
	 * If data in `cache_buffer' is out of range, read data from the zio
	 * file.
	 */
	if (cache_file != zio->file || zio->location < cache_location 
	    || cache_location + ZIO_SIZE_PAGE <= zio->location) {
	    cache_file = -1;
	    cache_location = zio->location - (zio->location % ZIO_SIZE_PAGE);

	    /*
	     * Get page location from index table in `zio->file'.
	     */
	    if (lseek(zio->file, zio->index_location
		+ zio->location / (ZIO_SIZE_PAGE * 16) * 36, SEEK_SET) < 0)
		goto failed;
	    if (zio_read_raw(zio->file, temporary_buffer, 36) != 36)
		goto failed;
	    page_location = zio_uint4(temporary_buffer)
		+ zio_uint2(temporary_buffer + 4
		    + (zio->location / ZIO_SIZE_PAGE % 16) * 2);

	    /*
	     * Read a compressed page from `zio->file' and uncompress it.
	     */
	    if (lseek(zio->file, page_location, SEEK_SET) < 0)
		goto failed;
	    if (zio->code == ZIO_EPWING) {
		if (zio_unzip_slice_epwing(cache_buffer, zio->file,
		    zio->huffman_root) < 0)
		    goto failed;
	    } else {
		if (zio_unzip_slice_epwing6(cache_buffer, zio->file,
		    zio->huffman_root) < 0)
		    goto failed;
	    }

	    cache_file = zio->file;
	}

	/*
	 * Copy data from `cache_buffer' to `buffer'.
	 */
	n = ZIO_SIZE_PAGE - (zio->location % ZIO_SIZE_PAGE);
	if (length - read_length < n)
	    n = length - read_length;
	if (zio->file_size - zio->location < n)
	    n = zio->file_size - zio->location;
	memcpy(buffer + read_length,
	    cache_buffer + (zio->location - cache_location), n);
	read_length += n;
	zio->location += n;
    }

    return read_length;

    /*
     * An error occurs...
     */
  failed:
    return -1;
}


#if 0
/*
 * Read data from the zio `file' compressed with the S-EBXA compression
 * format.
 */
static ssize_t
zio_read_sebxa(zio, buffer, length)
    Zio *zio;
    char *buffer;
    size_t length;
{
    char temporary_buffer[ZIO_SEBXA_ZIO_SLICE_LENGTH];
    size_t read_length = 0;
    size_t zipped_slice_size;
    off_t slice_location;
    off_t next_slice_location;
    int n;
    
    /*
     * Read data.
     */
    while (read_length < length) {
	if (zio->file_size <= zio->location)
	    return read_length;

	/*
	 * If data in `cache_buffer' is out of range, read data from the
	 * zio `file'.
	 */
	if (cache_file != zio->file || zio->location < cache_location 
	    || cache_location + zio->slice_size <= zio->location) {

	    cache_file = -1;
	    cache_location = zio->location - (zio->location % zio->slice_size);

	    /*
	     * Get buffer location and size from index table in `zio->file'.
	     */
	    if (lseek(zio->file, zio->location / zio->slice_size
		* zio->index_width + ZIO_SIZE_EBZIP_HEADER, SEEK_SET) < 0)
		goto failed;
	    if (zio_read_raw(zio->file, temporary_buffer, zio->index_width * 2)
		!= zio->index_width * 2)
		goto failed;

	    slice_location = zio_uint4(temporary_buffer);
	    next_slice_location = zio_uint4(temporary_buffer + 4);
	    zipped_slice_size = next_slice_location - slice_location;

	    if (next_slice_location <= slice_location
		|| zio->slice_size < zipped_slice_size)
		goto failed;

	    /*
	     * Read a compressed slice from `zio->file' and uncompress it.
	     * The data is not compressed if its size is equals to
	     * slice size.
	     */
	    if (lseek(zio->file, slice_location, SEEK_SET) < 0)
		goto failed;
	    if (zio_read_raw(zio->file, temporary_buffer, zipped_slice_size)
		!= zipped_slice_size)
		goto failed;
	    if (zio->slice_size == zipped_slice_size)
		memcpy(cache_buffer, temporary_buffer, zio->slice_size);
	    else if (zio_unzip_slice_ebzip1(cache_buffer, zio->slice_size,
		temporary_buffer, zipped_slice_size) < 0)
		goto failed;

	    cache_file = zio->file;
	}

	/*
	 * Copy data from `cache_buffer' to `buffer'.
	 */
	n = zio->slice_size - (zio->location % zio->slice_size);
	if (length - read_length < n)
	    n = length - read_length;
	if (zio->file_size - zio->location < n)
	    n = zio->file_size - zio->location;
	memcpy(buffer + read_length,
	    cache_buffer + (zio->location - cache_location), n);
	read_length += n;
	zio->location += n;
    }

    return read_length;

    /*
     * An error occurs...
     */
  failed:
    return -1;
}
#endif

/*
 * Read data from a file.
 * It repeats to call read() until all data will have been read.
 */
static ssize_t
zio_read_raw(file, buffer, length)
    int file;
    VOID *buffer;
    size_t length;
{
    char *buffer_p = buffer;
    ssize_t rest_length = length;
    ssize_t n;

    while (0 < rest_length) {
	errno = 0;
	n = read(file, buffer_p, rest_length);
	if (n < 0) {
	    if (errno == EINTR)
		continue;
	    return n;
	} else if (n == 0)
	    return length - rest_length;
	else {
	    rest_length -= n;
	    buffer_p += n;
	}
    }

    return length;
}


/*
 * Uncompress an ebzip'ped slice.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
static int
zio_unzip_slice_ebzip1(out_buffer, out_byte_length, in_buffer, in_byte_length)
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


/*
 * Uncompress an EPWING compressed slice.
 * The offset of `in_file' must points to the beginning of the compressed
 * slice.  Uncompressed data are put into `out_buffer'.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
static int
zio_unzip_slice_epwing(out_buffer, in_file, huffman_tree)
    char *out_buffer;
    int in_file;
    Zio_Huffman_Node *huffman_tree;
{
    Zio_Huffman_Node *node_p;
    int bit;
    char in_buffer[ZIO_SIZE_PAGE];
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
	while (node_p->type == ZIO_HUFFMAN_NODE_INTERMEDIATE) {

	    /*
	     * If no data is left in the input buffer, read next chunk.
	     */
	    if ((unsigned char *)in_buffer + in_read_length <= in_buffer_p) {
		in_read_length = zio_read_raw(in_file, in_buffer,
		    ZIO_SIZE_PAGE);
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

	if (node_p->type == ZIO_HUFFMAN_NODE_EOF) {
	    /*
	     * Fill the rest of the output buffer with NUL,
             * when we meet an EOF mark before decode ZIO_SIZE_PAGE bytes.
	     */
	    if (out_length < ZIO_SIZE_PAGE) {
#ifdef HAVE_MEMCPY
		memset(out_buffer_p, ZIO_SIZE_PAGE - out_length, '\0');
#else
		bzero(out_buffer_p, ZIO_SIZE_PAGE - out_length);
#endif
		out_length = ZIO_SIZE_PAGE;
	    }
	    return out_length;
	} else if (node_p->type == ZIO_HUFFMAN_NODE_LEAF16) {
	    /*
	     * The leaf is leaf16, decode 2 bytes character.
	     */
	    if (ZIO_SIZE_PAGE <= out_length)
		return -1;
	    else if (ZIO_SIZE_PAGE <= out_length + 1) {
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
	    if (ZIO_SIZE_PAGE <= out_length)
		return -1;
	    else {
		*out_buffer_p++ = node_p->value;
		out_length++;
	    }
	}
    }

    /* not reached */
    return -1;
}


/*
 * Uncompress an EPWING V6 compressed slice.
 * The offset of `in_file' must points to the beginning of the compressed
 * slice.  Uncompressed data are put into `out_buffer'.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
static int
zio_unzip_slice_epwing6(out_buffer, in_file, huffman_tree)
    char *out_buffer;
    int in_file;
    Zio_Huffman_Node *huffman_tree;
{
    Zio_Huffman_Node *node_p;
    int bit;
    char in_buffer[ZIO_SIZE_PAGE];
    unsigned char *in_buffer_p;
    size_t in_read_length;
    int in_bit_index;
    unsigned char *out_buffer_p;
    size_t out_length;
    int compression_type;

    in_buffer_p = (unsigned char *)in_buffer;
    in_bit_index = 7;
    in_read_length = 0;
    out_buffer_p = (unsigned char *)out_buffer;
    out_length = 0;

    /*
     * Get compression type.
     */
    if (zio_read_raw(in_file, in_buffer, 1) != 1)
	return -1;
    compression_type = zio_uint1(in_buffer);

    /*
     * If compression type is not 0, this page is not compressed.
     */
    if (compression_type != 0) {
	if (zio_read_raw(in_file, out_buffer, ZIO_SIZE_PAGE) != 1)
	    return -1;
	return ZIO_SIZE_PAGE;
    }

    while (out_length < ZIO_SIZE_PAGE) {
	/*
	 * Descend the huffman tree until reached to the leaf node.
	 */
	node_p = huffman_tree;
	while (node_p->type == ZIO_HUFFMAN_NODE_INTERMEDIATE) {

	    /*
	     * If no data is left in the input buffer, read next chunk.
	     */
	    if ((unsigned char *)in_buffer + in_read_length <= in_buffer_p) {
		in_read_length = zio_read_raw(in_file, in_buffer,
		    ZIO_SIZE_PAGE);
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

	if (node_p->type == ZIO_HUFFMAN_NODE_EOF) {
	    /*
	     * Fill the rest of the output buffer with NUL,
             * when we meet an EOF mark before decode ZIO_SIZE_PAGE bytes.
	     */
	    if (out_length < ZIO_SIZE_PAGE) {
#ifdef HAVE_MEMCPY
		memset(out_buffer_p, ZIO_SIZE_PAGE - out_length, '\0');
#else
		bzero(out_buffer_p, ZIO_SIZE_PAGE - out_length);
#endif
		out_length = ZIO_SIZE_PAGE;
	    }
	    return out_length;
	} else if (node_p->type == ZIO_HUFFMAN_NODE_LEAF32) {
	    /*
	     * The leaf is leaf32, decode 4 bytes character.
	     */
	    if (ZIO_SIZE_PAGE <= out_length + 1) {
		*out_buffer_p++ = (node_p->value >> 24) & 0xff;
		out_length++;
	    } else if (ZIO_SIZE_PAGE <= out_length + 2) {
		*out_buffer_p++ = (node_p->value >> 24) & 0xff;
		*out_buffer_p++ = (node_p->value >> 16) & 0xff;
		out_length += 2;
	    } else if (ZIO_SIZE_PAGE <= out_length + 3) {
		*out_buffer_p++ = (node_p->value >> 24) & 0xff;
		*out_buffer_p++ = (node_p->value >> 16) & 0xff;
		*out_buffer_p++ = (node_p->value >> 8)  & 0xff;
		out_length += 3;
	    } else {
		*out_buffer_p++ = (node_p->value >> 24) & 0xff;
		*out_buffer_p++ = (node_p->value >> 16) & 0xff;
		*out_buffer_p++ = (node_p->value >> 8)  & 0xff;
		*out_buffer_p++ = node_p->value         & 0xff;
		out_length += 4;
	    }
	} else if (node_p->type == ZIO_HUFFMAN_NODE_LEAF16) {
	    /*
	     * The leaf is leaf16, decode 2 bytes character.
	     */
	    if (ZIO_SIZE_PAGE <= out_length + 1) {
		*out_buffer_p++ = (node_p->value >> 8)  & 0xff;
		out_length++;
	    } else {
		*out_buffer_p++ = (node_p->value >> 8)  & 0xff;
		*out_buffer_p++ = node_p->value & 0xff;
		out_length += 2;
	    }
	} else {
	    /*
	     * The leaf is leaf8, decode 1 byte character.
	     */
	    *out_buffer_p++ = node_p->value;
	    out_length++;
	}
    }

    return out_length;
}

#if 0
#define ZIO_SEBXA_SLICE_LENGTH	4096

/*
 * Uncompress an S-EBXA compressed slice.
 * The offset of `in_file' must points to the beginning of the compressed
 * slice.  Uncompressed data are put into `out_buffer'.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
static int
zio_unzip_slice_sebxa(out_buffer, in_file)
    char *out_buffer;
    int in_file;
{
    char in_buffer[ZIO_SEBXA_SLICE_LENGTH];
    unsigned char *in_buffer_p;
    size_t in_read_rest;
    unsigned char *out_buffer_p;
    size_t out_length;
    int compression_flags[8];
    int copy_location;
    int copy_length;
    int i, j;

    in_buffer_p = (unsigned char *)in_buffer;
    in_read_rest = 0;
    out_buffer_p = (unsigned char *)out_buffer;
    out_length = 0;

    for (;;) {
	/*
	 * If no data is left in the input buffer, read next chunk.
	 */
	if (in_read_rest <= 0) {
	    in_read_rest = zio_read_raw(in_file, in_buffer, 
		ZIO_SEBXA_SLICE_LENGTH);
	    if (in_read_rest <= 0)
		return -1;
	    in_buffer_p = (unsigned char *)in_buffer;
	}

	/*
	 * The current input byte is recognized as compression flags
	 * for next 8 chunks.
	 */
	compression_flags[0] = (*in_buffer_p & 0x01);
	compression_flags[1] = (*in_buffer_p & 0x02) >> 1;
	compression_flags[2] = (*in_buffer_p & 0x04) >> 2;
	compression_flags[3] = (*in_buffer_p & 0x08) >> 3;
	compression_flags[4] = (*in_buffer_p & 0x10) >> 4;
	compression_flags[5] = (*in_buffer_p & 0x20) >> 5;
	compression_flags[6] = (*in_buffer_p & 0x40) >> 6;
	compression_flags[7] = (*in_buffer_p & 0x80) >> 7;
	in_buffer_p++;
	in_read_rest--;

	/*
	 * Decode 8 chunks.
	 */
	for (i = 0; i < 8; i++) {
	    if (compression_flags[i]) {
		/*
		 * This chunk is compressed.
		 * Copy `copy_length' bytes from `copy_p' to the current
		 * point.
		 */
		unsigned char *copy_p;

		if (in_read_rest <= 1)
		    return -1;

		/*
		 * Get 2 bytes from the current input, and recognize
		 * them as following:
		 * 
		 *               in_buffer_p   in_bufer_p + 1
		 *  bit pattern: [AAAA|BBBB]  [CCCC|DDDD]
		 * 
		 *  copy_location = ([CCCCAAAABBBB] + 4084) % slice_length
		 *  copy_length   = [DDDD] + 3
		 */
		copy_p = (unsigned char *)out_buffer
		    + (((*(in_buffer_p + 1) & 0xf0) << 4) + *in_buffer_p
			+ 4084) % ZIO_SEBXA_SLICE_LENGTH;
		if (out_length <= copy_location)
		    return -1;

		copy_length = (*(in_buffer_p + 1) & 0x0f) + 3;
		if (ZIO_SEBXA_SLICE_LENGTH < out_length + copy_length)
		    copy_length = ZIO_SEBXA_SLICE_LENGTH - out_length;

		for (j = 0; j < copy_length; j++)
		    *out_buffer_p++ = *copy_p++;

		in_read_rest -= 2;
		in_buffer_p += 2;
		out_length += copy_length;

	    } else {
		/*
		 * This chunk is not compressed.
		 * Put the current input byte as a decoded value.
		 */
		if (in_read_rest <= 0)
		    return -1;
		in_read_rest -= 1;
		*out_buffer_p++ = *in_buffer_p++;
		out_length += 1;
	    }

	    /*
	     * Return if the slice has been uncompressed.
	     */
	    if (ZIO_SEBXA_SLICE_LENGTH <= out_length)
		return out_length;
	}
    }

    /* not reached */
    return out_length;
}

#endif
