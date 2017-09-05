/*                                                            -*- C -*-
 * Copyright (c) 1998, 1999  Motoyuki Kasahara
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

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

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
 * Unexported function.
 */
static int eb_zopen_ebzipped EB_P((EB_Zip *zip, const char *));
static int eb_zopen_epwzipped EB_P((EB_Zip *zip, const char *));
static ssize_t eb_zread_ebzipped EB_P((EB_Zip *, int, char *, size_t));
static ssize_t eb_zread_epwzipped EB_P((EB_Zip *, int, char *, size_t));

/*
 * Clear `cache_buffer'.
 */
void
eb_zclear()
{
    if (cache_buffer != NULL)
	free(cache_buffer);
    cache_buffer = NULL;
    cache_file = -1;
}


/*
 * Open `filename' for reading.
 * The file may have been compressed.
 * We also adjust the suffix of the filename.
 */
int
eb_zopen(zip, filename)
    EB_Zip *zip;
    const char *filename;
{
    char zipped_filename[PATH_MAX + 1];
    size_t filename_length;
    EB_Case_Code fcase;
    const char *p;
    int file;

    /*
     * Try to open a normal file.
     * If succeed, return immediately.
     */
    file = open(filename, O_RDONLY | O_BINARY);
    if (0 <= file) {
	zip->code = EB_ZIP_NONE;
	return file;
    }

    /*
     * Determine the letter case of the suffix of the zip filename.
     */
    filename_length = strlen(filename);
    for (p = filename, fcase = EB_CASE_UPPER; *p != '\0'; p++) {
	if ('A' <= *p && *p <= 'Z')
	    fcase = EB_CASE_UPPER;
	else if ('a' <= *p && *p <= 'z')
	    fcase = EB_CASE_LOWER;
    }
	
    /*
     * Copy `filename' to `zipped_filename' and appennd a suffix (`.EBZ'
     * or `.ebz') to `zipped_filename'.
     *
     * Though the length of `zipped_filename' is increased, it never
     * overrun the buffer (PATH_MAX).  See eb_bind().
     */
    strcpy(zipped_filename, filename);
    if (*(filename + filename_length - 3) == '.') {
	if (fcase == EB_CASE_UPPER)
	    strcpy(zipped_filename + filename_length - 2, "EBZ;1");
	else
	    strcpy(zipped_filename + filename_length - 2, "ebz;1");
    } else if (*(filename + filename_length - 2) == ';') {
	if (fcase == EB_CASE_UPPER)
	    strcpy(zipped_filename + filename_length - 2, ".EBZ;1");
	else
	    strcpy(zipped_filename + filename_length - 2, ".ebz;1");
    } else if (*(filename + filename_length - 1) == '.') {
	if (fcase == EB_CASE_UPPER)
	    strcpy(zipped_filename + filename_length, "EBZ");
	else 
	    strcpy(zipped_filename + filename_length, "ebz");
    } else {
	if (fcase == EB_CASE_UPPER)
	    strcpy(zipped_filename + filename_length, ".EBZ");
	else 
	    strcpy(zipped_filename + filename_length, ".ebz");
    }

    /*
     * Try to open a `*.EBZ' file.
     */
    file = eb_zopen_ebzipped(zip, zipped_filename);
    if (0 <= file)
	return file;

    /*
     * Check for the basename of `filename'.
     * It must be `.../DATA/HONMON' or `.../data/honmon'.
     * If it is, add the suffix `2' to `zipped_filename'
     * (e.g. `HONMON;1' -> `HONMON2;1').
     */
    strcpy(zipped_filename, filename);
    if (12 <= filename_length
	&& (strcmp(filename + filename_length - 12, "/DATA/HONMON") == 0
	    || strcmp(filename + filename_length - 12, "/data/honmon")
	    == 0)) {
	strcpy(zipped_filename + filename_length, "2");
    } else if (13 <= filename_length
	&& (strcmp(filename + filename_length - 13, "/DATA/HONMON.") == 0
	    || strcmp(filename + filename_length - 13, "/data/honmon.")
	    == 0)) {
	strcpy(zipped_filename + filename_length - 1, "2.");
    } else if (14 <= filename_length
	&& (strcmp(filename + filename_length - 14, "/DATA/HONMON;1") == 0
	    || strcmp(filename + filename_length - 14, "/data/honmon;1")
	    == 0)) {
	strcpy(zipped_filename + filename_length - 2, "2;1");
    } else if (15 <= filename_length
	&& (strcmp(filename + filename_length - 15, "/DATA/HONMON.;1") == 0
	    || strcmp(filename + filename_length - 15, "/data/honmon.;1")
	    == 0)) {
	strcpy(zipped_filename + filename_length - 3, "2;1");
    } else {
	zipped_filename[0] = '\0';
    }

    /*
     * Try to open a `HONMON2' file instead of `HONMON'.
     */
    if (zipped_filename[0] != '\0') {
	file = eb_zopen_epwzipped(zip, zipped_filename);
	if (0 <= file)
	    return file;
    }

    /*
     * In some CD-ROM book discs, filename suffix is inconsistent.
     * We remove or append `.' to the filename and try to open the
     * file again.
     */
    strcpy(zipped_filename, filename);
    if (*(filename + filename_length - 3) == '.') {
	*(zipped_filename + filename_length - 3) = ';';
	*(zipped_filename + filename_length - 2) = '1';
	*(zipped_filename + filename_length - 1) = '\0';
    } else if (*(filename + filename_length - 2) == ';') {
	*(zipped_filename + filename_length - 2) = '.';
	*(zipped_filename + filename_length - 1) = ';';
	*(zipped_filename + filename_length)     = '1';
	*(zipped_filename + filename_length + 1) = '\0';
    } else if (*(filename + filename_length - 1) == '.') {
	*(zipped_filename + filename_length - 1) = '\0';
    } else {
	*(zipped_filename + filename_length)     = '.';
	*(zipped_filename + filename_length + 1) = '\0';
    }
    file = open(filename, O_RDONLY | O_BINARY);
    if (0 <= file) {
	zip->code = EB_ZIP_NONE;
	return file;
    }

    return -1;
}


