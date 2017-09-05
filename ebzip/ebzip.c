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
#endif

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>
#endif

#include <zlib.h>

#include "ebutils.h"

#ifndef HAVE_STRUCT_UTIMBUF
struct utimbuf {
    long actime;
    long modtime;
};
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

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp()
int strncasecmp();
#endif /* not __STDC__ */
#endif /* not HAVE_STRCASECMP */

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
#ifdef  STAT_MACROS_BROKEN
#ifdef  S_ISREG
#undef  S_ISREG
#endif
#ifdef  S_ISDIR
#undef  S_ISDIR
#endif
#endif  /* STAT_MACROS_BROKEN */

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
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

#include "eb/eb.h"
#include "eb/error.h"
#include "eb/internal.h"
#include "eb/font.h"

#include "fakelog.h"
#include "getopt.h"
#include "getumask.h"
#include "makedir.h"
#include "samefile.h"
#include "yesno.h"

/*
 * Trick for function protypes.
 */
#ifndef EB_P
#ifdef __STDC__
#define EB_P(p) p
#else /* not __STDC__ */
#define EB_P(p) ()
#endif /* not __STDC__ */
#endif /* EB_P */

/*
 * Tricks for gettext.
 */
#define _(string) gettext(string)
#ifdef gettext_noop
#define N_(string) gettext_noop(string)
#else
#define N_(string) (string)
#endif

/*
 * Defaults and limitations.
 */
#define DEFAULT_EBZIP_LEVEL		0
#define DEFAULT_BOOK_DIRECTORY          "."
#define DEFAULT_OUTPUT_DIRECTORY        "."

/*
 * Case of file names (upper or lower).
 */
#define CASE_UNCHANGE			-1
#define CASE_UPPER			0
#define CASE_LOWER			1

/*
 * Suffix to be added to file names (none, `.', or `.;1').
 */
#define SUFFIX_UNCHANGE			-1
#define SUFFIX_NONE			0
#define SUFFIX_DOT			1
#define SUFFIX_VERSION			2
#define SUFFIX_BOTH			3

/*
 * Information output interval.
 */
#define INFORMATION_INTERVAL_FACTOR		1024

/*
 * Maximum length of a file name list.
 */
#define MAX_LENGTH_FILE_NAME_LIST		3

/*
 * Character type tests and conversions.
 */
#define isdigit(c) ('0' <= (c) && (c) <= '9')
#define isupper(c) ('A' <= (c) && (c) <= 'Z')
#define islower(c) ('a' <= (c) && (c) <= 'z')
#define isalpha(c) (isupper(c) || islower(c))
#define isalnum(c) (isupper(c) || islower(c) || isdigit(c))
#define isxdigit(c) \
 (isdigit(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define toupper(c) (('a' <= (c) && (c) <= 'z') ? (c) - 0x20 : (c))
#define tolower(c) (('A' <= (c) && (c) <= 'Z') ? (c) + 0x20 : (c))

/*
 * Command line options.
 */
static const char *short_options = "fhikl:no:qS:tT:uvz";
static struct option long_options[] = {
    {"force-overwrite",   no_argument,       NULL, 'f'},
    {"help",              no_argument,       NULL, 'h'},
    {"information",       no_argument,       NULL, 'i'},
    {"keep",              no_argument,       NULL, 'k'},
    {"level",             required_argument, NULL, 'l'},
    {"no-overwrite",      no_argument,       NULL, 'n'},
    {"output-directory",  required_argument, NULL, 'o'},
    {"quiet",             no_argument,       NULL, 'q'},
    {"silent",            no_argument,       NULL, 'q'},
    {"subbook",           required_argument, NULL, 'S'},
    {"test",              no_argument,       NULL, 't'},
    {"uncompress",        no_argument,       NULL, 'u'},
    {"version",           no_argument,       NULL, 'v'},
    {"compress",          no_argument,       NULL, 'z'},
    {NULL, 0, NULL, 0}
};

/*
 * CD-ROM book.
 */
EB_Book book;

/*
 * Operation modes.
 */
#define EBZIP_ACTION_ZIP		0
#define EBZIP_ACTION_UNZIP		1
#define EBZIP_ACTION_INFO		2

int action_mode = EBZIP_ACTION_ZIP;

/*
 * Overwrite modes.
 */
#define EBZIP_OVERWRITE_QUERY		0
#define EBZIP_OVERWRITE_FORCE		1
#define EBZIP_OVERWRITE_NO		2

int overwrite_mode;

/*
 * A list of subbook names to be compressed/uncompressed.
 */
char subbook_name_list[EB_MAX_SUBBOOKS][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
int subbook_name_count = 0;

/*
 * A list of subbook codes to be compressed/uncompressed.
 */
EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
int subbook_count = 0;

/*
 * Zip level.
 */
int zip_level = DEFAULT_EBZIP_LEVEL;

/*
 * File name case and suffix.
 */
EB_Case_Code file_name_case = CASE_UNCHANGE;
EB_Suffix_Code file_name_suffix = SUFFIX_UNCHANGE;

/*
 * Flags.
 */
static int keep_flag = 0;
static int quiet_flag = 0;
static int test_flag = 0;

/*
 * File name to be deleted and file to be closed when signal is received.
 */
static const char *trap_file_name = NULL;
static int trap_file = -1;

/*
 * Unexported functions.
 */
static int parse_zip_level EB_P((const char *));
static void output_help EB_P((void));
static int zip_book EB_P((EB_Book *, const char *, const char *));
static int zip_eb_book EB_P((EB_Book *, const char *, const char *));
static int zip_epwing_book EB_P((EB_Book *, const char *, const char *));
static int unzip_book EB_P((EB_Book *, const char *, const char *));
static int unzip_eb_book EB_P((EB_Book *, const char *, const char *));
static int unzip_epwing_book EB_P((EB_Book *, const char *, const char *));
static int zipinfo_book EB_P((EB_Book *, const char *));
static int zipinfo_eb_book EB_P((EB_Book *, const char *));
static int zipinfo_epwing_book EB_P((EB_Book *, const char *));
static int zip_file EB_P((const char *, const char *,
    int (*) EB_P((EB_Zip *, const char *)) ));
static int unzip_file EB_P((const char *, const char *,
    int (*) EB_P((EB_Zip *, const char *)) ));
static int zipinfo_file EB_P((const char *,
    int (*) EB_P((EB_Zip *, const char *)) ));
static int copy_file EB_P((const char *, const char *));
static RETSIGTYPE trap EB_P((int));

/* filename.c */
void compose_out_path_name EB_P((const char *, const char *, const char *,
    char *));
void compose_out_path_name2 EB_P((const char *, const char *, const char *,
    const char *, char *));
void compose_out_path_name3 EB_P((const char *, const char *, const char *,
    const char *, const char *, char *));

/*
 * External functions.
 */
extern int ebzip1_slice EB_P((char *, size_t *, char *, size_t));

int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Error_Code error_code;
    EB_Subbook_Code subbook_code;
    char out_top_path[PATH_MAX + 1];
    char book_path[PATH_MAX + 1];
    int ch;
    int i;
    char *invoked_base_name;

    program_name = "ebzip";
    invoked_name = argv[0];
    strcpy(out_top_path, DEFAULT_OUTPUT_DIRECTORY);

    /*
     * Initialize locale data.
     */
#ifdef ENABLE_NLS
#ifdef HAVE_SETLOCALE
       setlocale(LC_ALL, "");
#endif
       bindtextdomain(TEXT_DOMAIN_NAME, LOCALEDIR);
       textdomain(TEXT_DOMAIN_NAME);
#endif

    /*
     * Set default action.
     */
    invoked_base_name = strrchr(argv[0], '/');
    if (invoked_base_name == NULL)
	invoked_base_name = argv[0];
    else
	invoked_base_name++;

    /*
     * Determine the default action.
     */
#ifndef EXEEXT_EXE
    if (strcmp(invoked_base_name, "ebunzip") == 0)
	action_mode = EBZIP_ACTION_UNZIP;
    else if (strcmp(invoked_base_name, "ebzipinfo") == 0)
	action_mode = EBZIP_ACTION_INFO;
#else /* EXEEXT_EXE */
    if (strcasecmp(invoked_base_name, "ebunzip") == 0
	|| strcasecmp(invoked_base_name, "ebunzip.exe") == 0) {
	action_mode = EBZIP_ACTION_UNZIP;
    } else if (strcasecmp(invoked_base_name, "ebzipinfo") == 0
	|| strcasecmp(invoked_base_name, "ebzipinf.exe") == 0) {
	action_mode = EBZIP_ACTION_INFO;
    }
#endif /* EXEEXT_EXE */

    /*
     * Set overwrite mode.
     */
    if (isatty(0))
	overwrite_mode = EBZIP_OVERWRITE_QUERY;
    else
	overwrite_mode = EBZIP_OVERWRITE_NO;

    /*
     * Initialize `book'.
     */
    eb_initialize_library();
    eb_initialize_book(&book);

    /*
     * Set fakelog behavior.
     */
    set_fakelog_name(invoked_name);
    set_fakelog_mode(FAKELOG_TO_STDERR);
    set_fakelog_level(FAKELOG_ERR);

    /*
     * Parse command line options.
     */
    for (;;) {
        ch = getopt_long(argc, argv, short_options, long_options, NULL);
        if (ch == EOF)
            break;
        switch (ch) {
        case 'f':
            /*
             * Option `-f'.  Set `force' to the overwrite flag.
             */
	    overwrite_mode = EBZIP_OVERWRITE_FORCE;
	    break;

        case 'h':
            /*
             * Option `-h'.  Display help message, then exit.
             */
            output_help();
            exit(0);

        case 'i':
	    /*
	     * Option `-i'.  Information mode.
	     */
	    action_mode = EBZIP_ACTION_INFO;
	    break;

        case 'k':
            /*
             * Option `-k'.  Keep (don't delete) input files.
             */
	    keep_flag = 1;
	    break;

        case 'l':
            /*
             * Option `-l'.  Specify compression level.
             */
	    if (parse_zip_level(optarg) < 0)
		exit(1);
	    break;

        case 'n':
            /*
             * Option `-n'.  Set `no' to the overwrite flag.
             */
	    overwrite_mode = EBZIP_OVERWRITE_NO;
	    break;

        case 'o':
            /*
             * Option `-o'.  Output files under DIRECOTRY.
	     * The length of the file name
	     *    "<out_top_path>/subdir/subsubdir/file.ebz;1"
	     * must not exceed PATH_MAX.
             */
            if (PATH_MAX < strlen(optarg)) {
                fprintf(stderr, _("%s: too long output directory path\n"),
                    invoked_name);
                exit(1);
            }
            strcpy(out_top_path, optarg);
	    canonicalize_path(out_top_path);
	    if (PATH_MAX < strlen(out_top_path) + 1
		+ EB_MAX_DIRECTORY_NAME_LENGTH + 1
		+ EB_MAX_DIRECTORY_NAME_LENGTH + 1
		+ EB_MAX_FILE_NAME_LENGTH) {
		fprintf(stderr, _("%s: too long output directory path\n"),
		    invoked_name);
		goto die;
	    }
	    break;

        case 'q':
            /*
             * Option `-q'.  Set quiet flag.
             */
	    quiet_flag = 1;
	    break;

        case 'S':
            /*
             * Option `-S'.  Specify target subbooks.
             */
            if (parse_subbook_name_argument(optarg, subbook_name_list,
		&subbook_name_count) < 0)
                exit(1);
            break;

        case 't':
            /*
             * Option `-t'.  Set test mode.
             */
	    test_flag = 1;
	    break;

        case 'u':
            /*
             * Option `-u'.  Decompression mode.
             */
	    action_mode = EBZIP_ACTION_UNZIP;
	    break;

        case 'v':
            /*
             * Option `-v'.  Display version number, then exit.
             */
            output_version();
            exit(0);

        case 'z':
            /*
             * Option `-z'.  Compression mode.
             */
	    action_mode = EBZIP_ACTION_ZIP;
	    break;

        default:
            output_try_help();
	    goto die;
	}
    }

    /*
     * Check the number of rest arguments.
     */
    if (1 < argc - optind) {
        fprintf(stderr, _("%s: too many arguments\n"), invoked_name);
        output_try_help();
	goto die;
    }

    /*
     * Set a book path.
     */
    if (argc == optind)
        strcpy(book_path, DEFAULT_BOOK_DIRECTORY);
    else
        strcpy(book_path, argv[optind]);
    canonicalize_path(book_path);

    /*
     * Bind a book.
     */
    error_code = eb_bind(&book, book_path);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: %s\n", invoked_name,
	    eb_error_message(error_code));
	fflush(stderr);
	goto die;
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
	    goto die;
	}
    } else {
	for (i = 0; i < subbook_name_count; i++) {
	    subbook_code = find_subbook(&book, subbook_name_list[i]);
	    if (subbook_code != EB_SUCCESS)
		goto die;
	    subbook_list[subbook_count++] = subbook_code;
	}
    }

    /*
     * Compress the book.
     */
    switch (action_mode) {
    case EBZIP_ACTION_ZIP:
	if (zip_book(&book, out_top_path, book_path) < 0)
	    goto die;
	break;
    case EBZIP_ACTION_UNZIP:
	if (unzip_book(&book, out_top_path, book_path) < 0)
	    goto die;
	exit(1);
	break;
    case EBZIP_ACTION_INFO:
	if (zipinfo_book(&book, book_path) < 0)
	    goto die;
	break;
    }

    eb_finalize_book(&book);
    eb_finalize_library();

    return 0;

    /*
     * A critical error occurs...
     */
  die:
    eb_finalize_book(&book);
    eb_finalize_library();
    exit(1);
}


