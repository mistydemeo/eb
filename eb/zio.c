/*                                                            -*- C -*-
 * Copyright (c) 1998  Motoyuki Kasahara
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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
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
static void eb_fix_zip_filename EB_P((char *));

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
 * Open `filename' for reading.  (`file' may have been compressed.)
 */
int
eb_zopen(zip, filename)
    EB_Zip *zip;
    char *filename;
{
    char header[EB_SIZE_ZIP_HEADER];
    int file = -1;

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
     * Cannot open a normal file.  Try ebzip'ped filename.
     */
    eb_fix_zip_filename(filename);
    file = open(filename, O_RDONLY | O_BINARY);
    if (file < 0)
	goto failed;

    /*
     * Read header part of the ebzip'ped file.
     */
    if (eb_read_all(file, header, EB_SIZE_ZIP_HEADER) != EB_SIZE_ZIP_HEADER)
	goto failed;
    zip->code = eb_uint1(header + 5) >> 4;
    zip->slice_size = EB_SIZE_PAGE << (eb_uint1(header + 5) & 0x0f);
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
	|| zip->code != EB_ZIP_MODE1
	|| EB_SIZE_PAGE << EB_MAX_ZIP_LEVEL < zip->slice_size)
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
    char tmp_buffer[EB_SIZE_PAGE << EB_MAX_ZIP_LEVEL];
    size_t read_nbytes = 0;
    size_t zip_size;
    off_t zip_location, next_location;
    int n;
    
    /*
     * If `file' is not compressed, call read() and return.
     */
    if (zip->code == EB_ZIP_NONE)
	return eb_read_all(file, buffer, nbytes);

    /*
     * Allocate memories for cache buffer.
     */
    if (cache_buffer == NULL) {
	cache_buffer = (char *) malloc(EB_SIZE_PAGE << EB_MAX_ZIP_LEVEL);
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
		+ EB_SIZE_ZIP_HEADER, SEEK_SET) < 0)
		return -1;
	    if (eb_read_all(file, tmp_buffer, zip->index_width * 2)
		!= zip->index_width * 2)
		return -1;

	    switch (zip->index_width) {
	    case 2:
		zip_location = eb_uint2(tmp_buffer);
		next_location = eb_uint2(tmp_buffer + 2);
		break;
	    case 3:
		zip_location = eb_uint3(tmp_buffer);
		next_location = eb_uint3(tmp_buffer + 3);
		break;
	    case 4:
		zip_location = eb_uint4(tmp_buffer);
		next_location = eb_uint4(tmp_buffer + 4);
		break;
	    default:
		return -1;
	    }
	    zip_size = next_location - zip_location;

	    if (next_location <= zip_location || zip->slice_size < zip_size)
		return -1;

	    /*
	     * Read a compressed slice from `file' and uncompress them.
	     * The data is not compressed if its size is equals to
	     * slice size.
	     */
	    if (lseek(file, zip_location, SEEK_SET) < 0)
		return -1;
	    if (eb_read_all(file, tmp_buffer, zip_size) != zip_size)
		return -1;
	    if (zip->slice_size == zip_size)
		memcpy(cache_buffer, tmp_buffer, zip->slice_size);
	    else if (eb_unzip_mode1(cache_buffer, zip->slice_size,
		tmp_buffer, zip_size) < 0)
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
 * Generate a zip filename from an original filename.
 * (e.g. generate `HONMON.EBZ;1' from `HONMON;1')
 */
static void
eb_fix_zip_filename(filename)
    char *filename;
{
    EB_Case_Code fcase;
    size_t length;
    char *suffix;
    const char *p;

    /*
     * Determine the letter case of the suffix of the zip filename.
     */
    for (p = filename, fcase =EB_CASE_UPPER; *p != '\0'; p++) {
	if ('A' <= *p && *p <= 'Z')
	    fcase = EB_CASE_UPPER;
	else if ('a' <= *p && *p <= 'z')
	    fcase = EB_CASE_LOWER;
    }
	
    /*
     * Add a suffix (`.EBZ' or `.ebz').
     *
     * Though the length of `filename' is increased, it never
     * overrun the buffer (PATH_MAX).  See eb_bind().
     */
    length = strlen(filename);
    if (*(filename + length - 3) == '.') {
	strcpy(filename + length - 2, "EBZ;1");
	suffix = filename + length - 2;
    } else if (*(filename + length - 2) == ';') {
	strcpy(filename + length - 2, ".EBZ;1");
	suffix = filename + length - 1;
    } else if (*(filename + length - 1) == '.') {
	strcpy(filename + length, "EBZ");
	suffix = filename + length;
    } else {
	strcpy(filename + length, ".EBZ");
	suffix = filename + length + 1;
    }
    if (fcase == EB_CASE_LOWER)
	memcpy(suffix, "ebz", 3);
}


/*
 * Read data from a file.
 * It repeats to call read() until all data will have been read.
 */
ssize_t
eb_read_all(file, buffer, nbyte)
    int file;
    VOID *buffer;
    size_t nbyte;
{
    char *bufp = buffer;
    ssize_t rest = nbyte;
    ssize_t n;

    while (0 < rest) {
	errno = 0;
	n = read(file, bufp, rest);
	if (n < 0) {
	    if (errno == EINTR)
		continue;
	    return n;
	} else if (n == 0)
	    return n;
	else {
	    rest -= n;
	    bufp += n;
	}
    }

    return nbyte;
}
