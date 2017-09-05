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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "binary.h"

/*
 * Initialize binary context of `book'.
 */
void
eb_initialize_binary(book)
    EB_Book *book;
{
    eb_lock(&book->lock);
    book->binary_context.zio = NULL;
    eb_unlock(&book->lock);
}


/*
 * Set monochrome bitmap picture as the current binary data.
 */
EB_Error_Code
eb_set_binary_bitmap(book, position, width, height)
    EB_Book *book;
    const EB_Position *position;
    int width;
    int height;
{
    EB_Error_Code error_code;
    EB_Binary_Context *context;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Current subbook must have a graphic file.
     */
    if (zio_file(&book->subbook_current->graphic_zio) < 0) {
	error_code = EB_ERR_NO_SUCH_BINARY;
	goto failed;
    }

    /*
     * Set binary context.
     */
    if (width <= 0 || height <= 0) {
	error_code = EB_ERR_NO_SUCH_BINARY;
	goto failed;
    }

    context = &book->binary_context;

    context->zio = &book->subbook_current->graphic_zio;
    context->location = (off_t)(position->page - 1) * EB_SIZE_PAGE
	+ position->offset;
    context->size = (width + 7) / 8 * height;
    context->offset = 0;
    context->cache_length = 0;
    context->cache_offset = 0;

    /*
     * Seek graphic file.
     */
    if (zio_lseek(context->zio, context->location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_BINARY;
	goto failed;
    }

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_binary(book);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Set WAVE sound as the current binary data.
 */
EB_Error_Code
eb_set_binary_wave(book, start_position, end_position)
    EB_Book *book;
    const EB_Position *start_position;
    const EB_Position *end_position;
{
    EB_Error_Code error_code;
    EB_Binary_Context *context;
    off_t start_location;
    off_t end_location;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Current subbook must have a sound file.
     */
    if (zio_file(&book->subbook_current->sound_zio) < 0) {
	error_code = EB_ERR_NO_SUCH_BINARY;
	goto failed;
    }

    /*
     * Set binary context.
     */
    start_location = (off_t)(start_position->page - 1) * EB_SIZE_PAGE
	+ start_position->offset;
    end_location   = (off_t)(end_position->page - 1)   * EB_SIZE_PAGE
	+ end_position->offset;

    context = &book->binary_context;

    context->zio = &book->subbook_current->sound_zio;
    context->location = start_location;
    if (start_location < end_location)
	context->size = end_location - start_location;
    else {
	error_code = EB_ERR_UNEXP_BINARY;
	goto failed;
    }
    context->offset = 0;

    /*
     * Read 4bytes from the sound file to check whether the sound
     * data contains a header part or not.
     */
    if (zio_lseek(context->zio, context->location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_BINARY;
	goto failed;
    }

    if (zio_read(context->zio, context->cache_buffer, 4) != 4) {
	error_code = EB_ERR_FAIL_READ_BINARY;
	goto failed;
    }

    /*
     * If the read data is "RIFF", the wave has a header part.
     * Otherwise, we must read a header in another location.
     *
     * The wave data consists of:
     * 
     *     "RIFF" wave-size(4bytes) "WAVE" header-fragment(28bytes) 
     *     data-part-size(4bytes) data
     *
     * wave-size      = "WAVE" + header-fragment + data-part-size + data
     *                = 4 + 28 + data + 4
     *                = data + 36
     * data-part-size = data
     */
    if (memcmp(context->cache_buffer, "RIFF", 4) == 0) {
	context->cache_length = 4;
    } else {
	/*
	 * Read and compose a WAVE header.
	 */
	memcpy(context->cache_buffer, "RIFF", 4);

	*(unsigned char *)(context->cache_buffer + 4)
	    = (context->size + 36)         & 0xff;
	*(unsigned char *)(context->cache_buffer + 5)
	    = ((context->size + 36) >> 8)  & 0xff;
	*(unsigned char *)(context->cache_buffer + 6)
	    = ((context->size + 36) >> 16) & 0xff;
	*(unsigned char *)(context->cache_buffer + 7)
	    = ((context->size + 36) >> 24) & 0xff;

	memcpy(context->cache_buffer + 8, "WAVE", 4);

	if (zio_lseek(context->zio, 
	    (book->subbook_current->sound.index_page - 1) * EB_SIZE_PAGE + 32,
	    SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_BINARY;
	    goto failed;
	}
	if (zio_read(context->zio, context->cache_buffer + 12, 28) != 28) {
	    error_code = EB_ERR_FAIL_SEEK_BINARY;
	    goto failed;
	}

	*(unsigned char *)(context->cache_buffer + 40)
	    = (context->size)       & 0xff;
	*(unsigned char *)(context->cache_buffer + 41)
	    = (context->size >> 8)  & 0xff;
	*(unsigned char *)(context->cache_buffer + 42)
	    = (context->size >> 16) & 0xff;
	*(unsigned char *)(context->cache_buffer + 43)
	    = (context->size >> 24) & 0xff;

	context->cache_length = 44;

	/*
	 * Seek sound file, again.
	 */
	if (zio_lseek(context->zio, context->location, SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_BINARY;
	    goto failed;
	}
    }

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_binary(book);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Length of the color graphic header.
 */
#define EB_COLOR_GRAPHIC_HEADER_LENGTH	8

/*
 * Set color graphic (BMP or JPEG) as the current binary data.
 */
EB_Error_Code
eb_set_binary_color_graphic(book, position)
    EB_Book *book;
    const EB_Position *position;
{
    EB_Error_Code error_code;
    EB_Binary_Context *context;
    char buffer[EB_COLOR_GRAPHIC_HEADER_LENGTH];

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Current subbook must have a graphic file.
     */
    if (zio_file(&book->subbook_current->graphic_zio) < 0) {
	error_code = EB_ERR_NO_SUCH_BINARY;
	goto failed;
    }

    /*
     * Set binary context.
     */
    context = &book->binary_context;

    context->zio = &book->subbook_current->graphic_zio;
    context->location = (off_t)(position->page - 1) * EB_SIZE_PAGE
	+ position->offset;
    context->offset = 0;
    context->cache_length = 0;
    context->cache_offset = 0;

    /*
     * Seek graphic file.
     */
    if (zio_lseek(context->zio, context->location, SEEK_SET) < 0) {
	error_code = EB_ERR_FAIL_SEEK_BINARY;
	goto failed;
    }

    /*
     * Read header of the graphic data.
     * Note that EB* JPEG file lacks the header.
     */
    if (zio_read(context->zio, buffer, EB_COLOR_GRAPHIC_HEADER_LENGTH) 
	!= EB_COLOR_GRAPHIC_HEADER_LENGTH) {
	error_code = EB_ERR_FAIL_READ_BINARY;
	goto failed;
    }

    if (memcmp(buffer, "data", 4) == 0) {
	context->size = eb_uint4_le(buffer + 4);
	context->location += EB_COLOR_GRAPHIC_HEADER_LENGTH;
    } else {
	context->size = 0;
	if (zio_lseek(context->zio, context->location, SEEK_SET) < 0) {
	    error_code = EB_ERR_FAIL_SEEK_BINARY;
	    goto failed;
	}
    }

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_binary(book);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Set MPEG movie as the current binary data.
 */
EB_Error_Code
eb_set_binary_mpeg(book, argv)
    EB_Book *book;
    const unsigned int *argv;
{
    /*
     * `movie_file_name' is base name, and `movie_path_name' is absolute
     * path of the movie.
     */
    char movie_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char movie_path_name[PATH_MAX + 1];
    EB_Error_Code error_code;
    EB_Subbook *subbook;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Initialize `movie_zio' again.
     */
    zio_finalize(&book->subbook_current->movie_zio);
    zio_initialize(&book->subbook_current->movie_zio);

    /*
     * Current subbook must have been set.
     */
    subbook = book->subbook_current;
    if (subbook == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Open the movie file and set binary context.
     */
    if (eb_compose_movie_file_name(argv, movie_file_name) < 0) {
	error_code = EB_ERR_NO_SUCH_BINARY;
	goto failed;
    }

    if (eb_compose_path_name3(book->path, subbook->directory_name, 
	subbook->movie_directory_name, movie_file_name, EB_SUFFIX_NONE,
	movie_path_name) != 0) {
	error_code = EB_ERR_NO_SUCH_BINARY;
	goto failed;
    }

    if (zio_open(&subbook->movie_zio, movie_path_name, ZIO_NONE) < 0) {
	subbook = NULL;
	error_code = EB_ERR_FAIL_OPEN_BINARY;
	goto failed;
    }

    book->binary_context.zio = &book->subbook_current->movie_zio;
    book->binary_context.location = 0;
    book->binary_context.size = 0;
    book->binary_context.offset = 0;
    book->binary_context.cache_length = 0;
    book->binary_context.cache_offset = 0;

    /*
     * Unlock the book.
     */
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_binary(book);
    eb_unlock(&book->lock);
    return error_code;
}


/*
 * Read binary data.
 */
EB_Error_Code
eb_read_binary(book, binary_max_length, binary, binary_length)
    EB_Book *book;
    size_t binary_max_length;
    char *binary;
    ssize_t *binary_length;

{
    EB_Error_Code error_code;
    EB_Binary_Context *context;
    size_t copy_length = 0;
    size_t read_length = 0;
    ssize_t read_result;

    /*
     * Lock the book.
     */
    eb_lock(&book->lock);

    /*
     * Current subbook must have been set.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Binary context must have been set.
     */
    context = &book->binary_context;

    if (context->zio == NULL) {
	error_code = EB_ERR_NO_CUR_BINARY;
	goto failed;
    }

    /*
     * Return immediately if `binary_max_length' is 0.
     */
    *binary_length = 0;

    if (binary_max_length <= 0)
	goto succeeded;

    /*
     * Copy cached data to `binary' if exists.
     */
    if (0 < context->cache_length) {
	if (binary_max_length < context->cache_length - context->cache_offset)
	    copy_length = binary_max_length;
	else
	    copy_length = context->cache_length - context->cache_offset;

	memcpy(binary, context->cache_buffer + context->cache_offset,
	    copy_length);
	*binary_length += copy_length;
	context->cache_offset += copy_length;

	if (context->cache_length <= context->cache_offset)
	    context->cache_length = 0;	

	if (binary_max_length <= copy_length)
	    goto succeeded;
    }

    /*
     * Read binary data if it is remained.
     * If context->size is 0, the binary data size is unknown.
     */
    if (0 < context->size && context->size <= context->offset)
	goto succeeded;

    if (context->size == 0)
	read_length = binary_max_length - copy_length;
    else if (binary_max_length - copy_length < context->size - context->offset)
	read_length = binary_max_length - copy_length;
    else
	read_length = context->size - context->offset;

    read_result = zio_read(context->zio, binary + copy_length, read_length);
    if (read_result != read_length) {
	error_code = EB_ERR_FAIL_READ_BINARY;
	goto failed;
    }

    *binary_length += read_result;
    context->offset += read_result;

    /*
     * Unlock the book.
     */
  succeeded:
    eb_unlock(&book->lock);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    eb_unset_binary(book);
    return error_code;
}


/*
 * Unset current binary.
 */
void
eb_unset_binary(book)
    EB_Book *book;
{
    eb_lock(&book->lock);
    book->binary_context.zio = NULL;
    eb_unlock(&book->lock);
}


