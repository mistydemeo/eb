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

/*
 * memcpy(), memchr(), memcmp(), memmove() and memset().
 */
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

/*
 * Flags for open().
 */
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

/*
 * Generic pointer type.
 */
#ifndef VOID
#ifdef __STDC__
#define VOID void
#else
#define VOID char
#endif
#endif

/*
 * Mutual exclusion lock of Pthreads.
 */
#ifndef ENABLE_PTHREAD
#define pthread_mutex_lock(m)
#define pthread_mutex_unlock(m)
#endif

/*
 * Debug message handler.
 */
#ifdef ENABLE_DEBUG
#define LOG(x) do {eb_log x;} while (0)
#else
#define LOG(x)
#endif

#ifdef __STDC__
void eb_log(const char *message, ...);
#else
void eb_log();
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
 * NULL Zio ID.
 */
#define ZIO_ID_NONE			-1

/*
 * Buffer for caching uncompressed data.
 */
static char *cache_buffer = NULL;

/*
 * Zio ID which caches data in `cache_buffer'.
 */
static int cache_zio_id = ZIO_ID_NONE;

/*
 * Offset of the beginning of the cached data `cache_buffer'.
 */
static off_t cache_location;

/*
 * Zio object counter.
 */
static int zio_counter = 0;

/*
 * Mutex for cache variables.
 */
#ifdef ENABLE_PTHREAD
static pthread_mutex_t zio_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Unexported function.
 */
#ifdef __STDC__
static int zio_reopen(Zio *, const char *);
static int zio_open_plain(Zio *, const char *);
static int zio_open_ebzip(Zio *, const char *);
static int zio_open_epwing(Zio *, const char *);
static int zio_open_epwing6(Zio *, const char *);
static int zio_make_epwing_huffman_tree(Zio *, int);
static ssize_t zio_read_ebzip(Zio *, char *, size_t);
static ssize_t zio_read_epwing(Zio *, char *, size_t);
static ssize_t zio_read_raw(int, void *, size_t nbyte);
static ssize_t zio_read_sebxa(Zio *, char *, size_t);
static int zio_unzip_slice_ebzip1(char *, int, size_t, size_t);
static int zio_unzip_slice_epwing(char *, int, Zio_Huffman_Node *);
static int zio_unzip_slice_epwing6(char *, int, Zio_Huffman_Node *);
static int zio_unzip_slice_sebxa(char *, int);
#else /* not __STDC__ */
static int zio_reopen();
static int zio_open_plain();
static int zio_open_ebzip();
static int zio_open_epwing();
static int zio_open_epwing6();
static int zio_make_epwing_huffman_tree();
static ssize_t zio_read_ebzip();
static ssize_t zio_read_epwing();
static ssize_t zio_read_raw();
static ssize_t zio_read_sebxa();
static int zio_unzip_slice_ebzip1();
static int zio_unzip_slice_epwing();
static int zio_unzip_slice_epwing6();
static int zio_unzip_slice_sebxa();
#endif /* not __STDC__ */

/*
 * Initialize cache buffer.
 */
int
zio_initialize_library()
{
    pthread_mutex_lock(&zio_mutex);
    LOG(("in: zio_initialize_library()"));

    /*
     * Allocate memory for cache buffer.
     */
    if (cache_buffer == NULL) {
	cache_buffer = (char *) malloc(ZIO_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL);
	if (cache_buffer == NULL)
	    goto failed;
    }

    LOG(("out: zio_initialize_library() = %d", 0));
    pthread_mutex_unlock(&zio_mutex);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_initialize_library() = %d", -1));
    pthread_mutex_unlock(&zio_mutex);
    return -1;
}


/*
 * Clear cache buffer.
 */
void
zio_finalize_library()
{
    pthread_mutex_lock(&zio_mutex);
    LOG(("in: zio_finalize_library()"));

    if (cache_buffer != NULL)
	free(cache_buffer);
    cache_buffer = NULL;
    cache_zio_id = ZIO_ID_NONE;

    LOG(("out: zio_finalize_library()"));
    pthread_mutex_unlock(&zio_mutex);
}


/*
 * Initialize `zio'.
 */
void
zio_initialize(zio)
    Zio *zio;
{
    LOG(("in: zio_initialize()"));

    zio->id = -1;
    zio->file = -1;
    zio->huffman_nodes = NULL;
    zio->huffman_root = NULL;
    zio->code = ZIO_INVALID;
    zio->file_size = 0;

    LOG(("out: zio_initialize()"));
}