/*
 * Parse an argument to option `--level (-l)'.
 * If the argument is valid form, 0 is returned.
 * Otherwise -1 is returned.
 */
static int
parse_zip_level(argument)
    const char *argument;
{
    char *end_p;

    zip_level = (int)strtol(argument, &end_p, 10);
    if (!isdigit(*argument) || *end_p != '\0'
	|| zip_level < 0 || EB_MAX_EBZIP_LEVEL < zip_level) {
	fprintf(stderr, _("%s: invalid compression level `%s'\n"),
	    invoked_name, argument);
	fflush(stderr);
	return -1;
    }

    return 0;
}


/*
 * Output help message to stdandard out.
 */
static void
output_help()
{
    printf(_("Usage: %s [option...] [book-directory]\n"), program_name);
    printf(_("Options:\n"));
    printf(_("  -f  --force-overwrite      force overwrite of output files\n"));
    printf(_("  -h  --help                 display this help, then exit\n"));
    printf(_("  -i  --information          list information of compressed files\n"));
    printf(_("  -k  --keep                 keep (don't delete) original files\n"));
    printf(_("  -l INTEGER  --level INTEGER\n"));
    printf(_("                             compression level; 0..%d\n"),
	EB_MAX_EBZIP_LEVEL);
    printf(_("                             (default: %d)\n"),
	DEFAULT_EBZIP_LEVEL);
    printf(_("  -n  --no-overwrite         don't overwrite output files\n"));
    printf(_("  -o DIRECTORY  --output-directory DIRECTORY\n"));
    printf(_("                             ouput files under DIRECTORY\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_OUTPUT_DIRECTORY);
    printf(_("  -q  --quiet  --silence     suppress all warnings\n"));
    printf(_("  -S SUBBOOK[,SUBBOOK...]  --subbook SUBBOOK[,SUBBOOK...]\n"));
    printf(_("                             target subbook\n"));
    printf(_("                             (default: all subbooks)\n"));
    printf(_("  -t  --test                 only check for input files\n"));
    printf(_("  -u  --uncompress           uncompress files\n"));
    printf(_("  -v  --version              display version number, then exit\n"));
    printf(_("  -z  --compress             compress files\n"));
    printf(_("\nArgument:\n"));
    printf(_("  book-directory             top directory of a CD-ROM book\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_BOOK_DIRECTORY);

    printf(_("\nDefault action:\n"));
#ifndef EXEEXT
    printf(_("  When invoked as `ebunzip', uncompression is the default action.\n"));
    printf(_("  When invoked as `ebzipinfo', listing information is the default action.\n"));
#else
    printf(_("  When invoked as `ebunzip.exe', uncompression is the default action.\n"));
    printf(_("  When invoked as `ebzipinf.exe', listing information is the default action.\n"));
#endif
    printf(_("  Otherwise, compression is the default action.\n"));
    printf(_("\nReport bugs to %s.\n"), MAILING_ADDRESS);
    fflush(stdout);
}


/*
 * Compress files in `book' and output them under `out_top_path'.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
zip_book(book, out_top_path, book_path)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
{
    if (book->disc_code == EB_DISC_EB)
	return zip_eb_book(book, out_top_path, book_path);
    else
	return zip_epwing_book(book, out_top_path, book_path);
}


/*
 * Internal function for `zip_book'.
 * This is used to compress an EB book.
 */
static int
zip_eb_book(book, out_top_path, book_path)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
{
    EB_Subbook *subbook;
    char in_path_name[PATH_MAX + 1];
    char in_file_name[EB_MAX_FILE_NAME_LENGTH];
    int (*in_open_function) EB_P((EB_Zip *, const char *));
    char out_sub_path[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];
    mode_t out_directory_mode;
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
    eb_initialize_all_subbooks(book);

    /*
     * Compress a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Make an output directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s", out_top_path, subbook->directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Compress START file.
	 */
	do {
	    /* Try `START' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    if (eb_fix_file_name2(book->path, subbook->directory_name, 
		in_file_name) == 0) {
		in_open_function = eb_zopen_none;
		break;
	    }

	    /* Try `START.EBZ' for input. */
	    strcat(in_file_name, EB_SUFFIX_EBZ);
	    if (eb_fix_file_name2(book->path, subbook->directory_name, 
		in_file_name) == 0) {
		in_open_function = eb_zopen_ebzip;
		break;
	    }

	    /* No START file exists. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    in_open_function = NULL;
	} while (0);

	sprintf(in_path_name, "%s/%s/%s", book->path, subbook->directory_name,
	    in_file_name);
	compose_out_path_name2(out_top_path, subbook->directory_name,
	    in_file_name, EB_SUFFIX_EBZ, out_path_name);
	if (in_open_function == NULL) {
	    fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		in_path_name);
	} else {
	    zip_file(out_path_name, in_path_name, in_open_function);
	}
    }

    /*
     * Compress a language file.
     */
    do {
	/* Try `START' for input. */
	strcpy(in_file_name, EB_FILE_NAME_LANGUAGE);
	if (eb_fix_file_name(book->path, in_file_name) == 0) {
	    in_open_function = eb_zopen_none;
	    break;
	}

	/* Try `START.EBZ' for input. */
	strcat(in_file_name, EB_SUFFIX_EBZ);
	if (eb_fix_file_name(book->path, in_file_name) == 0) {
	    in_open_function = eb_zopen_ebzip;
	    break;
	}

	/* No START file exists. */
	strcpy(in_file_name, EB_FILE_NAME_LANGUAGE);
	in_open_function = NULL;
    } while (0);

    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    compose_out_path_name(out_top_path, in_file_name, EB_SUFFIX_EBZ,
	out_path_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	zip_file(out_path_name, in_path_name, in_open_function);
    }

    /*
     * Copy a catalog/catalogs file.
     */
    strcpy(in_file_name, EB_FILE_NAME_CATALOG);
    if (eb_fix_file_name(book->path, in_file_name) == 0)
	in_open_function = eb_zopen_none;
    else
	in_open_function = NULL;
    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    compose_out_path_name(out_top_path, in_file_name, EB_SUFFIX_NONE,
	out_path_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	copy_file(out_path_name, in_path_name);
    }

    return 0;
}


/*
 * Internal function for `zip_book'.
 * This is used to compress an EPWING book.
 */
static int
zip_epwing_book(book, out_top_path, book_path)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
{
    EB_Subbook *subbook;
    EB_Font *font;
    char in_path_name[PATH_MAX + 1];
    char in_file_name[EB_MAX_FILE_NAME_LENGTH];
    int (*in_open_function) EB_P((EB_Zip *, const char *));
    char out_sub_path[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];
    mode_t out_directory_mode;
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
    eb_initialize_all_subbooks(book);

    /*
     * Compress a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Make an output directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s", out_top_path, subbook->directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Make `data' sub directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s/%s", out_top_path,
	    subbook->directory_name, subbook->data_directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Compress HONMON file.
	 */
	do {
	    /* Try `HONMON2' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_HONMON2);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_epwing;
		break;
	    }

	    /* Try `HONMON' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_HONMON);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_none;
		break;
	    }

	    /* Try `HONMON.EBZ' for input. */
	    strcat(in_file_name, EB_SUFFIX_EBZ);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_ebzip;
		break;
	    }

	    /* No HONMON file exists. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    in_open_function = NULL;
	} while (0);

	sprintf(in_path_name, "%s/%s/%s/%s", book->path,
	    subbook->directory_name, subbook->data_directory_name,
	    in_file_name);
	compose_out_path_name3(out_top_path, subbook->directory_name,
	    subbook->data_directory_name, in_file_name, EB_SUFFIX_EBZ,
	    out_path_name);
	if (in_open_function == NULL) {
	    fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		in_path_name);
	} else {
	    zip_file(out_path_name, in_path_name, in_open_function);
	}

	/*
	 * Make `gaiji' sub directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s/%s", out_top_path,
	    subbook->directory_name, subbook->gaiji_directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Compress narrow font files.
	 */
	for (j = 0; j < EB_MAX_FONTS; j++) {
	    font = subbook->narrow_fonts + j;

	    do {
		/* Try original for input. */
		strcpy(in_file_name, font->file_name);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_none;
		    break;
		}

		/* Try `<original>.ebz' for input. */
		strcat(in_file_name, EB_SUFFIX_EBZ);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_ebzip;
		    break;
		}

		/* No font file exists. */
		strcpy(in_file_name, font->file_name);
		in_open_function = NULL;
	    } while (0);

	    sprintf(in_path_name, "%s/%s/%s/%s", book->path,
		subbook->directory_name, subbook->gaiji_directory_name,
		in_file_name);
	    compose_out_path_name3(out_top_path, subbook->directory_name,
		subbook->data_directory_name, in_file_name, EB_SUFFIX_EBZ,
		out_path_name);
	    if (in_open_function == NULL) {
		fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		    in_path_name);
	    } else {
		zip_file(out_path_name, in_path_name, in_open_function);
	    }
	}

	/*
	 * Compress wide font files.
	 */
	for (j = 0; j < EB_MAX_FONTS; j++) {
	    font = subbook->wide_fonts + j;

	    do {
		/* Try original for input. */
		strcpy(in_file_name, font->file_name);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_none;
		    break;
		}

		/* Try `<original>.ebz' for input. */
		strcat(in_file_name, EB_SUFFIX_EBZ);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_ebzip;
		    break;
		}

		/* No font file exists. */
		strcpy(in_file_name, font->file_name);
		in_open_function = NULL;
	    } while (0);

	    sprintf(in_path_name, "%s/%s/%s/%s", book->path,
		subbook->directory_name, subbook->gaiji_directory_name,
		in_file_name);
	    compose_out_path_name3(out_top_path, subbook->directory_name,
		subbook->gaiji_directory_name, in_file_name, EB_SUFFIX_EBZ,
		out_path_name);
	    if (in_open_function == NULL) {
		fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		    in_path_name);
	    } else {
		zip_file(out_path_name, in_path_name, in_open_function);
	    }
	}
    }

    /*
     * Copy a catalog/catalogs file.
     */
    strcpy(in_file_name, EB_FILE_NAME_CATALOGS);
    if (eb_fix_file_name(book->path, in_file_name) == 0)
	in_open_function = eb_zopen_none;
    else
	in_open_function = NULL;
    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    compose_out_path_name(out_top_path, in_file_name, EB_SUFFIX_NONE,
	out_path_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	copy_file(out_path_name, in_path_name);
    }

    return 0;
}


