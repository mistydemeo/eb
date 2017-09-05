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
#define PATH_MAX	MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX	1024
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

#include "eb.h"
#include "error.h"
#include "internal.h"


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
static off_t cache_offset;

/*
 * Mutex for cache variables.
 */
#ifdef ENABLE_PTHREAD
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * Unexported function.
 */
static int eb_make_epwing_huffman_tree EB_P((EB_Zip *, int));
static ssize_t eb_zread_ebzip EB_P((EB_Zip *, int, char *, size_t));
static ssize_t eb_zread_epwing EB_P((EB_Zip *, int, char *, size_t));

/*
 * Initialize `cache_buffer'.
 */
int
eb_zinitialize()
{
    pthread_mutex_lock(&cache_mutex);

    /*
     * Allocate memory for cache buffer.
     */
    if (cache_buffer == NULL) {
	cache_buffer = (char *) malloc(EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL);
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
 * Clear `cache_buffer'.
 */
void
eb_zfinalize()
{
    pthread_mutex_lock(&cache_mutex);

    if (cache_buffer != NULL)
	free(cache_buffer);
    cache_buffer = NULL;
    cache_file = -1;

    pthread_mutex_unlock(&cache_mutex);
}


/*
 * Open an non-compressed file.
 */
int
eb_zopen_none(zip, file_name)
    EB_Zip *zip;
    const char *file_name;
{
    int file;

    file = open(file_name, O_RDONLY | O_BINARY);
    if (0 <= file) {
	zip->code = EB_ZIP_NONE;
	return file;
    }

    return -1;
}


/*
 * Open an EBZIP compression file.
 */
int
eb_zopen_ebzip(zip, file_name)
    EB_Zip *zip;
    const char *file_name;
{
    char header[EB_SIZE_EBZIP_HEADER];
    int file = -1;

    /*
     * Try to open a `*.ebz' file.
     */
    file = open(file_name, O_RDONLY | O_BINARY);
    if (file < 0)
	goto failed;

    /*
     * Read header part of the ebzip'ped file.
     */
    if (eb_read_all(file, header, EB_SIZE_EBZIP_HEADER)
	!= EB_SIZE_EBZIP_HEADER)
	goto failed;
    zip->code = eb_uint1(header + 5) >> 4;
    zip->zip_level = eb_uint1(header + 5) & 0x0f;
    zip->slice_size = EB_SIZE_PAGE << zip->zip_level;
    zip->file_size = eb_uint4(header + 10);
    zip->crc = eb_uint4(header + 14);
    zip->mtime = eb_uint4(header + 18);
    zip->offset = 0;

    if (zip->file_size < 1 << 16)
	zip->index_width = 2;
    else if (zip->file_size < 1 << 24)
	zip->index_width = 3;
    else
	zip->index_width = 4;

    /*
     * Check zip header information.
     */
    if (memcmp(header, "EBZip", 5) != 0
	|| zip->code != EB_ZIP_EBZIP1
	|| EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL < zip->slice_size)
	goto failed;

    return file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);
    return -1;
}


/*
 * The buffer size must be 512 bytes, the number of 8 bit nodes.
 */
#define EPWING_BUFFER_SIZE 512

/*
 * Open an EPWING compression file.
 */