/*
 * Finalize `zio'.
 */
void
zio_finalize(zio)
    Zio *zio;
{
    LOG(("in: zio_finalize(zio=%d)", (int)zio->id));

    zio_close(zio);
    if (zio->huffman_nodes != NULL)
	free(zio->huffman_nodes);

    zio->id = -1;
    zio->huffman_nodes = NULL;
    zio->huffman_root = NULL;
    zio->code = ZIO_INVALID;

    LOG(("out: zio_finalize()"));
}


/*
 * Set S-EBXA compression mode.
 */
int
zio_set_sebxa_mode(zio, index_location, index_base, zio_start_location,
    zio_end_location)
    Zio *zio;
    off_t index_location;
    off_t index_base;
    off_t zio_start_location;
    off_t zio_end_location;
{
    LOG(("in: zio_set_sebxa_mode(zio=%d, index_location=%ld, index_base=%ld, \
zio_start_location=%ld, zio_end_location=%ld)",
	(int)zio->id, (long)index_location, (long)index_base,
	(long)zio_start_location, (long)zio_end_location));

    if (zio->code != ZIO_PLAIN)
	goto failed;

    zio->code = ZIO_SEBXA;
    zio->index_location = index_location;
    zio->index_base = index_base;
    zio->zio_start_location = zio_start_location;
    zio->zio_end_location = zio_end_location;
    zio->file_size = zio_end_location;

    LOG(("out: zio_set_sebxa_mode() = %d", 0));
    return 0;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_set_sebxa_mode() = %d", -1));
    return -1;
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
    int result;

    LOG(("in: zio_open(zio=%d, file_name=%s, zio_code=%d)",
	(int)zio->id, file_name, zio_code));

    if (0 <= zio->file) {
	if (zio_code == ZIO_REOPEN) {
	    result = 0;
	    goto succeeded;
	}
	zio_finalize(zio);
	zio_initialize(zio);
    }

    if (zio_code == ZIO_REOPEN)
	result = zio_reopen(zio, file_name);
    else if (zio_code == ZIO_PLAIN)
	result = zio_open_plain(zio, file_name);
    else if (zio_code == ZIO_EBZIP1)
	result = zio_open_ebzip(zio, file_name);
    else if (zio_code == ZIO_EPWING)
	result = zio_open_epwing(zio, file_name);
    else if (zio_code == ZIO_EPWING6)
	result = zio_open_epwing6(zio, file_name);
    else if (zio_code == ZIO_SEBXA)
	result = zio_open_plain(zio, file_name);
    else
	result = -1;

  succeeded:
    LOG(("out: zio_open() = %d", result));
    return result;
}


/*
 * Reopen a file.
 */
static int
zio_reopen(zio, file_name)
    Zio *zio;
    const char *file_name;
{
    LOG(("in: zio_reopen(zio=%d, file_name=%s)", (int)zio->id, file_name));

    if (zio->code == ZIO_INVALID)
	goto failed;

    zio->file = open(file_name, O_RDONLY | O_BINARY);
    if (zio->file < 0) {
	zio->code = ZIO_INVALID;
	goto failed;
    }
    zio->location = 0;

    LOG(("out: zio_reopen() = %d", zio->file));
    return zio->file;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_reopen() = %d", -1));
    return -1;
}


/*
 * Open an non-compressed file.
 */