/*
 * Uncompress files in `book' and output them under `out_top_path'.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
unzip_book(book, out_top_path, book_path)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
{
    if (book->disc_code == EB_DISC_EB)
	return unzip_eb_book(book, out_top_path, book_path);
    else
	return unzip_epwing_book(book, out_top_path, book_path);
}


/*
 * Internal function for `unzip_book'.
 * This is used to compress an EB book.
 */
static int
unzip_eb_book(book, out_top_path, book_path)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
{
    EB_Subbook *subbook;
    char in_path_name[PATH_MAX + 1];
    char in_file_name[EB_MAX_FILE_NAME_LENGTH];
    int (*in_open_function) EB_P((EB_Zip *, const char *));
    char out_sub_path[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];
    mode_t out_directory_mode;
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
    eb_initialize_all_subbooks(book);

    /*
     * Uncompress a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Make an output directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s", out_top_path, subbook->directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Uncompress START file.
	 */
	do {
	    /* Try `START' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    if (eb_fix_file_name2(book->path, subbook->directory_name, 
		in_file_name) == 0) {
		in_open_function = eb_zopen_none;
		break;
	    }

	    /* Try `START.EBZ' for input. */
	    strcat(in_file_name, EB_SUFFIX_EBZ);
	    if (eb_fix_file_name2(book->path, subbook->directory_name, 
		in_file_name) == 0) {
		in_open_function = eb_zopen_ebzip;
		break;
	    }

	    /* No START file exists. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    in_open_function = NULL;
	} while (0);

	sprintf(in_path_name, "%s/%s/%s", book->path, subbook->directory_name,
	    in_file_name);
	compose_out_path_name2(out_top_path, subbook->directory_name,
	    in_file_name, EB_SUFFIX_NONE, out_path_name);
	if (in_open_function == NULL) {
	    fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		in_path_name);
	} else {
	    unzip_file(out_path_name, in_path_name, in_open_function);
	}
    }

    /*
     * Uncompress a language file.
     */
    do {
	/* Try `START' for input. */
	strcpy(in_file_name, EB_FILE_NAME_LANGUAGE);
	if (eb_fix_file_name(book->path, in_file_name) == 0) {
	    in_open_function = eb_zopen_none;
	    break;
	}
	/* Try `START.EBZ' for input. */
	strcat(in_file_name, EB_SUFFIX_EBZ);
	if (eb_fix_file_name(book->path, in_file_name) == 0) {
	    in_open_function = eb_zopen_ebzip;
	    break;
	}
	/* No START file exists. */
	strcpy(in_file_name, EB_FILE_NAME_LANGUAGE);
	in_open_function = NULL;
    } while (0);

    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    compose_out_path_name(out_top_path, in_file_name, EB_SUFFIX_NONE,
	out_path_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	unzip_file(out_path_name, in_path_name, in_open_function);
    }

    /*
     * Copy a catalog/catalogs file.
     */
    strcpy(in_file_name, EB_FILE_NAME_CATALOG);
    if (eb_fix_file_name(book->path, in_file_name) == 0)
	in_open_function = eb_zopen_none;
    else
	in_open_function = NULL;
    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    compose_out_path_name(out_top_path, in_file_name, EB_SUFFIX_NONE,
	out_path_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	copy_file(out_path_name, in_path_name);
    }

    return 0;
}


/*
 * Internal function for `unzip_book'.
 * This is used to compress an EPWING book.
 */
static int
unzip_epwing_book(book, out_top_path, book_path)
    EB_Book *book;
    const char *out_top_path;
    const char *book_path;
{
    EB_Subbook *subbook;
    EB_Font *font;
    char in_path_name[PATH_MAX + 1];
    char in_file_name[EB_MAX_FILE_NAME_LENGTH];
    int (*in_open_function) EB_P((EB_Zip *, const char *));
    char out_sub_path[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];
    mode_t out_directory_mode;
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
    eb_initialize_all_subbooks(book);

    /*
     * Uncompress a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Make an output directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s", out_top_path, subbook->directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Make `data' sub directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s/%s", out_top_path,
	    subbook->directory_name, subbook->data_directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Uncompress HONMON file.
	 */
	do {
	    /* Try `HONMON2' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_HONMON2);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_epwing;
		break;
	    }

	    /* Try `HONMON' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_HONMON);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_none;
		break;
	    }

	    /* Try `HONMON.EBZ' for input. */
	    strcat(in_file_name, EB_SUFFIX_EBZ);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_ebzip;
		break;
	    }

	    /* No HONMON file exists. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    in_open_function = NULL;
	} while (0);

	sprintf(in_path_name, "%s/%s/%s/%s", book->path,
	    subbook->directory_name, subbook->data_directory_name,
	    in_file_name);
	compose_out_path_name3(out_top_path, subbook->directory_name,
	    subbook->data_directory_name, in_file_name, EB_SUFFIX_NONE,
	    out_path_name);
	if (in_open_function == NULL) {
	    fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		in_path_name);
	} else {
	    unzip_file(out_path_name, in_path_name, in_open_function);
	}

	/*
	 * Make `gaiji' sub directory for the current subbook.
	 */
	sprintf(out_sub_path, "%s/%s/%s", out_top_path,
	    subbook->directory_name, subbook->gaiji_directory_name);
	if (!test_flag && make_missing_directory(out_sub_path,
	    out_directory_mode) < 0)
	    return -1;

	/*
	 * Uncompress narrow font files.
	 */
	for (j = 0; j < EB_MAX_FONTS; j++) {
	    font = subbook->narrow_fonts + j;
	    if (font->font_code == EB_FONT_INVALID)
		continue;

	    do {
		/* Try original for input. */
		strcpy(in_file_name, font->file_name);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_none;
		    break;
		}

		/* Try `<original>.ebz' for input. */
		strcat(in_file_name, EB_SUFFIX_EBZ);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_ebzip;
		    break;
		}

		/* No font file exists. */
		strcpy(in_file_name, font->file_name);
		in_open_function = NULL;
	    } while (0);

	    sprintf(in_path_name, "%s/%s/%s/%s", book->path,
		subbook->directory_name, subbook->gaiji_directory_name,
		in_file_name);
	    compose_out_path_name3(out_top_path, subbook->directory_name,
		subbook->gaiji_directory_name, in_file_name, EB_SUFFIX_NONE,
		out_path_name);
	    if (in_open_function == NULL) {
		fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		    in_path_name);
	    } else {
		unzip_file(out_path_name, in_path_name, in_open_function);
	    }
	}

	/*
	 * Uncompress wide font files.
	 */
	for (j = 0; j < EB_MAX_FONTS; j++) {
	    font = subbook->wide_fonts + j;
	    if (font->font_code == EB_FONT_INVALID)
		continue;

	    do {
		/* Try original for input. */
		strcpy(in_file_name, font->file_name);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_none;
		    break;
		}

		/* Try `<original>.ebz' for input. */
		strcat(in_file_name, EB_SUFFIX_EBZ);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_ebzip;
		    break;
		}

		/* No font file exists. */
		strcpy(in_file_name, font->file_name);
		in_open_function = NULL;
	    } while (0);

	    sprintf(in_path_name, "%s/%s/%s/%s", book->path,
		subbook->directory_name, subbook->gaiji_directory_name,
		in_file_name);
	    compose_out_path_name3(out_top_path, subbook->directory_name,
		subbook->gaiji_directory_name, in_file_name, EB_SUFFIX_NONE,
		out_path_name);
	    if (in_open_function == NULL) {
		fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		    in_path_name);
	    } else {
		unzip_file(out_path_name, in_path_name, in_open_function);
	    }
	}
    }

    /*
     * Copy a catalog/catalogs file.
     */
    strcpy(in_file_name, EB_FILE_NAME_CATALOGS);
    if (eb_fix_file_name(book->path, in_file_name) == 0)
	in_open_function = eb_zopen_none;
    else
	in_open_function = NULL;
    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    compose_out_path_name(out_top_path, in_file_name, EB_SUFFIX_NONE,
	out_path_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	copy_file(out_path_name, in_path_name);
    }

    return 0;
}