int
eb_zopen_epwing(zip, file_name)
    EB_Zip *zip;
    const char *file_name;
{
    int file = -1;
    int leaf16_count;
    int leaf_count;
    char buffer[EPWING_BUFFER_SIZE];
    char *buffer_p;
    size_t read_length;
    EB_Huffman_Node *tail_node_p;
    int i;

    zip->code = EB_ZIP_EPWING;
    zip->huffman_nodes = NULL;

    /*
     * Open `HONMON2'.
     */
    file = open(file_name, O_RDONLY | O_BINARY);
    if (file < 0)
	goto failed;

    /*
     * Read a header of `HONMON2' (32 bytes).
     * When `frequencies_length' is shorter than 512, we assumes the
     * file is broken.
     */
    if (eb_read_all(file, buffer, 32) != 32)
	goto failed;
    zip->offset = 0;
    zip->slice_size = EB_SIZE_PAGE;
    zip->index_location = eb_uint4(buffer);
    zip->index_length = eb_uint4(buffer + 4);
    zip->frequencies_location = eb_uint4(buffer + 8);
    zip->frequencies_length = eb_uint4(buffer + 12);
    leaf16_count = (zip->frequencies_length - (256 * 2)) / 4;
    leaf_count = leaf16_count + 256 + 1;
    if (zip->index_length < 36 || zip->frequencies_length < 512)
	goto failed;

    /*
     * Check for the length of an uncompressed file.
     *
     * If the index of the non-first page in the last index group
     * is 0x0000, we assumes the data corresponding with the index
     * doesn't exist.
     */
    if (lseek(file, zip->index_location + (zip->index_length - 36) / 36 * 36,
	SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, 36) != 36)
	goto failed;
    zip->file_size = (zip->index_length / 36) * (EB_SIZE_PAGE * 16);
    for (i = 1, buffer_p = buffer + 4 + 2; i < 16; i++, buffer_p += 2) {
	if (eb_uint2(buffer_p) == 0)
	    break;
    }
    zip->file_size -= EB_SIZE_PAGE * (16 - i);
    
    /*
     * Allocate memory for huffman nodes.
     */
    zip->huffman_nodes = (EB_Huffman_Node *) malloc(sizeof(EB_Huffman_Node)
	* leaf_count * 2);
    if (zip->huffman_nodes == NULL)
	goto failed;
    tail_node_p = zip->huffman_nodes;

    /*
     * Make leafs for 16bit character.
     */
    if (lseek(file, zip->frequencies_location, SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, read_length) != read_length)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < leaf16_count; i++) {
	if (buffer + read_length <= buffer_p) {
	    if (eb_read_all(file, buffer, read_length) != read_length)
		goto failed;
	    buffer_p = buffer;
	}
	tail_node_p->type = EB_HUFFMAN_NODE_LEAF16;
	tail_node_p->value = eb_uint2(buffer_p);
	tail_node_p->frequency = eb_uint2(buffer_p + 2);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 4;
	tail_node_p++;
    }

    /*
     * Make leafs for 8bit character.
     */
    if (lseek(file, zip->frequencies_location + leaf16_count * 4, SEEK_SET)
	< 0)
	goto failed;
    if (eb_read_all(file, buffer, 512) != 512)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < 256; i++) {
	tail_node_p->type = EB_HUFFMAN_NODE_LEAF8;
	tail_node_p->value = i;
	tail_node_p->frequency = eb_uint2(buffer_p);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 2;
	tail_node_p++;
    }

    /*
     * Make a leaf for the end-of-page character.
     */
    tail_node_p->type = EB_HUFFMAN_NODE_EOF;
    tail_node_p->value = 256;
    tail_node_p->frequency = 1;
    tail_node_p++;
    
    /*
     * Make a huffman tree.
     */
    if (eb_make_epwing_huffman_tree(zip, leaf_count) < 0)
	goto failed;

    return file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);
    if (zip->huffman_nodes != NULL)
	free(zip->huffman_nodes);
    zip->huffman_root = NULL;

    return -1;
}


/*
 * Open an EPWING compression file.
 */