static int
zio_open_plain(zio, file_name)
    Zio *zio;
    const char *file_name;
{
    LOG(("in: zio_open_plain(zio=%d, file_name=%s)", (int)zio->id, file_name));

    zio->file = open(file_name, O_RDONLY | O_BINARY);
    if (zio->file < 0)
	goto failed;

    zio->code = ZIO_PLAIN;
    zio->file_size = lseek(zio->file, 0, SEEK_END);
    zio->slice_size = ZIO_SIZE_PAGE;
    if (zio->file_size < 0 || lseek(zio->file, 0, SEEK_SET) < 0)
	goto failed;

    /*
     * Assign ID.
     */
    pthread_mutex_lock(&zio_mutex);
    zio->id = zio_counter++;
    pthread_mutex_unlock(&zio_mutex);

    LOG(("out: zio_open_plain(zio=%d) = %d", (int)zio->id, zio->file));
    return zio->file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= zio->file)
	close(zio->file);
    zio->file = -1;
    zio->code = ZIO_INVALID;

    LOG(("out: zio_open_plain() = %d", -1));
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

    LOG(("in: zio_open_ebzip(zio=%d, file_name=%s)", (int)zio->id, file_name));

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

    /*
     * Assign ID.
     */
    pthread_mutex_lock(&zio_mutex);
    zio->id = zio_counter++;
    pthread_mutex_unlock(&zio_mutex);

    LOG(("out: zio_open_ebzip(zio=%d) = %d", (int)zio->id, zio->file));
    return zio->file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= zio->file)
	close(zio->file);
    zio->file = -1;
    zio->code = ZIO_INVALID;
    LOG(("out: zio_open_ebzip() = %d", -1));
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
    ssize_t read_length;
    Zio_Huffman_Node *tail_node_p;
    int i;

    LOG(("in: zio_open_epwing(zio=%d, file_name=%s)", (int)zio->id,
	file_name));

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

    /*
     * Assign ID.
     */
    pthread_mutex_lock(&zio_mutex);
    zio->id = zio_counter++;
    pthread_mutex_unlock(&zio_mutex);

    LOG(("out: zio_open_epwing(zio=%d) = %d", (int)zio->id, zio->file));
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

    LOG(("out: zio_open_epwing() = %d", -1));
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
    ssize_t read_length;
    Zio_Huffman_Node *tail_node_p;
    int i;

    LOG(("in: zio_open_epwing6(zio=%d, file_name=%s)", (int)zio->id,
	file_name));

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

    /*
     * Assign ID.
     */
    pthread_mutex_lock(&zio_mutex);
    zio->id = zio_counter++;
    pthread_mutex_unlock(&zio_mutex);

    LOG(("out: zio_open_epwing6(zio=%d) = %d", (int)zio->id, zio->file));
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

    LOG(("out: zio_open_epwing6() = %d", -1));
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

    LOG(("in: zio_make_epwing_huffman_tree(zio=%d, leaf_count=%d)",
	(int)zio->id, leaf_count));

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
	    goto failed;
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
	    goto failed;
	tail_node_p->right = least_node_p;
	tail_node_p->frequency += least_node_p->frequency;
	least_node_p->frequency = 0;

	tail_node_p++;
    }

    /*
     * Set a root node of the huffman tree.
     */ 
    zio->huffman_root = tail_node_p - 1;

    LOG(("out: zio_make_epwing_huffman_tree() = %d", 0));
    return 0;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_make_epwing_huffman_tree() = %d", -1));
    return -1;
}


/*
 * Close `zio'.
 */
void
zio_close(zio)
    Zio *zio;
{
    pthread_mutex_lock(&zio_mutex);
    LOG(("in: zio_close(zio=%d)", (int)zio->id));

    /*
     * If contents of the file is cached, clear the cache.
     */
    if (0 <= zio->file)
	close(zio->file);
    zio->file = -1;

    LOG(("out: zio_close()"));
    pthread_mutex_unlock(&zio_mutex);
}


/*
 * Return file descriptor of `zio'.
 */
int
zio_file(zio)
    Zio *zio;
{
    LOG(("in+out: zio_file(zio=%d) = %d", (int)zio->id, zio->file));

    return zio->file;
}


/*
 * Return compression mode of `zio'.
 */