/*
 * List compressed book information.
 * If is succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
zipinfo_book(book, book_path)
    EB_Book *book;
    const char *book_path;
{
    if (book->disc_code == EB_DISC_EB)
	return zipinfo_eb_book(book, book_path);
    else
	return zipinfo_epwing_book(book, book_path);
}


/*
 * Internal function for `zipinfo_book'.
 * This is used to list files in an EB book.
 */
static int
zipinfo_eb_book(book, book_path)
    EB_Book *book;
    const char *book_path;
{
    EB_Subbook *subbook;
    char in_path_name[PATH_MAX + 1];
    char in_file_name[EB_MAX_FILE_NAME_LENGTH];
    int (*in_open_function) EB_P((EB_Zip *, const char *));
    int i;

    /*
     * If `out_top_path' and/or `book_path' represents "/", replace it
     * to an empty string.
     */
    if (strcmp(book_path, "/") == 0)
	book_path++;

    /*
     * Initialize variables.
     */
    eb_initialize_all_subbooks(book);

    /*
     * Inspect a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Inspect START file.
	 */
	do {
	    /* Try `START' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    if (eb_fix_file_name2(book->path, subbook->directory_name, 
		in_file_name) == 0) {
		in_open_function = eb_zopen_none;
		break;
	    }

	    /* Try `START.EBZ' for input. */
	    strcat(in_file_name, EB_SUFFIX_EBZ);
	    if (eb_fix_file_name2(book->path, subbook->directory_name, 
		in_file_name) == 0) {
		in_open_function = eb_zopen_ebzip;
		break;
	    }

	    /* No START file exists. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    in_open_function = NULL;
	} while (0);

	sprintf(in_path_name, "%s/%s/%s", book->path, subbook->directory_name,
	    in_file_name);
	if (in_open_function == NULL) {
	    fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		in_path_name);
	} else {
	    zipinfo_file(in_path_name, in_open_function);
	}
    }

    /*
     * Inspect a language file.
     */
    do {
	/* Try `START' for input. */
	strcpy(in_file_name, EB_FILE_NAME_LANGUAGE);
	if (eb_fix_file_name(book->path, in_file_name) == 0) {
	    in_open_function = eb_zopen_none;
	    break;
	}
	/* Try `START.EBZ' for input. */
	strcat(in_file_name, EB_SUFFIX_EBZ);
	if (eb_fix_file_name(book->path, in_file_name) == 0) {
	    in_open_function = eb_zopen_ebzip;
	    break;
	}
	/* No START file exists. */
	strcpy(in_file_name, EB_FILE_NAME_LANGUAGE);
	in_open_function = NULL;
    } while (0);

    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	zipinfo_file(in_path_name, in_open_function);
    }

    /*
     * Copy a catalog/catalogs file.
     */
    strcpy(in_file_name, EB_FILE_NAME_CATALOG);
    if (eb_fix_file_name(book->path, in_file_name) == 0)
	in_open_function = eb_zopen_none;
    else
	in_open_function = NULL;
    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	zipinfo_file(in_path_name, in_open_function);
    }

    return 0;
}


