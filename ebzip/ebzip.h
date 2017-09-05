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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_UTIME_H
#include <utime.h>
#else
#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#endif
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else /* not HAVE_DIRENT_H */
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif /* HAVE_SYS_NDIR_H */
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif /* HAVE_SYS_DIR_H */
#if HAVE_NDIR_H
#include <ndir.h>
#endif /* HAVE_NDIR_H */
#endif /* not HAVE_DIRENT_H */

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>
#endif

#include <zlib.h>

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef HAVE_STRCASECMP
#if defined(__STDC__) || defined(WIN32)
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ or WIN32 */
int strcasecmp()
int strncasecmp();
#endif /* not __STDC__ or WIN32 */
#endif /* not HAVE_STRCASECMP or WIN32 */

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#if defined(__STDC__) || defined(WIN32)
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ or WIN32 */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ or WIN32 */
#endif

/*
 * O_BINARY flag for open().
 */
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

/*
 * stat macros.
 */
#ifdef	STAT_MACROS_BROKEN
#ifdef	S_ISREG
#undef	S_ISREG
#endif
#ifdef	S_ISDIR
#undef	S_ISDIR
#endif
#endif	/* STAT_MACROS_BROKEN */

#ifndef S_ISREG
#define S_ISREG(m)   (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)   (((m) & S_IFMT) == S_IFDIR)
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

#include "eb.h"
#include "error.h"
#include "font.h"
#include "build-post.h"

#include "getopt.h"
#include "getumask.h"
#include "makedir.h"
#include "samefile.h"
#include "yesno.h"

/*
 * Trick for function protypes.
 */
#ifndef EB_P
#if defined(__STDC__) || defined(WIN32)
#define EB_P(p) p
#else /* not __STDC__ or WIN32 */
#define EB_P(p) ()
#endif /* not __STDC__ or WIN32 */
#endif /* EB_P */

/*
 * Tricks for gettext.
 */
#ifdef ENABLE_NLS
#define _(string) gettext(string)
#ifdef gettext_noop
#define N_(string) gettext_noop(string)
#else
#define N_(string) (string)
#endif
#else
#define _(string) (string)       
#define N_(string) (string)
#endif

/*
 * Character type tests and conversions.
 */
#define ASCII_ISDIGIT(c) ('0' <= (c) && (c) <= '9')
#define ASCII_ISUPPER(c) ('A' <= (c) && (c) <= 'Z')
#define ASCII_ISLOWER(c) ('a' <= (c) && (c) <= 'z')
#define ASCII_ISALPHA(c) \
 (ASCII_ISUPPER(c) || ASCII_ISLOWER(c))
#define ASCII_ISALNUM(c) \
 (ASCII_ISUPPER(c) || ASCII_ISLOWER(c) || ASCII_ISDIGIT(c))
#define ASCII_ISXDIGIT(c) \
 (ASCII_ISDIGIT(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define ASCII_TOUPPER(c) (('a' <= (c) && (c) <= 'z') ? (c) - 0x20 : (c))
#define ASCII_TOLOWER(c) (('A' <= (c) && (c) <= 'Z') ? (c) + 0x20 : (c))

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

/* sebxa.c */
int fix_sebxa_start EB_P((const char *, int));
int get_sebxa_indexes EB_P((const char *, int, off_t *, off_t *, off_t *,
    off_t *));

/* unzipbook.c */
int ebzip_unzip_book EB_P((const char *, const char *,
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));

/* unzipfile.c */
int ebzip_unzip_file EB_P((const char *, const char *, Zio_Code));
int ebzip_unzip_start_file EB_P((const char *, const char *, Zio_Code, int));

/* zipbook.c */
int ebzip_zip_book EB_P((const char *, const char *,
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));

/* zipfile.c */
int ebzip_zip_file EB_P((const char *, const char *, Zio_Code));
int ebzip_zip_start_file EB_P((const char *, const char *, Zio_Code, int));

/* zipinfobook.c */
int ebzip_zipinfo_book EB_P((const char *,
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));

/* zipinfofile.c */
int ebzip_zipinfo_file EB_P((const char *, Zio_Code));
int ebzip_zipinfo_start_file EB_P((const char *, Zio_Code, int));

#endif /* EBZIP_H */