Zio_Code
zio_mode(zio)
    Zio *zio;
{
    LOG(("in+out: zio_mode(zio=%d) = %d", (int)zio->id, zio->code));

    return zio->code;
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
    off_t result;

    LOG(("in: zio_lseek(zio=%d, location=%ld, whence=%d)",
	(int)zio->id, (long)location, whence));

    if (zio->file < 0)
	goto failed;

    if (zio->code == ZIO_PLAIN) {
	/*
	 * If `zio' is not compressed, simply call lseek().
	 */
	result = lseek(zio->file, location, whence);
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
	    goto failed;
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
	result = zio->location;
    }

    LOG(("out: zio_lseek() = %ld", (long)result));
    return result;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_lseek() = %ld", (long)-1));
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

    pthread_mutex_lock(&zio_mutex);
    LOG(("in: zio_read(zio=%d, length=%ld)", (int)zio->id, (long)length));

    /*
     * If the zio `file' is not compressed, call read() and return.
     */
    if (zio->file < 0)
	goto failed;

    switch (zio->code) {
    case ZIO_PLAIN:
	read_length = zio_read_raw(zio->file, buffer, length);
	break;
    case ZIO_EBZIP1:
	read_length = zio_read_ebzip(zio, buffer, length);
	break;
    case ZIO_EPWING:
    case ZIO_EPWING6:
	read_length = zio_read_epwing(zio, buffer, length);
	break;
    case ZIO_SEBXA:
	read_length = zio_read_sebxa(zio, buffer, length);
	break;
    default:
	goto failed;
    }

    LOG(("out: zio_read() = %ld", (long)read_length));
    pthread_mutex_unlock(&zio_mutex);

    return read_length;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_read() = %ld", (long)-1));
    return -1;
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
    char temporary_buffer[8];
    ssize_t read_length = 0;
    size_t zipped_slice_size;
    off_t slice_location;
    off_t next_slice_location;
    int n;
    
    LOG(("in: zio_read_ebzip(zio=%d, length=%ld)", (int)zio->id,
	(long)length));

    /*
     * Read data.
     */
    while (read_length < length) {
	if (zio->file_size <= zio->location)
	    goto succeeded;

	/*
	 * If data in `cache_buffer' is out of range, read data from
	 * `zio->file'.
	 */
	if (cache_zio_id != zio->id
	    || zio->location < cache_location 
	    || cache_location + zio->slice_size <= zio->location) {

	    cache_zio_id = ZIO_ID_NONE;
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
	    if (zio_unzip_slice_ebzip1(cache_buffer, zio->file,
		zio->slice_size, zipped_slice_size) < 0)
		goto failed;

	    cache_zio_id = zio->id;
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

  succeeded:
    LOG(("out: zio_read_ebzip() = %ld", (long)read_length));
    return read_length;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_read_ebzip() = %ld", (long)-1));
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
    ssize_t read_length = 0;
    off_t page_location;
    int n;
    
    LOG(("in: zio_read_epwing(zio=%d, length=%ld)", (int)zio->id,
	(long)length));

    /*
     * Read data.
     */
    while (read_length < length) {
	if (zio->file_size <= zio->location)
	    goto succeeded;

	/*
	 * If data in `cache_buffer' is out of range, read data from the zio
	 * file.
	 */
	if (cache_zio_id != zio->id
	    || zio->location < cache_location 
	    || cache_location + ZIO_SIZE_PAGE <= zio->location) {
	    cache_zio_id = ZIO_ID_NONE;
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

	    cache_zio_id = zio->id;
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

  succeeded:
    LOG(("out: zio_read_epwing() = %ld", (long)read_length));
    return read_length;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_read_epwing() = %ld", (long)-1));
    return -1;
}


#define ZIO_SEBXA_SLICE_LENGTH	4096

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
    char temporary_buffer[4];
    ssize_t read_length = 0;
    off_t slice_location;
    ssize_t n;
    int slice_index;

    LOG(("in: zio_read_sebxa(zio=%d, length=%ld)", (int)zio->id,
	(long)length));

    /*
     * Read data.
     */
    while (read_length < length) {
	if (zio->file_size <= zio->location)
	    goto succeeded;

	if (zio->location < zio->zio_start_location) {
	    /*
	     * Data is located in front of compressed text.
	     */
	    if (zio->zio_start_location - zio->location < length - read_length)
		n = zio->zio_start_location - zio->location;
	    else
		n = length - read_length;
	    if (lseek(zio->file, zio->location, SEEK_SET) < 0)
		goto failed;
	    if (zio_read_raw(zio->file, buffer, n) != n)
		goto failed;
	    read_length += n;

	} else if (zio->zio_end_location <= zio->location) {
	    /*
	     * Data is located behind compressed text.
	     */
	    if (lseek(zio->file, zio->location, SEEK_SET) < 0)
		goto failed;
	    if (zio_read_raw(zio->file, buffer, length - read_length)
		!= length - read_length)
		goto failed;
	    read_length = length;

	} else {
	    /*
	     * Data is located in compressed text.
	     * 
	     * If data in `cache_buffer' is out of range, read data from
	     * `file'.
	     */
	    if (cache_zio_id != zio->id
		|| zio->location < cache_location 
		|| cache_location + ZIO_SEBXA_SLICE_LENGTH <= zio->location) {

		cache_zio_id = ZIO_ID_NONE;
		cache_location = zio->location
		    - (zio->location % ZIO_SEBXA_SLICE_LENGTH);

		/*
		 * Get buffer location and size.
		 */
		slice_index = (zio->location - zio->zio_start_location)
		    / ZIO_SEBXA_SLICE_LENGTH;
		if (slice_index == 0)
		    slice_location = zio->index_base;
		else {
		    if (lseek(zio->file, (slice_index - 1) * 4
			+ zio->index_location, SEEK_SET) < 0)
			goto failed;
		    if (zio_read_raw(zio->file, temporary_buffer, 4) != 4)
			goto failed;
		    slice_location = zio->index_base 
			+ zio_uint4(temporary_buffer);
		}

		/*
		 * Read a compressed slice from `zio->file' and uncompress it.
		 */
		if (lseek(zio->file, slice_location, SEEK_SET) < 0)
		    goto failed;
		if (zio_unzip_slice_sebxa(cache_buffer, zio->file) < 0)
		    goto failed;

		cache_zio_id = zio->id;
	    }

	    /*
	     * Copy data from `cache_buffer' to `buffer'.
	     */
	    n = ZIO_SEBXA_SLICE_LENGTH
		- (zio->location % ZIO_SEBXA_SLICE_LENGTH);
	    if (length - read_length < n)
		n = length - read_length;
	    if (zio->file_size - zio->location < n)
		n = zio->file_size - zio->location;
	    memcpy(buffer + read_length,
		cache_buffer + (zio->location - cache_location), n);
	    read_length += n;
	    zio->location += n;
	}
    }

  succeeded:
    LOG(("out: zio_read_sebxa() = %ld", (long)read_length));
    return read_length;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_read_sebxa() = %ld", (long)-1));
    return -1;
}

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

    LOG(("in: zio_read_raw(file=%d, length=%ld)", file, (long)length));

    while (0 < rest_length) {
	errno = 0;
	n = read(file, buffer_p, rest_length);
	if (n < 0) {
	    if (errno == EINTR)
		continue;
	    goto failed;
	} else if (n == 0)
	    break;
	else {
	    rest_length -= n;
	    buffer_p += n;
	}
    }

    LOG(("out: zio_read_raw() = %ld", (long)(length - rest_length)));
    return length - rest_length;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_read_raw() = %ld", (long)-1));
    return -1;
}


/*
 * Uncompress an ebzip'ped slice.
 *
 * If it succeeds, 0 is returned.  Otherwise, -1 is returned.
 */
static int
zio_unzip_slice_ebzip1(out_buffer, in_file, slice_size, zipped_slice_size)
    char *out_buffer;
    int in_file;
    size_t slice_size;
    size_t zipped_slice_size;
{
    char temporary_buffer[ZIO_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL];
    z_stream stream;

    LOG(("in: zio_unzip_slice_ebzip1(in_file=%d, slice_size=%ld, \
zipped_slice_size=%ld)", 
	in_file, (long)slice_size, (long)zipped_slice_size));

    stream.zalloc = NULL;
    stream.zfree = NULL;
    stream.opaque = NULL;

    if (slice_size == zipped_slice_size) {
	if (zio_read_raw(in_file, out_buffer, slice_size) != slice_size)
	    goto failed;
	goto succeeded;
    }

    if (zio_read_raw(in_file, temporary_buffer, zipped_slice_size)
	!= zipped_slice_size)
	goto succeeded;

    if (inflateInit(&stream) != Z_OK)
	goto failed;
    
    stream.next_in = (Bytef *) temporary_buffer;
    stream.avail_in = zipped_slice_size;
    stream.next_out = (Bytef *) out_buffer;
    stream.avail_out = slice_size;

    if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
	goto failed;

    if (inflateEnd(&stream) != Z_OK)
	goto failed;

    if (stream.total_out < slice_size) {
#ifdef HAVE_MEMCPY
	memset(out_buffer + stream.total_out, 0,
	    slice_size - stream.total_out);
#else
	bzero(out_buffer + stream.total_out, slice_size - stream.total_out);
#endif
    }

  succeeded:
    LOG(("out: zio_unzip_slice_ebzip1() = %d", 0));
    return 0;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_unzip_slice_ebzip1() = %d", -1));
    return -1;
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
    ssize_t in_read_length;
    int in_bit_index;
    unsigned char *out_buffer_p;
    size_t out_length;

    LOG(("in: zio_unzip_slice_epwing(in_file=%d)", in_file));

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
		    goto failed;
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
		goto failed;

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
	    break;

	} else if (node_p->type == ZIO_HUFFMAN_NODE_LEAF16) {
	    /*
	     * The leaf is leaf16, decode 2 bytes character.
	     */
	    if (ZIO_SIZE_PAGE <= out_length)
		goto failed;
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
		goto failed;
	    else {
		*out_buffer_p++ = node_p->value;
		out_length++;
	    }
	}
    }

    LOG(("out: zio_unzip_slice_epwing() = %d", 0));
    return 0;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_unzip_slice_epwing() = %d", -1));
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
    ssize_t in_read_length;
    int in_bit_index;
    unsigned char *out_buffer_p;
    size_t out_length;
    int compression_type;

    LOG(("in: zio_unzip_slice_epwing6(in_file=%d)", in_file));

    in_buffer_p = (unsigned char *)in_buffer;
    in_bit_index = 7;
    in_read_length = 0;
    out_buffer_p = (unsigned char *)out_buffer;
    out_length = 0;

    /*
     * Get compression type.
     */
    if (zio_read_raw(in_file, in_buffer, 1) != 1)
	goto failed;
    compression_type = zio_uint1(in_buffer);

    /*
     * If compression type is not 0, this page is not compressed.
     */
    if (compression_type != 0) {
	if (zio_read_raw(in_file, out_buffer, ZIO_SIZE_PAGE) != 1)
	    goto failed;
	goto succeeded;
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
		    goto failed;
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
		goto failed;

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
	    break;

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

  succeeded:
    LOG(("out: zio_unzip_slice_epwing6() = %d", 0));
    return 0;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_unzip_slice_epwing6() = %d", -1));
    return -1;
}

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
    int copy_offset;
    int copy_length;
    int i, j;

    LOG(("in: zio_unzip_slice_sebxa(in_file=%d)", in_file));

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
		goto failed;
	    in_buffer_p = (unsigned char *)in_buffer;
	}

	/*
	 * The current input byte is recognized as compression flags
	 * for next 8 chunks.
	 */
	compression_flags[0] = !(*in_buffer_p & 0x01);
	compression_flags[1] = !(*in_buffer_p & 0x02);
	compression_flags[2] = !(*in_buffer_p & 0x04);
	compression_flags[3] = !(*in_buffer_p & 0x08);
	compression_flags[4] = !(*in_buffer_p & 0x10);
	compression_flags[5] = !(*in_buffer_p & 0x20);
	compression_flags[6] = !(*in_buffer_p & 0x40);
	compression_flags[7] = !(*in_buffer_p & 0x80);
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
		unsigned char c0, c1;

		if (in_read_rest <= 1)
		    goto failed;

		/*
		 * Get 2 bytes from the current input, and recognize
		 * them as following:
		 * 
		 *              *in_buffer_p   *(in_bufer_p + 1)
		 *  bit pattern: [AAAA|BBBB]   [CCCC|DDDD]
		 * 
		 *  copy_offset = ([CCCCAAAABBBB] + 18) % 4096
		 *  copy_length   = [DDDD] + 3
		 */
		c0 = *(unsigned char *)in_buffer_p;
		c1 = *((unsigned char *)in_buffer_p + 1);
		copy_offset = (((c1 & 0xf0) << 4) + c0 + 18)
		    % ZIO_SEBXA_SLICE_LENGTH;
		copy_length = (c1 & 0x0f) + 3;

		if (ZIO_SEBXA_SLICE_LENGTH < out_length + copy_length)
		    copy_length = ZIO_SEBXA_SLICE_LENGTH - out_length;

		if (copy_offset < out_length) {
		    copy_p = (unsigned char *)out_buffer + copy_offset;
		    for (j = 0; j < copy_length; j++)
			*out_buffer_p++ = *copy_p++;
		} else {
		    for (j = 0; j < copy_length; j++)
			*out_buffer_p++ = 0x00;
		}

		in_read_rest -= 2;
		in_buffer_p += 2;
		out_length += copy_length;

	    } else {
		/*
		 * This chunk is not compressed.
		 * Put the current input byte as a decoded value.
		 */
		if (in_read_rest <= 0)
		    goto failed;
		in_read_rest -= 1;
		*out_buffer_p++ = *in_buffer_p++;
		out_length += 1;
	    }

	    /*
	     * Return if the slice has been uncompressed.
	     */
	    if (ZIO_SEBXA_SLICE_LENGTH <= out_length)
		goto succeeded;
	}
    }

  succeeded:
    LOG(("out: zio_unzip_slice_sebxa() = %d", 0));
    return 0;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: zio_unzip_slice_sebxa() = %d", -1));
    return -1;
}