int
eb_zopen_epwing6(zip, file_name)
    EB_Zip *zip;
    const char *file_name;
{
    int file = -1;
    int leaf32_count;
    int leaf16_count;
    int leaf_count;
    char buffer[EPWING_BUFFER_SIZE];
    char *buffer_p;
    size_t read_length;
    EB_Huffman_Node *tail_node_p;
    int i;

    zip->code = EB_ZIP_EPWING6;
    zip->huffman_nodes = NULL;

    /*
     * Open `HONMON2'.
     */
    file = open(file_name, O_RDONLY | O_BINARY);
    if (file < 0)
	goto failed;

    /*
     * Read a header of `HONMON2' (48 bytes).
     * When `frequencies_length' is shorter than 512, we assumes the
     * file is broken.
     */
    if (eb_read_all(file, buffer, 48) != 48)
	goto failed;
    zip->offset = 0;
    zip->slice_size = EB_SIZE_PAGE;
    zip->index_location = eb_uint4(buffer);
    zip->index_length = eb_uint4(buffer + 4);
    zip->frequencies_location = eb_uint4(buffer + 8);
    zip->frequencies_length = eb_uint4(buffer + 12);
    leaf16_count = 0x400;
    leaf32_count = (zip->frequencies_length - (leaf16_count * 4) - (256 * 2))
	/ 6;
    leaf_count = leaf32_count + leaf16_count + 256 + 1;
    if (zip->index_length < 36 || zip->frequencies_length < 512)
	goto failed;

    /*
     * Check for the length of an uncompressed file.
     *
     * If the index of the non-first page in the last index group
     * is 0x0000, we assumes the data corresponding with the index
     * doesn't exist.
     */
    if (lseek(file, zip->index_location + (zip->index_length - 36) / 36 * 36,
	SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, 36) != 36)
	goto failed;
    zip->file_size = (zip->index_length / 36) * (EB_SIZE_PAGE * 16);
    for (i = 1, buffer_p = buffer + 4 + 2; i < 16; i++, buffer_p += 2) {
	if (eb_uint2(buffer_p) == 0)
	    break;
    }
    zip->file_size -= EB_SIZE_PAGE * (16 - i);
    
    /*
     * Allocate memory for huffman nodes.
     */
    zip->huffman_nodes = (EB_Huffman_Node *) malloc(sizeof(EB_Huffman_Node)
	* leaf_count * 2);
    if (zip->huffman_nodes == NULL)
	goto failed;
    tail_node_p = zip->huffman_nodes;

    /*
     * Make leafs for 32bit character.
     */
    read_length = EPWING_BUFFER_SIZE - (EPWING_BUFFER_SIZE % 6);
    if (lseek(file, zip->frequencies_location, SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, read_length) != read_length)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < leaf32_count; i++) {
	if (buffer + read_length <= buffer_p) {
	    if (eb_read_all(file, buffer, read_length) != read_length)
		goto failed;
	    buffer_p = buffer;
	}
	tail_node_p->type = EB_HUFFMAN_NODE_LEAF32;
	tail_node_p->value = eb_uint4(buffer_p);
	tail_node_p->frequency = eb_uint2(buffer_p + 4);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 6;
	tail_node_p++;
    }

    /*
     * Make leafs for 16bit character.
     */
    read_length = EPWING_BUFFER_SIZE - (EPWING_BUFFER_SIZE % 4);
    if (lseek(file, zip->frequencies_location + leaf32_count * 6, SEEK_SET)
	< 0)
	goto failed;
    if (eb_read_all(file, buffer, read_length) != read_length)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < leaf16_count; i++) {
	if (buffer + read_length <= buffer_p) {
	    if (eb_read_all(file, buffer, read_length) != read_length)
		goto failed;
	    buffer_p = buffer;
	}
	tail_node_p->type = EB_HUFFMAN_NODE_LEAF16;
	tail_node_p->value = eb_uint2(buffer_p);
	tail_node_p->frequency = eb_uint2(buffer_p + 2);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 4;
	tail_node_p++;
    }

    /*
     * Make leafs for 8bit character.
     */
    if (lseek(file,
	zip->frequencies_location + leaf32_count * 6 + leaf16_count * 4,
	SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, 512) != 512)
	goto failed;

    buffer_p = buffer;
    for (i = 0; i < 256; i++) {
	tail_node_p->type = EB_HUFFMAN_NODE_LEAF8;
	tail_node_p->value = i;
	tail_node_p->frequency = eb_uint2(buffer_p);
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;
	buffer_p += 2;
	tail_node_p++;
    }

    /*
     * Make a leaf for the end-of-page character.
     */
    tail_node_p->type = EB_HUFFMAN_NODE_EOF;
    tail_node_p->value = 256;
    tail_node_p->frequency = 1;
    tail_node_p++;

    /*
     * Make a huffman tree.
     */
    if (eb_make_epwing_huffman_tree(zip, leaf_count) < 0)
	goto failed;

    return file;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);
    if (zip->huffman_nodes != NULL)
	free(zip->huffman_nodes);
    zip->huffman_root = NULL;

    return -1;
}


/*
 * Make a huffman tree for decompressing EPWING compression data.
 */
