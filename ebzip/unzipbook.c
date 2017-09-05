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

#include "eb.h"
#include "error.h"
#include "font.h"
#include "build-post.h"

#include "ebzip.h"
#include "ebutils.h"

#include "getumask.h"
#include "makedir.h"

/*
 * Unexported function.
 */
static int ebzip_unzip_book_eb EB_P((EB_Book *, const char *, const char *,
    EB_Subbook_Code *, int));
static int ebzip_unzip_book_epwing EB_P((EB_Book *, const char *, const char *,
    EB_Subbook_Code *, int));


/*
 * Uncompress files in `book' and output them under `out_top_path'.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_unzip_book(out_top_path, book_path, subbook_name_list,
    subbook_name_count)
    const char *out_top_path;
    const char *book_path;
    char subbook_name_list[][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_name_count;
{
    EB_Book book;
    EB_Error_Code error_code;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    EB_Subbook_Code subbook_code;
    int subbook_count = 0;
    int result;
    int i;

    eb_initialize_book(&book);

    /*
     * Bind a book.
     */
    error_code = eb_bind(&book, book_path);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: %s\n", invoked_name,
	    eb_error_message(error_code));
	fflush(stderr);
	return -1;
    }

    /*
     * For each targe subbook, convert a subbook-names to a subbook-codes.
     * If no subbook is specified by `--subbook'(`-S'), set all subbooks
     * as the target.
     */
    if (subbook_name_count == 0) {
	error_code = eb_subbook_list(&book, subbook_list, &subbook_count);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s\n", invoked_name,
		eb_error_message(error_code));
	    fflush(stderr);
	    return -1;
	}
    } else {
	for (i = 0; i < subbook_name_count; i++) {
	    error_code = find_subbook(&book, subbook_name_list[i],
		&subbook_code);
	    if (error_code != EB_SUCCESS) {
		fprintf(stderr, _("%s: unknown subbook name `%s'\n"),
		    invoked_name, subbook_name_list[i]);
		return -1;
	    }
	    subbook_list[subbook_count++] = subbook_code;
	}
    }

    /*
     * Uncompress the book.
     */
    if (book.disc_code == EB_DISC_EB) {
	result = ebzip_unzip_book_eb(&book, out_top_path, book_path,
	    subbook_list, subbook_count);
    } else {
	result = ebzip_unzip_book_epwing(&book, out_top_path, book_path,
	    subbook_list, subbook_count);
    }

    eb_finalize_book(&book);

    return result;
}


/*
 * Internal function for `unzip_book'.
 * This is used to compress an EB book.
 */
static int
ebzip_unzip_book_eb(book, out_top_path, book_path, subbook_list,
    subbook_count)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
    EB_Subbook_Code *subbook_list;
    int subbook_count;
{
    EB_Subbook *subbook;
    char in_path_name[PATH_MAX + 1];
    char out_sub_path[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];
    char catalog_file_name[EB_MAX_FILE_NAME_LENGTH];
    char language_file_name[EB_MAX_FILE_NAME_LENGTH];
    mode_t out_directory_mode;
    Zio_Code in_zio_code;
    int i;

    /*
     * If `out_top_path' and/or `book_path' represents "/", replace it
     * to an empty string.
     */
    if (strcmp(out_top_path, "/") == 0)
	out_top_path++;
    if (strcmp(book_path, "/") == 0)
	book_path++;

    /*
     * Initialize variables.
     */
    out_directory_mode = 0777 ^ get_umask();
    eb_load_all_subbooks(book);

    /*
     * Uncompress a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Make an output directory for the current subbook.
	 */
	eb_compose_path_name(out_top_path, subbook->directory_name,
	    out_sub_path);
	if (!ebzip_test_flag
	    && make_missing_directory(out_sub_path, out_directory_mode) < 0)
	    return -1;

	/*
	 * Uncompress START file.
	 */
	in_zio_code = zio_mode(&subbook->text_zio);

	if (in_zio_code != ZIO_INVALID) {
	    eb_compose_path_name2(book->path, subbook->directory_name,
		subbook->text_file_name, in_path_name);
	    eb_compose_path_name2(out_top_path, subbook->directory_name,
		subbook->text_file_name, out_path_name);
	    eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_NONE);
	    ebzip_unzip_file(out_path_name, in_path_name, in_zio_code);
	}
    }

    /*
     * Uncompress a language file.
     */
    if (eb_find_file_name(book->path, "language", language_file_name)
	== EB_SUCCESS) {
	eb_compose_path_name(book->path, language_file_name, in_path_name);
	eb_compose_path_name(out_top_path, language_file_name, out_path_name);
	eb_path_name_zio_code(in_path_name, ZIO_PLAIN, &in_zio_code);
	eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_NONE);
	ebzip_unzip_file(out_path_name, in_path_name, in_zio_code);
    }

    /*
     * Copy CATALOG file.
     */
    if (eb_find_file_name(book->path, "catalog", catalog_file_name)
	== EB_SUCCESS) {
	eb_compose_path_name(book->path, catalog_file_name, in_path_name);
	eb_compose_path_name(out_top_path, catalog_file_name, out_path_name);
	ebzip_copy_file(out_path_name, in_path_name);
    }

    return 0;
}