/*
 * Open `filename' for reading.
 * The file may have been compressed.
 * Unlink `eb_zopen', we never adjust the suffix of the filename.
 */
int
eb_zopen2(zip, filename)
    EB_Zip *zip;
    const char *filename;
{
    int file;
    int is_ebzipped;
    int is_epwzipped;
    size_t filename_length;

    filename_length = strlen(filename);

    /*
     * Try to open a `*.EBZ' file.
     */
    is_ebzipped = 0;
    if (4 <= filename_length
	&& (strcmp(filename + filename_length - 4, ".EBZ") == 0
	    || strcmp(filename + filename_length - 4, ".ebz") == 0)) {
	is_ebzipped = 1;
    } else if (6 <= filename_length
	&& (strcmp(filename + filename_length - 6, ".EBZ;1") == 0
	    || strcmp(filename + filename_length - 6, ".ebz;1") == 0)) {
	is_ebzipped = 1;
    }
    if (is_ebzipped) {
	file = eb_zopen_ebzipped(zip, filename);
	return file;
    }

    /*
     * Try to open a `HONMON2' file instead of `HONMON'.
     */
    is_epwzipped = 0;
    if (13 <= filename_length
	&& (strcmp(filename + filename_length - 13, "/DATA/HONMON2") == 0
	    || strcmp(filename + filename_length - 13, "/data/honmon2")
	    == 0)) {
	is_epwzipped = 1;
    } else if (14 <= filename_length
	&& (strcmp(filename + filename_length - 14, "/DATA/HONMON2.") == 0
	    || strcmp(filename + filename_length - 14, "/data/honmon2.")
	    == 0)) {
	is_epwzipped = 1;
    } else if (15 <= filename_length
	&& (strcmp(filename + filename_length - 15, "/DATA/HONMON2;1") == 0
	    || strcmp(filename + filename_length - 15, "/data/honmon2;1")
	    == 0)) {
	is_epwzipped = 1;
    } else if (16 <= filename_length
	&& (strcmp(filename + filename_length - 16, "/DATA/HONMON2.;1") == 0
	    || strcmp(filename + filename_length - 16, "/data/honmon2.;1")
	    == 0)) {
	is_epwzipped = 1;
    }
    if (is_epwzipped) {
	file = eb_zopen_epwzipped(zip, filename);
	return file;
    }

    /*
     * Try to open a normal file.
     * If succeed, return immediately.
     */
    file = open(filename, O_RDONLY | O_BINARY);
    if (0 <= file) {
	zip->code = EB_ZIP_NONE;
	zip->slice_size = EB_SIZE_PAGE;
	zip->file_size = lseek(file, 0, SEEK_END);
	if (zip->file_size < 0 || lseek(file, 0, SEEK_SET) < 0) {
	    close(file);
	    return -1;
	}
	return file;
    }

    return -1;
}