/*
 * Internal function for `zipinfo_book'.
 * This is used to list files in an EPWING book.
 */
static int
zipinfo_epwing_book(book, book_path)
    EB_Book *book;
    const char *book_path;
{
    EB_Subbook *subbook;
    EB_Font *font;
    char in_path_name[PATH_MAX + 1];
    char in_file_name[EB_MAX_FILE_NAME_LENGTH];
    int (*in_open_function) EB_P((EB_Zip *, const char *));
    int i, j;

    /*
     * If `out_top_path' and/or `book_path' represents "/", replace it
     * to an empty string.
     */
    if (strcmp(book_path, "/") == 0)
	book_path++;

    /*
     * Initialize variables.
     */
    eb_initialize_all_subbooks(book);

    /*
     * Uncompress a book.
     */
    for (i = 0; i < subbook_count; i++) {
	subbook = book->subbooks + subbook_list[i];

	/*
	 * Uncompress HONMON file.
	 */
	do {
	    /* Try `HONMON2' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_HONMON2);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_epwing;
		break;
	    }

	    /* Try `HONMON' for input. */
	    strcpy(in_file_name, EB_FILE_NAME_HONMON);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_none;
		break;
	    }

	    /* Try `HONMON.EBZ' for input. */
	    strcat(in_file_name, EB_SUFFIX_EBZ);
	    if (eb_fix_file_name3(book->path, subbook->directory_name, 
		subbook->data_directory_name, in_file_name) == 0) {
		in_open_function = eb_zopen_ebzip;
		break;
	    }

	    /* No HONMON file exists. */
	    strcpy(in_file_name, EB_FILE_NAME_START);
	    in_open_function = NULL;
	} while (0);

	sprintf(in_path_name, "%s/%s/%s/%s", book->path,
	    subbook->directory_name, subbook->data_directory_name,
	    in_file_name);
	if (in_open_function == NULL) {
	    fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		in_path_name);
	} else {
	    zipinfo_file(in_path_name, in_open_function);
	}

	/*
	 * Uncompress narrow font files.
	 */
	for (j = 0; j < EB_MAX_FONTS; j++) {
	    font = subbook->narrow_fonts + j;

	    do {
		/* Try original for input. */
		strcpy(in_file_name, font->file_name);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_none;
		    break;
		}

		/* Try `<original>.ebz' for input. */
		strcat(in_file_name, EB_SUFFIX_EBZ);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_ebzip;
		    break;
		}

		/* No font file exists. */
		strcpy(in_file_name, font->file_name);
		in_open_function = NULL;
	    } while (0);

	    sprintf(in_path_name, "%s/%s/%s/%s", book->path,
		subbook->directory_name, subbook->gaiji_directory_name,
		in_file_name);
	    if (in_open_function == NULL) {
		fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		    in_path_name);
	    } else {
		zipinfo_file(in_path_name, in_open_function);
	    }
	}

	/*
	 * Uncompress wide font files.
	 */
	for (j = 0; j < EB_MAX_FONTS; j++) {
	    font = subbook->wide_fonts + j;

	    do {
		/* Try original for input. */
		strcpy(in_file_name, font->file_name);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_none;
		    break;
		}

		/* Try `<original>.ebz' for input. */
		strcat(in_file_name, EB_SUFFIX_EBZ);
		if (eb_fix_file_name3(book->path, subbook->directory_name, 
		    subbook->gaiji_directory_name, in_file_name) == 0) {
		    in_open_function = eb_zopen_ebzip;
		    break;
		}

		/* No font file exists. */
		strcpy(in_file_name, font->file_name);
		in_open_function = NULL;
	    } while (0);

	    sprintf(in_path_name, "%s/%s/%s/%s", book->path,
		subbook->directory_name, subbook->gaiji_directory_name,
		in_file_name);
	    if (in_open_function == NULL) {
		fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
		    in_path_name);
	    } else {
		zipinfo_file(in_path_name, in_open_function);
	    }
	}
    }

    /*
     * Copy a catalog/catalogs file.
     */
    strcpy(in_file_name, EB_FILE_NAME_CATALOGS);
    if (eb_fix_file_name(book->path, in_file_name) == 0)
	in_open_function = eb_zopen_none;
    else
	in_open_function = NULL;
    sprintf(in_path_name, "%s/%s", book->path, in_file_name);
    if (in_open_function == NULL) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
	    in_path_name);
    } else {
	zipinfo_file(in_path_name, in_open_function);
    }

    return 0;
}