/*
 * Internal function for `unzip_book'.
 * This is used to compress an EPWING book.
 */
static int
ebzip_unzip_book_epwing(book, out_top_path, book_path, subbook_list,
    subbook_count)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
    EB_Subbook_Code *subbook_list;
    int subbook_count;
{
    EB_Subbook *subbook;
    EB_Font *font;
    char in_path_name[PATH_MAX + 1];
    char out_sub_path[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];
    char catalogs_file_name[EB_MAX_FILE_NAME_LENGTH];
    mode_t out_directory_mode;
    Zio_Code in_zio_code;
    int i, j;

    /*
     * If `out_top_path' and/or `book_path' represents "/", replace it
     * to an empty string.
     */
    if (strcmp(out_top_path, "/") == 0)
	out_top_path++;
    if (strcmp(book_path, "/") == 0)
	book_path++;

    /*
     * Initialize variables.
     */
    out_directory_mode = 0777 ^ get_umask();
    eb_load_all_subbooks(book);

    /*
     * Uncompress a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Make an output directory for the current subbook.
	 */
	eb_compose_path_name(out_top_path, subbook->directory_name,
	    out_sub_path);
	if (!ebzip_test_flag
	    && make_missing_directory(out_sub_path, out_directory_mode) < 0)
	    return -1;

	/*
	 * Make `data' sub directory for the current subbook.
	 */
	eb_compose_path_name2(out_top_path, subbook->directory_name,
	    subbook->data_directory_name, out_sub_path);
	if (!ebzip_test_flag
	    && make_missing_directory(out_sub_path, out_directory_mode) < 0)
	    return -1;

	/*
	 * Uncompress HONMON/HONMON2 file.
	 */
	in_zio_code = zio_mode(&subbook->text_zio);

	if (in_zio_code != ZIO_INVALID) {
	    eb_compose_path_name3(book->path, subbook->directory_name,
		subbook->data_directory_name, subbook->text_file_name,
		in_path_name);
	    eb_compose_path_name3(out_top_path, subbook->directory_name,
		subbook->data_directory_name, subbook->text_file_name,
		out_path_name);
	    if (strncasecmp(subbook->text_file_name, "honmon2", 7) == 0)
		eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_ORG);
	    else
		eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_NONE);
	    ebzip_unzip_file(out_path_name, in_path_name, in_zio_code);
	}

	/*
	 * Uncompress HONMONS file.
	 */
	in_zio_code = zio_mode(&subbook->sound_zio);

	if (!ebzip_skip_flag_sound
	    && in_zio_code != ZIO_INVALID
	    && strncasecmp(subbook->sound_file_name, "honmons", 7) == 0) {
	    eb_compose_path_name3(book->path, subbook->directory_name,
		subbook->data_directory_name, subbook->sound_file_name,
		in_path_name);
	    eb_compose_path_name3(out_top_path, subbook->directory_name,
		subbook->data_directory_name, subbook->sound_file_name,
		out_path_name);
	    eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_NONE);
	    ebzip_unzip_file(out_path_name, in_path_name, in_zio_code);
	}

	/*
	 * Uncompress HONMONG file.
	 */
	in_zio_code = zio_mode(&subbook->graphic_zio);

	if (!ebzip_skip_flag_graphic
	    && in_zio_code != ZIO_INVALID
	    && strncasecmp(subbook->graphic_file_name, "honmong", 7) == 0) {
	    eb_compose_path_name3(book->path, subbook->directory_name,
		subbook->data_directory_name, subbook->graphic_file_name,
		in_path_name);
	    eb_compose_path_name3(out_top_path, subbook->directory_name,
		subbook->data_directory_name, subbook->graphic_file_name,
		out_path_name);
	    eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_NONE);
	    ebzip_unzip_file(out_path_name, in_path_name, in_zio_code);
	}

	/*
	 * Make `gaiji' sub directory for the current subbook.
	 */
	if (!ebzip_skip_flag_font) {
	    eb_compose_path_name2(out_top_path, subbook->directory_name,
		subbook->gaiji_directory_name, out_sub_path);
	    if (!ebzip_test_flag
		&& make_missing_directory(out_sub_path, out_directory_mode)
		< 0) {
		return -1;
	    }

	    /*
	     * Uncompress narrow font files.
	     */
	    for (j = 0; j < EB_MAX_FONTS; j++) {
		font = subbook->narrow_fonts + j;
		if (font->font_code == EB_FONT_INVALID)
		    continue;

		in_zio_code = zio_mode(&font->zio);

		if (in_zio_code != ZIO_INVALID) {
		    eb_compose_path_name3(book->path,
			subbook->directory_name, subbook->gaiji_directory_name,
			font->file_name, in_path_name);
		    eb_compose_path_name3(out_top_path,
			subbook->directory_name, subbook->gaiji_directory_name,
			font->file_name, out_path_name);
		    eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_NONE);
		    ebzip_unzip_file(out_path_name, in_path_name, in_zio_code);
		}
	    }

	    /*
	     * Uncompress wide font files.
	     */
	    for (j = 0; j < EB_MAX_FONTS; j++) {
		font = subbook->wide_fonts + j;
		if (font->font_code == EB_FONT_INVALID)
		    continue;

		in_zio_code = zio_mode(&font->zio);

		if (in_zio_code != ZIO_INVALID) {
		    eb_compose_path_name3(book->path,
			subbook->directory_name, subbook->gaiji_directory_name,
			font->file_name, in_path_name);
		    eb_compose_path_name3(out_top_path,
			subbook->directory_name, subbook->gaiji_directory_name,
			font->file_name, out_path_name);
		    eb_fix_path_name_suffix(out_path_name, EBZIP_SUFFIX_NONE);
		    ebzip_unzip_file(out_path_name, in_path_name, in_zio_code);
		}
	    }
	}

	/*
	 * Copy movie files.
	 */
	if (!ebzip_skip_flag_movie) {
	    eb_compose_path_name2(book->path, subbook->directory_name,
		subbook->movie_directory_name, in_path_name);
	    eb_compose_path_name2(out_top_path, subbook->directory_name,
		subbook->movie_directory_name, out_path_name);
	    ebzip_copy_files_in_directory(out_path_name, in_path_name);
	}
    }

    /*
     * Copy CATALOGS file.
     */
    if (eb_find_file_name(book->path, "catalogs", catalogs_file_name)
	== EB_SUCCESS) {
	eb_compose_path_name(book->path, catalogs_file_name, in_path_name);
	eb_compose_path_name(out_top_path, catalogs_file_name, out_path_name);
	ebzip_copy_file(out_path_name, in_path_name);
    }

    return 0;
}