/*
 * Try to open an EBZIP compression file `*.EBZ'.
 */
static int
eb_zopen_ebzipped(zip, filename)
    EB_Zip *zip;
    const char *filename;
{
    char header[EB_SIZE_EBZIP_HEADER];
    int file = -1;

    /*
     * Try to open a `*.ebz' file.
     */
    file = open(filename, O_RDONLY | O_BINARY);
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
    if (memcmp(header, "EBZip", 5) != 0 || zip->code != EB_ZIP_EBZIP1
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
 * The buffer size must be 512 bytes or larger, and multiple of 4.
 */
#define EPWZIP_BUFFER_SIZE 512

/*
 * Try to open an EPWING compression file `HONMON2'.
 */
static int
eb_zopen_epwzipped(zip, filename)
    EB_Zip *zip;
    const char *filename;
{
    int file = -1;
    int leaf16_count;
    int leaf_count;
    char buffer[EPWZIP_BUFFER_SIZE];
    char *bufp;
    EB_Huffman_Node *tail_nodep;
    int i;

    /*
     * Initialize `huffman_nodes' in `zip'.
     */
    zip->huffman_nodes = NULL;

    /*
     * Open `HONMON2'.
     */
    file = open(filename, O_RDONLY | O_BINARY);
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
     *
     */
    if (lseek(file, zip->index_location + zip->index_length / 36 * 36,
	SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, 36) != 36)
	goto failed;
    zip->file_size = (zip->index_length / 36) * (EB_SIZE_PAGE * 16);
    for (i = 1, bufp = buffer + 4 + 2; i < 16; i++, bufp += 2) {
	if (eb_uint2(bufp) == 0)
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
    tail_nodep = zip->huffman_nodes;

    /*
     * Make leafs for 16bit character.
     */
    if (lseek(file, zip->frequencies_location, SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, EPWZIP_BUFFER_SIZE) != EPWZIP_BUFFER_SIZE)
	goto failed;

    bufp = buffer;
    for (i = 0; i < leaf16_count; i++) {
	if (buffer + EPWZIP_BUFFER_SIZE <= bufp) {
	    if (eb_read_all(file, buffer, EPWZIP_BUFFER_SIZE)
		!= EPWZIP_BUFFER_SIZE)
		goto failed;
	    bufp = buffer;
	}
	tail_nodep->type = EB_HUFFMAN_NODE_LEAF16;
	tail_nodep->value = eb_uint2(bufp);
	tail_nodep->frequency = eb_uint2(bufp + 2);
	tail_nodep->left = NULL;
	tail_nodep->right = NULL;
	bufp += 4;
	tail_nodep++;
    }

    /*
     * Make leafs for 8bit character.
     */
    if (lseek(file, zip->frequencies_location + zip->frequencies_length - 512,
	SEEK_SET) < 0)
	goto failed;
    if (eb_read_all(file, buffer, 512) != 512)
	goto failed;

    bufp = buffer;
    for (i = 0; i < 256; i++) {
	tail_nodep->type = EB_HUFFMAN_NODE_LEAF8;
	tail_nodep->value = i;
	tail_nodep->frequency = eb_uint2(bufp);
	tail_nodep->left = NULL;
	tail_nodep->right = NULL;
	bufp += 2;
	tail_nodep++;
    }

    /*
     * Make a leaf for the end-of-page character.
     */
    tail_nodep->type = EB_HUFFMAN_NODE_EOF;
    tail_nodep->value = 256;
    tail_nodep->frequency = 1;
    tail_nodep++;
    
    /*
     * Sort the leaf nodes in frequency order.
     */
    for (i = 0; i < leaf_count - 1; i++) {
        EB_Huffman_Node *target_node = zip->huffman_nodes + i;
        EB_Huffman_Node *most_node = target_node;
        EB_Huffman_Node *nodep = zip->huffman_nodes + i + 1;
        EB_Huffman_Node tmp_node;
	int j;
  
        for (j = i + 1; j < leaf_count; j++) {
            if (most_node->frequency < nodep->frequency)
                most_node = nodep;
            nodep++;
        } 

        tmp_node.type = most_node->type;
        tmp_node.value = most_node->value;
        tmp_node.frequency = most_node->frequency;

        most_node->type = target_node->type;
        most_node->value = target_node->value;
        most_node->frequency = target_node->frequency;

        target_node->type = tmp_node.type;
        target_node->value = tmp_node.value;
        target_node->frequency = tmp_node.frequency;
    }

    /*
     * Make intermediate nodes of the huffman tree.
     * The number of intermediate nodes of the tree is <the number of
     * leaf nodes> - 1.
     */
    for (i = 1; i < leaf_count; i++) {
	EB_Huffman_Node *least_nodep;
	EB_Huffman_Node *nodep;

	/*
	 * Initialize a new intermediate node.
	 */
	tail_nodep->type = EB_HUFFMAN_NODE_INTERMEDIATE;
	tail_nodep->left = NULL;
	tail_nodep->right = NULL;

	/*
	 * Find for a least frequent node.
	 * That node becomes a left child of the new intermediate node.
	 */
	least_nodep = NULL;
	for (nodep = zip->huffman_nodes; nodep < tail_nodep; nodep++) {
	    if (nodep->frequency == 0)
		continue;
	    if (least_nodep == NULL
		|| nodep->frequency <= least_nodep->frequency)
		least_nodep = nodep;
	}
	tail_nodep->left = (struct eb_huffman_node *) least_nodep;
	tail_nodep->frequency = least_nodep->frequency;
	least_nodep->frequency = 0;

	/*
	 * Find for a next least frequent node.
	 * That node becomes a right child of the new intermediate node.
	 */
	least_nodep = NULL;
	for (nodep = zip->huffman_nodes; nodep < tail_nodep; nodep++) {
	    if (nodep->frequency == 0)
		continue;
	    if (least_nodep == NULL
		|| nodep->frequency <= least_nodep->frequency)
		least_nodep = nodep;
	}
	tail_nodep->right = (struct eb_huffman_node *) least_nodep;
	tail_nodep->frequency += least_nodep->frequency;
	least_nodep->frequency = 0;

	tail_nodep++;
    }

    /*
     * Set a root node of the huffman tree.
     */ 
    zip->huffman_root = tail_nodep - 1;
    zip->code = EB_ZIP_EPWING;

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
 * Close `file'.  (`file' may have been compressed.)
 */
int
eb_zclose(zip, file)
    EB_Zip *zip;
    int file;
{
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
    /*
     * If `file' is normal file, simply call lseek().
     */
    if (zip->code == EB_ZIP_NONE)
	return lseek(file, offset, whence);

    /*
     * Calculate `new_offset' according with `whence'.
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


/*
 * Read data from `file'.  (`file' may have been compressed.)
 */
ssize_t
eb_zread(zip, file, buffer, nbytes)
    EB_Zip *zip;
    int file;
    char *buffer;
    size_t nbytes;
{
    /*
     * If `file' is not compressed, call read() and return.
     */
    if (zip->code == EB_ZIP_NONE)
	return eb_read_all(file, buffer, nbytes);
    else if (zip->code == EB_ZIP_EBZIP1)
	return eb_zread_ebzipped(zip, file, buffer, nbytes);
    else if (zip->code == EB_ZIP_EPWING)
	return eb_zread_epwzipped(zip, file, buffer, nbytes);

    return -1;
}


/*
 * Read data from `file' compressed with the ebzip compression format.
 */
static ssize_t
eb_zread_ebzipped(zip, file, buffer, nbytes)
    EB_Zip *zip;
    int file;
    char *buffer;
    size_t nbytes;
{
    char tmp_buffer[EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL];
    size_t read_nbytes = 0;
    size_t zipped_slice_size;
    off_t slice_location;
    off_t next_slice_location;
    int n;
    
    /*
     * Allocate memory for cache buffer.
     */
    if (cache_buffer == NULL) {
	cache_buffer = (char *) malloc(EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL);
	if (cache_buffer == NULL)
	    return -1;
    }

    /*
     * Read data.
     */
    while (read_nbytes < nbytes) {
	if (zip->file_size <= zip->offset)
	    return read_nbytes;

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
		return -1;
	    if (eb_read_all(file, tmp_buffer, zip->index_width * 2)
		!= zip->index_width * 2)
		return -1;

	    switch (zip->index_width) {
	    case 2:
		slice_location = eb_uint2(tmp_buffer);
		next_slice_location = eb_uint2(tmp_buffer + 2);
		break;
	    case 3:
		slice_location = eb_uint3(tmp_buffer);
		next_slice_location = eb_uint3(tmp_buffer + 3);
		break;
	    case 4:
		slice_location = eb_uint4(tmp_buffer);
		next_slice_location = eb_uint4(tmp_buffer + 4);
		break;
	    default:
		return -1;
	    }
	    zipped_slice_size = next_slice_location - slice_location;

	    if (next_slice_location <= slice_location
		|| zip->slice_size < zipped_slice_size)
		return -1;

	    /*
	     * Read a compressed slice from `file' and uncompress it.
	     * The data is not compressed if its size is equals to
	     * slice size.
	     */
	    if (lseek(file, slice_location, SEEK_SET) < 0)
		return -1;
	    if (eb_read_all(file, tmp_buffer, zipped_slice_size)
		!= zipped_slice_size)
		return -1;
	    if (zip->slice_size == zipped_slice_size)
		memcpy(cache_buffer, tmp_buffer, zip->slice_size);
	    else if (eb_ebunzip1_slice(cache_buffer, zip->slice_size,
		tmp_buffer, zipped_slice_size) < 0)
		return -1;

	    cache_file = file;
	}

	/*
	 * Copy data from `cache_buffer' to `buffer'.
	 */
	n = zip->slice_size - (zip->offset % zip->slice_size);
	if (nbytes - read_nbytes < n)
	    n = nbytes - read_nbytes;
	if (zip->file_size - zip->offset < n)
	    n = zip->file_size - zip->offset;
	memcpy(buffer + read_nbytes,
	    cache_buffer + (zip->offset - cache_offset), n);
	read_nbytes += n;
	zip->offset += n;
    }

    return read_nbytes;
}


/*
 * Read data from `file' compressed with the EPWING compression format.
 */
static ssize_t
eb_zread_epwzipped(zip, file, buffer, nbytes)
    EB_Zip *zip;
    int file;
    char *buffer;
    size_t nbytes;
{
    char tmp_buffer[36];
    size_t read_nbytes = 0;
    off_t page_location;
    int n;
    
    /*
     * Allocate memory for cache buffer.
     */
    if (cache_buffer == NULL) {
	cache_buffer = (char *) malloc(EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL);
	if (cache_buffer == NULL)
	    return -1;
    }

    /*
     * Read data.
     */
    while (read_nbytes < nbytes) {
	if (zip->file_size <= zip->offset)
	    return read_nbytes;

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
		return -1;
	    if (eb_read_all(file, tmp_buffer, 36) != 36)
		return -1;
	    page_location = eb_uint4(tmp_buffer) + eb_uint2(tmp_buffer + 4
		+ (zip->offset / EB_SIZE_PAGE % 16) * 2);

	    /*
	     * Read a compressed page from `file' and uncompress it.
	     * The data is not compressed if its size is equals to
	     * page size.
	     */
	    if (lseek(file, page_location, SEEK_SET) < 0)
		return -1;
	    if (eb_epwunzip_slice(cache_buffer, file, zip->huffman_root) < 0)
		return -1;

	    cache_file = file;
	}

	/*
	 * Copy data from `cache_buffer' to `buffer'.
	 */
	n = EB_SIZE_PAGE - (zip->offset % EB_SIZE_PAGE);
	if (nbytes - read_nbytes < n)
	    n = nbytes - read_nbytes;
	if (zip->file_size - zip->offset < n)
	    n = zip->file_size - zip->offset;
	memcpy(buffer + read_nbytes,
	    cache_buffer + (zip->offset - cache_offset), n);
	read_nbytes += n;
	zip->offset += n;
    }

    return read_nbytes;
}


/*
 * Read data from a file.
 * It repeats to call read() until all data will have been read.
 */
ssize_t
eb_read_all(file, buffer, nbytes)
    int file;
    VOID *buffer;
    size_t nbytes;
{
    char *bufp = buffer;
    ssize_t rest = nbytes;
    ssize_t n;

    while (0 < rest) {
	errno = 0;
	n = read(file, bufp, rest);
	if (n < 0) {
	    if (errno == EINTR)
		continue;
	    return n;
	} else if (n == 0)
	    return nbytes - rest;
	else {
	    rest -= n;
	    bufp += n;
	}
    }

    return nbytes;
}