/*
 * Compress a file `in_file_name'.
 * It compresses the existed file nearest to the beginning of the list.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
zip_file(out_file_name, in_file_name, in_open_function)
    const char *out_file_name;
    const char *in_file_name;
    int (*in_open_function) EB_P((EB_Zip *, const char *));
{
    EB_Zip in_zip, out_zip;
    unsigned char *in_buffer = NULL, *out_buffer = NULL;
    size_t in_total_length, out_total_length;
    int in_file = -1, out_file = -1;
    ssize_t in_length;
    size_t out_length;
    struct stat in_status, out_status;
    off_t slice_location = 0;
    off_t next_location;
    size_t index_length;
    int information_interval;
    int total_slices;
    int i;

    /*
     * Output information.
     */
    if (!quiet_flag) {
	printf(_("==> compress %s <==\n"), in_file_name);
	printf(_("output to %s\n"), out_file_name);
	fflush(stdout);
    }

    /*
     * Get status of the input file.
     */
    if (stat(in_file_name, &in_status) < 0 || !S_ISREG(in_status.st_mode)) {
        fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
            in_file_name);
        goto failed;
    }

    /*
     * Do nothing if the `in_file_name' and `out_file_name' are the same.
     */
    if (is_same_file(out_file_name, in_file_name)) {
	if (!quiet_flag) {
	    printf(_("the input and output files are the same, skipped.\n\n"));
	    fflush(stdout);
	}
	return 0;
    }

    /*
     * Allocate memories for in/out buffers.
     */
    in_buffer = (unsigned char *)malloc(EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL);
    if (in_buffer == NULL) {
	fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	goto failed;
    }

    out_buffer = (unsigned char *) malloc((EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL)
	+ EB_SIZE_EBZIP_MARGIN);
    if (out_buffer == NULL) {
	fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	goto failed;
    }
    
    /*
     * If the file `out_file_name' already exists, confirm and unlink it.
     */
    if (!test_flag
	&& stat(out_file_name, &out_status) == 0
	&& S_ISREG(out_status.st_mode)) {
	if (overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!quiet_flag) {
		fputs(_("already exists, skip the file\n\n"), stderr);
		fflush(stderr);
	    }
	    return 0;
	} else if (overwrite_mode == EBZIP_OVERWRITE_QUERY) {
	    int y_or_n;

	    fprintf(stderr, _("\nthe file already exists: %s\n"),
		out_file_name);
	    y_or_n = query_y_or_n(_("do you wish to overwrite (y or n)? "));
	    fputc('\n', stderr);
	    fflush(stderr);
	    if (!y_or_n)
		return 0;
        }
	if (unlink(out_file_name) < 0) {
	    fprintf(stderr, _("%s: failed to unlink the file: %s\n"),
		invoked_name, out_file_name);
	    goto failed;
	}
    }

    /*
     * Open files.
     */
    in_file = in_open_function(&in_zip, in_file_name);
    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }
    if (!test_flag) {
	trap_file_name = out_file_name;
#ifdef SIGHUP
	signal(SIGHUP, trap);
#endif
	signal(SIGINT, trap);
#ifdef SIGQUIT
	signal(SIGQUIT, trap);
#endif
#ifdef SIGTERM
	signal(SIGTERM, trap);
#endif

#ifdef O_CREAT
	out_file = open(out_file_name, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	    0666 ^ get_umask());
#else
	out_file = creat(out_file_name, 0666 ^ get_umask());
#endif
	if (out_file < 0) {
	    fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	trap_file = out_file;
    }

    /*
     * Initialize `zip'.
     */
    out_zip.code = EB_ZIP_EBZIP1;
    out_zip.slice_size = EB_SIZE_PAGE << zip_level;
    out_zip.file_size = in_zip.file_size;
    out_zip.crc = 1;
    out_zip.mtime = in_status.st_mtime;

    if (out_zip.file_size < 1 << 16)
	out_zip.index_width = 2;
    else if (out_zip.file_size < 1 << 24)
	out_zip.index_width = 3;
    else 
	out_zip.index_width = 4;

    /*
     * Fill header and index part with `\0'.
     * 
     * Original File:
     *   +-----------------+-----------------+-....-+-------+
     *   |     slice 1     |     slice 2     |      |slice N| [EOF]
     *   |                 |                 |      |       |
     *   +-----------------+-----------------+-....-+-------+          
     *        slice_size        slice_size            odds
     *   <-------------------- file size ------------------->
     * 
     * Compressed file:
     *   +------+---------+...+---------+---------+----------+...+-
     *   |Header|index for|   |index for|index for|compressed|   |
     *   |      | slice 1 |   | slice N |   EOF   |  slice 1 |   |
     *   +------+---------+...+---------+---------+----------+...+-
     *             index         index     index
     *             width         width     width
     *          <---------  index_length --------->
     *
     *     total_slices = N = (file_size + slice_size - 1) / slice_size
     *     index_length = (N + 1) * index_width
     */
    total_slices = (out_zip.file_size + out_zip.slice_size - 1)
	/ out_zip.slice_size;
    index_length = (total_slices + 1) * out_zip.index_width;
    memset(out_buffer, '\0', out_zip.slice_size);

    if (!test_flag) {
	for (i = index_length + EB_SIZE_EBZIP_HEADER;
	     out_zip.slice_size <= i; i -= out_zip.slice_size) {
	    if (write(out_file, out_buffer, out_zip.slice_size)
		!= out_zip.slice_size) {
		fprintf(stderr, _("%s: failed to write to the file: %s\n"),
		    invoked_name, out_file_name);
		goto failed;
	    }
	}
	if (0 < i) {
	    if (write(out_file, out_buffer, i) != i) {
		fprintf(stderr, _("%s: failed to write to the file: %s\n"),
		    invoked_name, out_file_name);
		goto failed;
	    }
	}
    }

    /*
     * Read a slice from the input file, compress it, and then
     * write it to the output file.
     */
    in_total_length = 0;
    out_total_length = 0;
    information_interval = INFORMATION_INTERVAL_FACTOR >> zip_level;
    for (i = 0; i < total_slices; i++) {
	/*
	 * Read a slice from the original file.
	 */
	if (eb_zlseek(&in_zip, in_file, in_total_length, SEEK_SET) < 0) {
	    fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	}
	in_length = eb_zread(&in_zip, in_file, (char *)in_buffer,
	    out_zip.slice_size);
	if (in_length < 0) {
	    fprintf(stderr, _("%s: failed to read from the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	} else if (in_length == 0) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	} else if (in_length != out_zip.slice_size
	    && in_total_length + in_length != out_zip.file_size) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	}

	/*
	 * Update CRC.  (Calculate adler32 again.)
	 */
	out_zip.crc = adler32((uLong)out_zip.crc, (Bytef *)in_buffer,
	    (uInt)in_length);

	/*
	 * If this is last slice and its length is shorter than
	 * `slice_size', fill `\0'.
	 */
	if (in_length < out_zip.slice_size) {
	    memset(in_buffer + in_length, '\0',
		out_zip.slice_size - in_length);
	    in_length = out_zip.slice_size;
	}

	/*
	 * Compress the slice.
	 */
	if (ebzip1_slice((char *)out_buffer, &out_length, (char *)in_buffer,
	    out_zip.slice_size) < 0) {
	    fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	    goto failed;
	}
	if (out_zip.slice_size <= out_length) {
	    memcpy(out_buffer, in_buffer, out_zip.slice_size);
	    out_length = out_zip.slice_size;
	}

	/*
	 * Write the slice to the zip file.
	 * If the length of the zipped slice is not shorter than
	 * original, write orignal slice.
	 */
	if (!test_flag) {
	    slice_location = lseek(out_file, 0, SEEK_END);
	    if (slice_location < 0) {
		fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	    if (write(out_file, out_buffer, out_length) != out_length) {
		fprintf(stderr, _("%s: failed to write to the file: %s\n"),
		    invoked_name, out_file_name);
		goto failed;
	    }
	}

	/*
	 * Write an index for the slice.
	 */
	next_location = slice_location + out_length;
	switch (out_zip.index_width) {
	case 2:
	    out_buffer[0] = (slice_location >> 8) & 0xff;
	    out_buffer[1] = slice_location & 0xff;
	    out_buffer[2] = (next_location >> 8) & 0xff;
	    out_buffer[3] = next_location & 0xff;
	    break;
	case 3:
	    out_buffer[0] = (slice_location >> 16) & 0xff;
	    out_buffer[1] = (slice_location >> 8) & 0xff;
	    out_buffer[2] = slice_location & 0xff;
	    out_buffer[3] = (next_location >> 16) & 0xff;
	    out_buffer[4] = (next_location >> 8) & 0xff;
	    out_buffer[5] = next_location & 0xff;
	    break;
	case 4:
	    out_buffer[0] = (slice_location >> 24) & 0xff;
	    out_buffer[1] = (slice_location >> 16) & 0xff;
	    out_buffer[2] = (slice_location >> 8) & 0xff;
	    out_buffer[3] = slice_location & 0xff;
	    out_buffer[4] = (next_location >> 24) & 0xff;
	    out_buffer[5] = (next_location >> 16) & 0xff;
	    out_buffer[6] = (next_location >> 8) & 0xff;
	    out_buffer[7] = next_location & 0xff;
	    break;
	}

	if (!test_flag) {
	    if (lseek(out_file, EB_SIZE_EBZIP_HEADER + i * out_zip.index_width,
		SEEK_SET) < 0) {
		fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	    if (write(out_file, out_buffer, out_zip.index_width * 2)
		!= out_zip.index_width * 2) {
		fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	}

	in_total_length += in_length;
	out_total_length += out_length + out_zip.index_width;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!quiet_flag && i % information_interval + 1
	    == information_interval) {
	    printf(_("%4.1f%% done (%lu / %lu bytes)\n"),
		(double)(i + 1) * 100.0 / (double)total_slices,
		(unsigned long)in_total_length,
		(unsigned long)in_zip.file_size);
	    fflush(stdout);
	}
    }

    /*
     * Write a header part (22 bytes):
     *     magic-id		5   bytes  ( 0 ...  4)
     *     zip-mode		4/8 bytes  ( 5)
     *     slice_size		4/8 bytes  ( 5)
     *     (reserved)		4   bytes  ( 6 ...  9)
     *     file_size		4   bytes  (10 ... 13)
     *     crc			4   bytes  (14 ... 17)
     *     mtime		4   bytes  (18 ... 21)
     */
    memcpy(out_buffer, "EBZip", 5);
    out_buffer[ 5] = (EB_ZIP_EBZIP1 << 4) + (zip_level & 0x0f);
    out_buffer[ 6] = 0;
    out_buffer[ 7] = 0;
    out_buffer[ 8] = 0;
    out_buffer[ 9] = 0;
    out_buffer[10] = (out_zip.file_size >> 24) & 0xff;
    out_buffer[11] = (out_zip.file_size >> 16) & 0xff;
    out_buffer[12] = (out_zip.file_size >> 8) & 0xff;
    out_buffer[13] = out_zip.file_size & 0xff;
    out_buffer[14] = (out_zip.crc >> 24) & 0xff;
    out_buffer[15] = (out_zip.crc >> 16) & 0xff;
    out_buffer[16] = (out_zip.crc >> 8) & 0xff;
    out_buffer[17] = out_zip.crc & 0xff;
    out_buffer[18] = (out_zip.mtime >> 24) & 0xff;
    out_buffer[19] = (out_zip.mtime >> 16) & 0xff;
    out_buffer[20] = (out_zip.mtime >> 8) & 0xff;
    out_buffer[21] = out_zip.mtime & 0xff;

    if (!test_flag) {
	if (lseek(out_file, 0, SEEK_SET) < 0) {
	    fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	if (write(out_file, out_buffer, EB_SIZE_EBZIP_HEADER)
	    != EB_SIZE_EBZIP_HEADER) {
	    fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
    }

    /*
     * Output the result information unless quiet mode.
     */
    out_total_length += EB_SIZE_EBZIP_HEADER + out_zip.index_width;
    if (!quiet_flag) {
	printf(_("completed (%lu / %lu bytes)\n"),
	    (unsigned long)in_zip.file_size, (unsigned long)in_zip.file_size);
	if (in_total_length != 0) {
	    printf(_("%lu -> %lu bytes (%4.1f%%)\n\n"),
		(unsigned long)in_zip.file_size,
		(unsigned long)out_total_length, 
		(double)out_total_length * 100.0 / (double)in_zip.file_size);
	}
	fflush(stdout);
    }

    /*
     * Close files.
     */
    eb_zclose(&in_zip, in_file);
    in_file = -1;

    if (!test_flag) {
	close(out_file);
	out_file = -1;
	trap_file = -1;
	trap_file_name = NULL;
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
    }

    /*
     * Delete an original file unless `keep_flag' is set.
     */
    if (!test_flag  && !keep_flag && unlink(in_file_name) < 0) {
	fprintf(stderr, _("%s: failed to unlink the file: %s\n"), invoked_name,
	    in_file_name);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and utime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
    if (!test_flag) {
	struct utimbuf utim;

#if defined(HAVE_CHOWN)
	chown(out_file_name, in_status.st_uid, in_status.st_gid);
#endif
#if defined(HAVE_CHMOD)
	chmod(out_file_name, in_status.st_mode);
#endif
#if defined(HAVE_UTIME)
	utim.actime = in_status.st_atime;
	utim.modtime = in_status.st_mtime;
	utime(out_file_name, &utim);
#endif
    }

    /*
     * Dispose memories.
     */
    free(in_buffer);
    free(out_buffer);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (in_buffer != NULL)
	free(in_buffer);
    if (out_buffer != NULL)
	free(out_buffer);

    if (0 <= in_file)
	close(in_file);
    if (0 <= out_file) {
	close(out_file);
	trap_file = -1;
	trap_file_name = NULL;
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
    }

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}


/*
 * Uncompress a file `in_file_name'.
 * It uncompresses the existed file nearest to the beginning of the
 * list.  If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
unzip_file(out_file_name, in_file_name, in_open_function)
    const char *out_file_name;
    const char *in_file_name;
    int (*in_open_function) EB_P((EB_Zip *, const char *));
{
    EB_Zip in_zip;
    unsigned char *buffer = NULL;
    size_t total_length;
    int in_file = -1, out_file = -1;
    size_t length;
    struct stat in_status, out_status;
    unsigned int crc = 1;
    int information_interval;
    int total_slices;
    int i;

    /*
     * Simply copy a file, when an input file is not compressed.
     */
    if (in_open_function == eb_zopen_none)
	return copy_file(out_file_name, in_file_name);

    /*
     * Output file name information.
     */
    if (!quiet_flag) {
	printf(_("==> uncompress %s <==\n"), in_file_name);
	printf(_("output to %s\n"), out_file_name);
	fflush(stdout);
    }

    /*
     * Get status of the input file.
     */
    if (stat(in_file_name, &in_status) < 0 || !S_ISREG(in_status.st_mode)) {
        fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
            in_file_name);
        goto failed;
    }

    /*
     * Do nothing if the `in_file_name' and `out_file_name' are the same.
     */
    if (is_same_file(out_file_name, in_file_name)) {
	if (!quiet_flag) {
	    printf(_("the input and output files are the same, skipped.\n\n"));
	    fflush(stdout);
	}
	return 0;
    }

    /*
     * Allocate memories for in/out buffers.
     */
    buffer = (unsigned char *)malloc(EB_SIZE_PAGE << EB_MAX_EBZIP_LEVEL);
    if (buffer == NULL) {
	fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	goto failed;
    }

    /*
     * If the file `out_file_name' already exists, confirm and unlink it.
     */
    if (!test_flag
	&& stat(out_file_name, &out_status) == 0
	&& S_ISREG(out_status.st_mode)) {
	if (overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!quiet_flag) {
		fputs(_("already exists, skip the file\n\n"), stderr);
		fflush(stderr);
	    }
	    return 0;
	} else if (overwrite_mode == EBZIP_OVERWRITE_QUERY) {
	    int y_or_n;

	    fprintf(stderr, _("\nthe file already exists: %s\n"),
		out_file_name);
	    y_or_n = query_y_or_n(_("do you wish to overwrite (y or n)? "));
	    fputc('\n', stderr);
	    fflush(stderr);
	    if (!y_or_n)
		return 0;
        }
	if (unlink(out_file_name) < 0) {
	    fprintf(stderr, _("%s: failed to unlink the file: %s\n"),
		invoked_name, out_file_name);
	    goto failed;
	}
    }

    /*
     * Open files.
     */
    in_file = in_open_function(&in_zip, in_file_name);
    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }
    if (!test_flag) {
	trap_file_name = out_file_name;
#ifdef SIGHUP
	signal(SIGHUP, trap);
#endif
	signal(SIGINT, trap);
#ifdef SIGQUIT
	signal(SIGQUIT, trap);
#endif
#ifdef SIGTERM
	signal(SIGTERM, trap);
#endif

#ifdef O_CREAT
	out_file = open(out_file_name, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	    0666 ^ get_umask());
#else
	out_file = creat(out_file_name, 0666 ^ get_umask());
#endif
	if (out_file < 0) {
	    fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	trap_file = out_file;
    }

    /*
     * Read a slice from the input file, uncompress it if required,
     * and then write it to the output file.
     */
    total_length = 0;
    total_slices = (in_zip.file_size + in_zip.slice_size - 1)
	/ in_zip.slice_size;
    information_interval = INFORMATION_INTERVAL_FACTOR;
    for (i = 0; i < total_slices; i++) {
	/*
	 * Read the slice from `file' and unzip it, if it is zipped.
	 * We assumes the slice is not compressed if its length is
	 * equal to `slice_size'.
	 */
	if (eb_zlseek(&in_zip, in_file, total_length, SEEK_SET) < 0) {
	    fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	}
	length = eb_zread(&in_zip, in_file, (char *)buffer, in_zip.slice_size);
	if (length < 0) {
	    fprintf(stderr, _("%s: failed to read from the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	} else if (length == 0) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	} else if (length != in_zip.slice_size
	    && total_length + length != in_zip.file_size) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	}

	/*
	 * Update CRC.  (Calculate adler32 again.)
	 */
	if (in_zip.code == EB_ZIP_EBZIP1)
	    crc = adler32((uLong)crc, (Bytef *)buffer, (uInt)length);

	/*
	 * Write the slice to `out_file'.
	 */
	if (!test_flag) {
	    if (write(out_file, buffer, length) != length) {
		fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	}
	total_length += length;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!quiet_flag && i % information_interval + 1
	    == information_interval) {
	    printf(_("%4.1f%% done (%lu / %lu bytes)\n"),
		(double)(i + 1) * 100.0 / (double)total_slices,
		(unsigned long)total_length, (unsigned long)in_zip.file_size);
	    fflush(stdout);
	}
    }

    /*
     * Output the result unless quiet mode.
     */
    if (!quiet_flag) {
	printf(_("completed (%lu / %lu bytes)\n"),
	    (unsigned long)in_zip.file_size, (unsigned long)in_zip.file_size);
	printf(_("%lu -> %lu bytes\n\n"),
	    (unsigned long)in_status.st_size, (unsigned long)total_length);
	fflush(stdout);
    }

    /*
     * Close files.
     */
    eb_zclose(&in_zip, in_file);
    in_file = -1;

    if (!test_flag) {
	close(out_file);
	out_file = -1;
	trap_file = -1;
	trap_file_name = NULL;
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
    }

    /*
     * Check for CRC.
     */
    if (in_zip.code == EB_ZIP_EBZIP1 && in_zip.crc != crc) {
	fprintf(stderr, _("%s: CRC error: %s\n"), invoked_name, out_file_name);
	goto failed;
    }

    /*
     * Delete an original file unless `keep_flag' is set.
     */
    if (!test_flag  && !keep_flag && unlink(in_file_name) < 0) {
	fprintf(stderr, _("%s: failed to unlink the file: %s\n"), invoked_name,
	    in_file_name);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and mtime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
    if (!test_flag) {
	struct utimbuf utim;

#if defined(HAVE_UTIME)
	utim.actime = in_status.st_atime;
	utim.modtime = in_status.st_mtime;
	utime(out_file_name, &utim);
#endif
    }

    /*
     * Dispose memories.
     */
    free(buffer);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (buffer != NULL)
	free(buffer);

    if (0 <= in_file)
	close(in_file);
    if (0 <= out_file) {
	close(out_file);
	trap_file = -1;
	trap_file_name = NULL;
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
    }

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}


/*
 * Output status of a file `file_name'.
 * It outputs status of the existed file nearest to the beginning of
 * the list.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
zipinfo_file(in_file_name, in_open_function)
    const char *in_file_name;
    int (*in_open_function) EB_P((EB_Zip *, const char *));
{
    EB_Zip in_zip;
    int in_file = -1;
    struct stat in_status;

    /*
     * Output file name information.
     */
    printf("==> %s <==\n", in_file_name);
    fflush(stdout);

    /*
     * Open the file.
     */
    in_file = in_open_function(&in_zip, in_file_name);
    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }

    /*
     * Close the file.
     */
    close(in_file);

    /*
     * Output information.
     */
    if (in_zip.code == EB_ZIP_NONE)
	printf(_("%lu bytes (not compressed)\n"),
	    (unsigned long)in_status.st_size);
    else {
	printf(_("%lu -> %lu bytes "),
	    (unsigned long)in_zip.file_size, (unsigned long)in_status.st_size);
	if (in_zip.file_size == 0)
	    fputs(_("(empty original file, "), stdout);
	else {
	    printf("(%4.1f%%, ", (double)in_status.st_size * 100.0
		/ (double)in_zip.file_size);
	}
	if (in_zip.code == EB_ZIP_EBZIP1)
	    printf(_("ebzip level %d compression)\n"), in_zip.zip_level);
	else
	    printf(_("EPWING compression)\n"));
    }

    fputc('\n', stdout);
    fflush(stdout);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= in_file)
	close(in_file);

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}


/*
 * Copy a file.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
copy_file(out_file_name, in_file_name)
    const char *out_file_name;
    const char *in_file_name;
{
    unsigned char buffer[EB_SIZE_PAGE];
    size_t total_length;
    struct stat in_status, out_status;
    int in_file = -1, out_file = -1;
    size_t in_length;
    int information_interval;
    int total_slices;
    int i;

    /*
     * Check for the input file.
     */
    if (stat(in_file_name, &in_status) != 0 || !S_ISREG(in_status.st_mode)) {
	fprintf(stderr, _("%s: no such file: %s\n"), invoked_name, 
	    in_file_name);
	goto failed;
    }

    /*
     * Output file name information.
     */
    if (!quiet_flag) {
	printf(_("==> copy %s <==\n"), in_file_name);
	printf(_("output to %s\n"), out_file_name);
	fflush(stdout);
    }

    /*
     * Do nothing if the `in_file_name' and `out_file_name' are the same.
     */
    if (is_same_file(out_file_name, in_file_name)) {
	if (!quiet_flag) {
	    printf(_("the input and output files are the same, skipped.\n\n"));
	    fflush(stdout);
	}
	return 0;
    }

    /*
     * When test mode, return immediately.
     */
    if (test_flag) {
	fputc('\n', stderr);
	fflush(stderr);
	return 0;
    }

    /*
     * If the file to be output already exists, confirm and unlink it.
     */
    if (!test_flag
	&& stat(out_file_name, &out_status) == 0
	&& S_ISREG(out_status.st_mode)) {
	if (overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!quiet_flag) {
		fputs(_("already exists, skip the file\n\n"), stderr);
		fflush(stderr);
	    }
	    return 0;
	}
	if (overwrite_mode == EBZIP_OVERWRITE_QUERY) {
	    int y_or_n;

	    fprintf(stderr, _("\nthe file already exists: %s\n"),
		out_file_name);
	    y_or_n = query_y_or_n(_("do you wish to overwrite (y or n)? "));
	    fputc('\n', stderr);
	    fflush(stderr);
	    if (!y_or_n)
		return 0;
        }
	if (unlink(out_file_name) < 0) {
	    fprintf(stderr, _("%s: failed to unlink the file: %s\n"),
		invoked_name, out_file_name);
	    goto failed;
	}
    }

    /*
     * Open files.
     */
    in_file = open(in_file_name, O_RDONLY | O_BINARY);
    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }
    if (!test_flag) {
	trap_file_name = out_file_name;
#ifdef SIGHUP
	signal(SIGHUP, trap);
#endif
	signal(SIGINT, trap);
#ifdef SIGQUIT
	signal(SIGQUIT, trap);
#endif
#ifdef SIGTERM
	signal(SIGTERM, trap);
#endif

#ifdef O_CREAT
	out_file = open(out_file_name, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	    0666 ^ get_umask());
#else
	out_file = creat(out_file_name, 0666 ^ get_umask());
#endif
	if (out_file < 0) {
	    fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	trap_file = out_file;
    }

    /*
     * Read data from the input file, compress the data, and then
     * write them to the output file.
     */
    total_length = 0;
    total_slices = (in_status.st_size + EB_SIZE_PAGE - 1) / EB_SIZE_PAGE;
    information_interval = INFORMATION_INTERVAL_FACTOR;
    for (i = 0; i < total_slices; i++) {
	/*
	 * Read data from `in_file', and write them to `out_file'.
	 */
	in_length = read(in_file, buffer, EB_SIZE_PAGE);
	if (in_length < 0) {
	    fprintf(stderr, _("%s: failed to read from the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	} else if (in_length == 0) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	} else if (in_length != EB_SIZE_PAGE
	    && total_length + in_length != in_status.st_size) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	}

	/*
	 * Write decoded data to `out_file'.
	 */
	if (!test_flag) {
	    if (write(out_file, buffer, in_length) != in_length) {
		fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	}
	total_length += in_length;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!quiet_flag
	    && i % information_interval + 1 == information_interval) {
	    printf(_("%4.1f%% done (%lu / %lu bytes)\n"),
		(double)(i + 1) * 100.0 / (double)total_slices,
		(unsigned long)total_length, (unsigned long)in_status.st_size);
	    fflush(stdout);
	}
    }

    /*
     * Output the result unless quiet mode.
     */
    if (!quiet_flag) {
	printf(_("completed (%lu / %lu bytes)\n"),
	    (unsigned long)total_length, (unsigned long)total_length);
	fputc('\n', stdout);
	fflush(stdout);
    }

    /*
     * Close files.
     */
    close(in_file);
    in_file = -1;

    if (!test_flag) {
	close(out_file);
	out_file = -1;
	trap_file = -1;
	trap_file_name = NULL;
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
    }

    /*
     * Delete an original file unless `keep_flag' is set.
     */
    if (!test_flag  && !keep_flag && unlink(in_file_name) < 0) {
	fprintf(stderr, _("%s: failed to unlink the file: %s\n"), invoked_name,
	    in_file_name);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and mtime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
    if (!test_flag) {
	struct utimbuf utim;

#if defined(HAVE_UTIME)
	utim.actime = in_status.st_atime;
	utim.modtime = in_status.st_mtime;
	utime(out_file_name, &utim);
#endif
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= in_file)
	close(in_file);
    if (0 <= out_file)
	close(out_file);

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}


/*
 * Signal handler.
 */
static RETSIGTYPE
trap(signal_number)
    int signal_number;
{
    if (0 <= trap_file)
	close(trap_file);
    if (trap_file_name != NULL)
	unlink(trap_file_name);
    
    exit(1);

    /* not reached */
#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}