static int
eb_make_epwing_huffman_tree(zip, leaf_count)
    EB_Zip *zip;
    int leaf_count;
{    
    EB_Huffman_Node *target_node;
    EB_Huffman_Node *most_node;
    EB_Huffman_Node *node_p;
    EB_Huffman_Node temporary_node;
    EB_Huffman_Node *least_node_p;
    EB_Huffman_Node *tail_node_p;
    int i;
    int j;

    tail_node_p = zip->huffman_nodes + leaf_count;

    /*
     * Sort the leaf nodes in frequency order.
     */
    for (i = 0; i < leaf_count - 1; i++) {
        target_node = zip->huffman_nodes + i;
        most_node = target_node;
        node_p = zip->huffman_nodes + i + 1;
  
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
	tail_node_p->type = EB_HUFFMAN_NODE_INTERMEDIATE;
	tail_node_p->left = NULL;
	tail_node_p->right = NULL;

	/*
	 * Find for a least frequent node.
	 * That node becomes a left child of the new intermediate node.
	 */
	least_node_p = NULL;
	for (node_p = zip->huffman_nodes; node_p < tail_node_p; node_p++) {
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
	for (node_p = zip->huffman_nodes; node_p < tail_node_p; node_p++) {
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
    zip->huffman_root = tail_node_p - 1;

    return 0;
}


/*
 * Close `file'.  (`file' may have been compressed.)
 */
int
eb_zclose(zip, file)
    EB_Zip *zip;
    int file;
{
    pthread_mutex_lock(&cache_mutex);

    /*
     * If contents of the file is cached, clear the cache.
     */
    if (cache_file == file)
	cache_file = -1;

    /*
     * Dispose huffman nodes.
     */
    if (zip->code == EB_ZIP_EPWING) {
	free(zip->huffman_nodes);
	zip->huffman_nodes = NULL;
	zip->huffman_root = NULL;
    }

    pthread_mutex_unlock(&cache_mutex);
    return close(file);
}


/*
 * Seek `file'.  (`file' may have been compressed.)
 */
off_t
eb_zlseek(zip, file, offset, whence)
    EB_Zip *zip;
    int file;
    off_t offset;
    int whence;
{
    if (zip->code == EB_ZIP_NONE)
	/*
	 * If `file' is not compressed, simply call lseek().
	 */
	return lseek(file, offset, whence);
    else {
	/*
	 * Calculate new offset according with `whence'.
	 */
	switch (whence) {
	case SEEK_SET:
	    zip->offset = offset;
	    break;
	case SEEK_CUR:
	    zip->offset = zip->offset + offset;
	    break;
	case SEEK_END:
	    zip->offset = zip->file_size - offset;
	    break;
	default:
#ifdef EINVAL
	    errno = EINVAL;
#endif
	    return -1;
	}

	/*
	 * Adjust offset.
	 */
	if (zip->offset < 0)
	    zip->offset = 0;
	if (zip->file_size < zip->offset)
	    zip->offset = zip->file_size;

	/*
	 * Update `zip->offset'.
	 * (We don't actually seek the file.)
	 */
	return zip->offset;
    }

    /* never reached */
    return -1;
}


/*
 * Read data from `file'.  (`file' may have been compressed.)
 */
ssize_t
eb_zread(zip, file, buffer, length)
    EB_Zip *zip;
    int file;
    char *buffer;
    size_t length;
{
    ssize_t read_length;

    /*
     * If `file' is not compressed, call read() and return.
     */
    if (zip->code == EB_ZIP_NONE)
	read_length = eb_read_all(file, buffer, length);
    else if (zip->code == EB_ZIP_EBZIP1)
	read_length = eb_zread_ebzip(zip, file, buffer, length);
    else if (zip->code == EB_ZIP_EPWING)
	read_length = eb_zread_epwing(zip, file, buffer, length);
    else if (zip->code == EB_ZIP_EPWING6)
	read_length = eb_zread_epwing(zip, file, buffer, length);
    else
	read_length = -1;

    return read_length;
}


/*
 * Read data from `file' compressed with the ebzip compression format.
 */
static ssize_t
eb_zread_ebzip(zip, file, buffer, length)
    EB_Zip *zip;
    int file;
    char *buffer;
    size_t length;
{
    char temporary_buffer[EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL];
    size_t read_length = 0;
    size_t zipped_slice_size;
    off_t slice_location;
    off_t next_slice_location;
    int n;
    
    pthread_mutex_lock(&cache_mutex);

    /*
     * Read data.
     */
    while (read_length < length) {
	if (zip->file_size <= zip->offset)
	    goto succeeded;

	/*
	 * If data in `cache_buffer' is out of range, read data from `file'.
	 */
	if (cache_file != file || zip->offset < cache_offset 
	    || cache_offset + zip->slice_size <= zip->offset) {

	    cache_file = -1;
	    cache_offset = zip->offset - (zip->offset % zip->slice_size);

	    /*
	     * Get buffer location and size from index table in `file'.
	     */
	    if (lseek(file, zip->offset / zip->slice_size * zip->index_width
		+ EB_SIZE_EBZIP_HEADER, SEEK_SET) < 0)
		goto failed;
	    if (eb_read_all(file, temporary_buffer, zip->index_width * 2)
		!= zip->index_width * 2)
		goto failed;

	    switch (zip->index_width) {
	    case 2:
		slice_location = eb_uint2(temporary_buffer);
		next_slice_location = eb_uint2(temporary_buffer + 2);
		break;
	    case 3:
		slice_location = eb_uint3(temporary_buffer);
		next_slice_location = eb_uint3(temporary_buffer + 3);
		break;
	    case 4:
		slice_location = eb_uint4(temporary_buffer);
		next_slice_location = eb_uint4(temporary_buffer + 4);
		break;
	    default:
		goto failed;
	    }
	    zipped_slice_size = next_slice_location - slice_location;

	    if (next_slice_location <= slice_location
		|| zip->slice_size < zipped_slice_size)
		goto failed;

	    /*
	     * Read a compressed slice from `file' and uncompress it.
	     * The data is not compressed if its size is equals to
	     * slice size.
	     */
	    if (lseek(file, slice_location, SEEK_SET) < 0)
		goto failed;
	    if (eb_read_all(file, temporary_buffer, zipped_slice_size)
		!= zipped_slice_size)
		goto failed;
	    if (zip->slice_size == zipped_slice_size)
		memcpy(cache_buffer, temporary_buffer, zip->slice_size);
	    else if (eb_unzip_slice_ebzip1(cache_buffer, zip->slice_size,
		temporary_buffer, zipped_slice_size) < 0)
		goto failed;

	    cache_file = file;
	}

	/*
	 * Copy data from `cache_buffer' to `buffer'.
	 */
	n = zip->slice_size - (zip->offset % zip->slice_size);
	if (length - read_length < n)
	    n = length - read_length;
	if (zip->file_size - zip->offset < n)
	    n = zip->file_size - zip->offset;
	memcpy(buffer + read_length,
	    cache_buffer + (zip->offset - cache_offset), n);
	read_length += n;
	zip->offset += n;
    }

  succeeded:
    pthread_mutex_unlock(&cache_mutex);
    return read_length;

    /*
     * An error occurs...
     */
  failed:
    pthread_mutex_unlock(&cache_mutex);
    return -1;
}


/*
 * Read data from `file' compressed with the EPWING or EPWING V6 
 * compression format.
 */
static ssize_t
eb_zread_epwing(zip, file, buffer, length)
    EB_Zip *zip;
    int file;
    char *buffer;
    size_t length;
{
    char temporary_buffer[36];
    size_t read_length = 0;
    off_t page_location;
    int n;
    
    pthread_mutex_lock(&cache_mutex);

    /*
     * Read data.
     */
    while (read_length < length) {
	if (zip->file_size <= zip->offset)
	    goto succeeded;

	/*
	 * If data in `cache_buffer' is out of range, read data from `file'.
	 */
	if (cache_file != file || zip->offset < cache_offset 
	    || cache_offset + EB_SIZE_PAGE <= zip->offset) {
	    cache_file = -1;
	    cache_offset = zip->offset - (zip->offset % EB_SIZE_PAGE);

	    /*
	     * Get page location from index table in `file'.
	     */
	    if (lseek(file, zip->index_location
		+ zip->offset / (EB_SIZE_PAGE * 16) * 36, SEEK_SET) < 0)
		goto failed;
	    if (eb_read_all(file, temporary_buffer, 36) != 36)
		goto failed;
	    page_location = eb_uint4(temporary_buffer)
		+ eb_uint2(temporary_buffer + 4
		    + (zip->offset / EB_SIZE_PAGE % 16) * 2);

	    /*
	     * Read a compressed page from `file' and uncompress it.
	     */
	    if (lseek(file, page_location, SEEK_SET) < 0)
		goto failed;
	    if (zip->code == EB_ZIP_EPWING) {
		if (eb_unzip_slice_epwing(cache_buffer, file,
		    zip->huffman_root) < 0)
		    goto failed;
	    } else {
		if (eb_unzip_slice_epwing6(cache_buffer, file,
		    zip->huffman_root) < 0)
		    goto failed;
	    }

	    cache_file = file;
	}

	/*
	 * Copy data from `cache_buffer' to `buffer'.
	 */
	n = EB_SIZE_PAGE - (zip->offset % EB_SIZE_PAGE);
	if (length - read_length < n)
	    n = length - read_length;
	if (zip->file_size - zip->offset < n)
	    n = zip->file_size - zip->offset;
	memcpy(buffer + read_length,
	    cache_buffer + (zip->offset - cache_offset), n);
	read_length += n;
	zip->offset += n;
    }

  succeeded:
    pthread_mutex_unlock(&cache_mutex);
    return read_length;

    /*
     * An error occurs...
     */
  failed:
    pthread_mutex_unlock(&cache_mutex);
    return -1;
}


/*
 * Read data from a file.
 * It repeats to call read() until all data will have been read.
 */
ssize_t
eb_read_all(file, buffer, length)
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
