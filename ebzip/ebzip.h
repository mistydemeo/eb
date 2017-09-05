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

#ifndef EBZIP_H
#define EBZIP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * File name suffixes.
 */
#define EBZIP_SUFFIX_NONE		""
#define EBZIP_SUFFIX_EBZ		".ebz"
#define EBZIP_SUFFIX_ORG		".org"

/*
 * Defaults input and output directories.
 */
#define EBZIP_DEFAULT_BOOK_DIRECTORY	"."
#define EBZIP_DEFAULT_OUTPUT_DIRECTORY	"."

/*
 * Information output interval.
 */
#define EBZIP_PROGRESS_INTERVAL_FACTOR	1024

/*
 * Overwrite modes.
 */
#define EBZIP_OVERWRITE_QUERY		0
#define EBZIP_OVERWRITE_FORCE		1
#define EBZIP_OVERWRITE_NO		2

/*
 * Defaults.
 */
#define EBZIP_DEFAULT_LEVEL		0
#define EBZIP_DEFAULT_KEEP		0
#define EBZIP_DEFAULT_QUIET		0
#define EBZIP_DEFAULT_TEST		0
#define EBZIP_DEFAULT_OVERWRITE		EBZIP_OVERWRITE_QUERY

#define EBZIP_DEFAULT_SKIP_FONT		0
#define EBZIP_DEFAULT_SKIP_GRAPHIC	0
#define EBZIP_DEFAULT_SKIP_MOVIE	0
#define EBZIP_DEFAULT_SKIP_SOUND	0

/*
 * Global variables.
 */
extern const char *program_name;
extern const char *program_version;
extern const char *invoked_name;

extern int ebzip_level;
extern int ebzip_keep_flag;
extern int ebzip_quiet_flag;
extern int ebzip_test_flag;
extern int ebzip_overwrite_mode;

extern int ebzip_skip_flag_font;
extern int ebzip_skip_flag_graphic;
extern int ebzip_skip_flag_movie;
extern int ebzip_skip_flag_sound;

/*
 * Function declarations.
 */
/* copyfile.c */
int ebzip_copy_file EB_P((const char *, const char *));
int ebzip_copy_files_in_directory EB_P((const char *, const char *));

/* ebzip1.c */
int ebzip1_slice EB_P((char *, size_t *, char *, size_t));

/* filename.c */
void fix_path_name_suffix EB_P((char *, const char *));

/* unzipbook.c */
int ebzip_unzip_book EB_P((const char *, const char *,
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));
/* unzipfile.c */
int ebzip_unzip_file EB_P((const char *, const char *, Zio_Code));

/* zipbook.c */
int ebzip_zip_book EB_P((const char *, const char *,
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));
/* zipfile.c */
int ebzip_zip_file EB_P((const char *, const char *, Zio_Code));

/* zipinfobook.c */
int ebzip_zipinfo_book EB_P((const char *,
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));
/* zipinfofile.c */
int ebzip_zipinfo_file EB_P((const char *, Zio_Code));

#endif /* EBZIP_H */
