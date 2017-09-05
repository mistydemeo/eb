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
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

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

#ifndef HAVE_STRUCT_UTIMBUF
struct utimbuf {
    long actime;
    long modtime;
};
#endif

#include <zlib.h>

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
 * Defaults and limitations.
 */
#define DEFAULT_ZIP_LEVEL		0
#define DEFAULT_BOOK_DIRECTORY          "."
#define DEFAULT_OUTPUT_DIRECTORY        "."

#define MAXLEN_FILE_TYPE_NAME		8
#define STATUS_INTERVAL_FACTOR		1024


/*
 * Command line options.
 */
static const char *short_options = "c:fhikl:no:qs:S:tT:uvz";
static struct option long_options[] = {
    {"case",              required_argument, NULL, 'c'},
    {"force-overwrite",   no_argument,       NULL, 'f'},
    {"help",              no_argument,       NULL, 'h'},
    {"information",       no_argument,       NULL, 'i'},
    {"keep",              no_argument,       NULL, 'k'},
    {"level",             required_argument, NULL, 'l'},
    {"no-overwrite",      no_argument,       NULL, 'n'},
    {"output-directory",  required_argument, NULL, 'o'},
    {"quiet",             no_argument,       NULL, 'q'},
    {"silent",            no_argument,       NULL, 'q'},
    {"suffix",            required_argument, NULL, 's'},
    {"subbook",           required_argument, NULL, 'S'},
    {"test",              no_argument,       NULL, 't'},
    {"uncompress",        no_argument,       NULL, 'u'},
    {"version",           no_argument,       NULL, 'v'},
    {"compress",          no_argument,       NULL, 'z'},
    {NULL, 0, NULL, 0}
};

/*
 * Generic name of the program.
 */
const char *program_name = "ebzip";

/*
 * Program version.
 */
const char *program_version = VERSION;

/*
 * Actual program name. (argv[0])
 */
const char *invoked_name;

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
char subname_list[EB_MAX_SUBBOOKS][EB_MAXLEN_BASENAME + 1];
int subname_count = 0;

/*
 * A list of subbook codes to be compressed/uncompressed.
 */
EB_Subbook_Code sub_list[EB_MAX_SUBBOOKS];
int sub_count = 0;

/*
 * Zip level.
 */
int zip_level = DEFAULT_ZIP_LEVEL;

/*
 * Filename case and suffix.
 */
EB_Case_Code filename_case = -1;
EB_Suffix_Code filename_suffix = -1;

/*
 * Flags.
 */
static int keep_flag = 0;
static int quiet_flag = 0;
static int test_flag = 0;

/*
 * Filename to be deleted and file to be closed when signal is received.
 */
static const char *trap_filename = NULL;
static int trap_file = -1;

/*
 * Unexported functions.
 */
static int parse_subname_argument EB_P((const char *));
static int parse_zip_level EB_P((const char *));
static EB_Subbook_Code find_subbook EB_P((EB_Book *, const char *));
static void output_help EB_P((void));
static void output_try_help EB_P((void));
static void output_version EB_P((void));
static int zip_book EB_P((EB_Book *, const char *));
static int unzip_book EB_P((EB_Book *, const char *));
static int zipinfo_book EB_P((EB_Book *));
static void fix_filename EB_P((char *, EB_Case_Code, EB_Suffix_Code));
static int zip_file EB_P((const char *, const char *));
static int unzip_file EB_P((const char *, const char *));
static int zipinfo_file EB_P((const char *, const char *));
static int copy_file EB_P((const char *, const char *));
static RETSIGTYPE trap EB_P((int));

/*
 * External functions.
 */
extern int zip_mode1 EB_P((char *, size_t *, char *, size_t));

int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Subbook_Code sub;
    const char *book_path;
    char out_path[PATH_MAX + 1];
    int ch;
    int i;
    char *invoked_basename;

    invoked_name = argv[0];
    strcpy(out_path, DEFAULT_OUTPUT_DIRECTORY);

    /*
     * Set default action.
     */
    invoked_basename = strrchr(argv[0], '/');
    if (invoked_basename == NULL)
	invoked_basename = argv[0];
    else
	invoked_basename++;

    /*
     * Determine the default action.
     */
#ifndef DOS_FILE_PATH
    if (strcmp(invoked_basename, "ebunzip") == 0)
	action_mode = EBZIP_ACTION_UNZIP;
    else if (strcmp(invoked_basename, "ebzipinfo") == 0)
	action_mode = EBZIP_ACTION_INFO;
#else /* DOS_FILE_PATH */
    if (strcasecmp(invoked_basename, "ebunzip") == 0
	|| strcasecmp(invoked_basename, "ebunzip.exe") == 0) {
	action_mode = EBZIP_ACTION_UNZIP;
    } else if (strcasecmp(invoked_basename, "ebzipinfo") == 0
	|| strcasecmp(invoked_basename, "ebzipinfo.exe") == 0) {
	action_mode = EBZIP_ACTION_INFO;
    }
#endif /* DOS_FILE_PATH */

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
    eb_initialize(&book);

    /*
     * Set fakelog behavior.
     */
    set_fakelog_name(invoked_name);
    set_fakelog_mode(FAKELOG_TO_STDERR);
    set_fakelog_level(LOG_ERR);

    /*
     * Parse command line options.
     */
    for (;;) {
        ch = getopt_long(argc, argv, short_options, long_options, NULL);
        if (ch == EOF)
            break;
        switch (ch) {
        case 'c':
            /*
             * Option `-c'.  Set case of filenames.
             */
	    if (strcasecmp(optarg, "upper") == 0)
		filename_case = EB_CASE_UPPER;
	    else if (strcasecmp(optarg, "lower") == 0)
		filename_case = EB_CASE_LOWER;
	    else {
		fprintf(stderr, "%s: unknown filename case `%s'\n",
		    invoked_name, optarg);
		exit(1);
	    }
	    break;

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
	     * The length of the filename
	     *    "<out_path>/subdir/subsubdir/file.ebz;1"
	     * must not exceed PATH_MAX.
             */
            if (PATH_MAX < strlen(optarg)) {
                fprintf(stderr, "%s: too long output directory path\n",
                    invoked_name);
                exit(1);
            }
            strcpy(out_path, optarg);
	    eb_canonicalize_filename(out_path);
	    if (PATH_MAX < strlen(out_path) + (1 + EB_MAXLEN_BASENAME + 1
		+ EB_MAXLEN_BASENAME + 1 + EB_MAXLEN_BASENAME + 6)) {
		fprintf(stderr, "%s: too long output directory path\n",
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

        case 's':
            /*
             * Option `-s'.  Set suffix of filenames.
             */
	    if (strcasecmp(optarg, "none") == 0)
		filename_suffix = EB_SUFFIX_NONE;
	    else if (strcasecmp(optarg, "dot") == 0)
		filename_suffix = EB_SUFFIX_DOT;
	    else if (strcasecmp(optarg, "version") == 0)
		filename_suffix = EB_SUFFIX_VERSION;
	    else if (strcasecmp(optarg, "both") == 0)
		filename_suffix = EB_SUFFIX_BOTH;
	    else {
		fprintf(stderr, "%s: unknown filename suffix `%s'\n",
		    invoked_name, optarg);
		exit(1);
	    }
	    break;

        case 'S':
            /*
             * Option `-S'.  Specify target subbooks.
             */
            if (parse_subname_argument(optarg) < 0)
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
        fprintf(stderr, "%s: too many arguments\n", invoked_name);
        output_try_help();
	goto die;
    }

    /*
     * Set a book path.
     */
    if (argc == optind)
        book_path = DEFAULT_BOOK_DIRECTORY;
    else
        book_path = argv[optind];

    /*
     * Bind a book.
     */
    if (eb_bind(&book, book_path) < 0) {
        fprintf(stderr, "%s: %s\n", invoked_name, eb_error_message());
	goto die;
    }

    /*
     * For each targe subbook, convert a subbook-names to a subbook-codes.
     * If no subbook is specified by `--subbook'(`-S'), set all subbooks
     * as the target.
     */
    if (subname_count == 0) {
	sub_count = eb_subbook_list(&book, sub_list);
	if (sub_count < 0) {
	    fprintf(stderr, "%s: %s\n", invoked_name, eb_error_message());
	    eb_clear(&book);
	}
    } else {
	for (i = 0; i < subname_count; i++) {
	    sub = find_subbook(&book, subname_list[i]);
	    if (sub < 0) {
		fprintf(stderr, "%s: unknown subbook `%s'\n", invoked_name,
		    subname_list[i]);
	    }
	    sub_list[sub_count++] = sub;
	}
    }

    /*
     * Fix case and suffix types of output filenames.
     *
     * When action mode is ZIP, modify suffix-type.
     * Since the suffix `*.ebz' is added to output filenames, we don't
     * add a DOT to the output filenames.  (If added, the filenames
     * come to `*.ebz.' or `*.ebz.;1'.)
     */
    if (filename_case < 0)
	filename_case = book.case_code;
    if (filename_suffix < 0)
	filename_suffix = book.suffix_code;

    /*
     * Compress the book.
     */
    switch (action_mode) {
    case EBZIP_ACTION_ZIP:
	if (zip_book(&book, out_path) < 0)
	    goto die;
	break;
    case EBZIP_ACTION_UNZIP:
	if (unzip_book(&book, out_path) < 0)
	    goto die;
	exit(1);
	break;
    case EBZIP_ACTION_INFO:
	if (zipinfo_book(&book) < 0)
	    goto die;
	break;
    }

    return 0;

    /*
     * A critical error occurs...
     */
  die:
    eb_clear(&book);
    exit(1);
}


/*
 * Parse an argument to option `--subbook (-S)'.
 * If the argument is valid form, 0 is returned.
 * Otherwise -1 is returned.
 */
static int
parse_subname_argument(argument)
    const char *argument;
{
    const char *arg = argument;
    char subname[EB_MAXLEN_BASENAME + 1], *name;
    char (*list)[EB_MAXLEN_BASENAME + 1];
    int i;

    while (*arg != '\0') {
	/*
	 * Check current `subname_count'.
	 */
	if (EB_MAX_SUBBOOKS <= subname_count) {
	    fprintf(stderr, "%s: too many subbooks\n", invoked_name);
	    fflush(stderr);
	    return -1;
	}

	/*
	 * Take a next element in the argument.
	 */
	for (i = 0, name = subname;
	     *arg != ',' && *arg != '\0' && i < EB_MAXLEN_BASENAME;
	      i++, name++, arg++) {
	    if (islower(*arg))
		*name = toupper(*arg);
	    else
		*name = *arg;
	}
	*name = '\0';
	if (*arg == ',')
	    arg++;
	else if (*arg != '\0') {
	    fprintf(stderr, "%s: invalid subbook name `%s...'\n",
		invoked_name, subname);
	    fflush(stderr);
	    return -1;
	}

	/*
	 * If the font name is not found in `font_list', it is added to
	 * the list.
	 */
	for (i = 0, list = subname_list; i < subname_count; i++, list++) {
	    if (strcmp(subname, *list) == 0)
		break;
	}
	if (subname_count <= i) {
	    strcpy(*list, subname);
	    subname_count++;
	}
    }

    return 0;
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
    char *endp;

    zip_level = (int)strtol(argument, &endp, 10);
    if (!isdigit(*argument) || *endp != '\0'
	|| zip_level < 0 || EB_MAX_ZIP_LEVEL < zip_level) {
	fprintf(stderr, "%s: invalid compression level `%s'\n",
	    invoked_name, argument);
	fflush(stderr);
	return -1;
    }

    return 0;
}


/*
 * Return a subbook-code of the subbook which contains the directory
 * name `dirname'.
 * When no sub-book is matched to `dirname', -1 is returned.
 */
static EB_Subbook_Code
find_subbook(book, directory)
    EB_Book *book;
    const char *directory;
{
    EB_Subbook_Code sublist[EB_MAX_SUBBOOKS];
    const char *directory2;
    int subcount;
    int i;

    /*
     * Find the subbook in the current book.
     */
    subcount = eb_subbook_list(book, sublist);
    for (i = 0; i < subcount; i++) {
        directory2 = eb_subbook_directory2(book, sublist[i]);
        if (directory2 == NULL || strcasecmp(directory, directory2) == 0)
            return sublist[i];
    }

    return -1;
}


/*
 * Output help message to stdandard out.
 */
static void
output_help()
{
    printf("Usage: %s [option...] [book-directory]\n", program_name);
    printf("Options:\n");
    printf("  -c CASE  --case CASE       output files which have filenames with CASE\n");
    printf("                             letters; upper or lower\n");
    printf("                             (default: depend on `catalog(s)(.ebz)')\n");
    printf("  -f  --force-overwrite      force overwrite of output files\n");
    printf("  -h  --help                 display this help, then exit\n");
    printf("  -i  --information          list information of compressed files\n");
    printf("  -k  --keep                 keep (don't delete) original files\n");
    printf("  -l INTEGER  --level INTEGER\n");
    printf("                             compression level; 0..%d\n",
	EB_MAX_ZIP_LEVEL);
    printf("                             (default: %d)\n",
	DEFAULT_ZIP_LEVEL);
    printf("  -n  --no-overwrite         don't overwrite output files\n");
    printf("  -o DIRECTORY  --output-directory DIRECTORY\n");
    printf("                             ouput compressed files under the directory\n");
    printf("                             (default: %s)\n",
	DEFAULT_OUTPUT_DIRECTORY);
    printf("  -q  --quiet  --silence     suppress all warnings\n");
    printf("  -s SUFFIX  --suffix SUFFIX\n");
    printf("                             output files which have filenames with SUFFIX;\n");
    printf("                             none, dot, version or both\n");
    printf("                             (default: depend on `catalog(s)(.ebz)')\n");
    printf("  -S SUBBOOK[,SUBBOOK...]  --subbook SUBBOOK[,SUBBOOK...]\n");
    printf("                             specify target subbook\n");
    printf("                             (default: all subbooks)\n");
    printf("  -t  --test                 only check for input files\n");
    printf("  -u  --uncompress           uncompress files\n");
    printf("  -v  --version              display version number, then exit\n");
    printf("  -z  --compress             compress files\n");
    printf("\nArgument:\n");
    printf("  book-directory             top directory of a CD-ROM book\n");
    printf("                             (default: %s)\n",
	DEFAULT_BOOK_DIRECTORY);
    printf("\nDefault action:\n");
    printf("  When invoked as `ebunzip', uncompression is the default action.\n");
    printf("  When invoked as `ebzipinfo', listing information is the default action.\n");
    printf("  Otherwise, compression is the default action.\n");
    printf("\nReport bugs to %s.\n", MAILING_ADDRESS);
    fflush(stdout);
}


/*
 * Output ``try ...'' message to standard error.
 */
static void
output_try_help()
{
    fprintf(stderr, "try `%s --help' for more information\n", invoked_name);
    fflush(stderr);
}


/*
 * Output version number to stdandard out.
 */
static void
output_version()
{
    printf("%s (EB Library) version %s\n", program_name, program_version);
    printf("Copyright (c) 1998  Motoyuki Kasahara\n\n");
    printf("This is free software; you can redistribute it and/or modify\n");
    printf("it under the terms of the GNU General Public License as published by\n");
    printf("the Free Software Foundation; either version 2, or (at your option)\n");
    printf("any later version.\n\n");
    printf("This program is distributed in the hope that it will be useful,\n");
    printf("but WITHOUT ANY WARRANTY; without even the implied warranty\n");
    printf("of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf("GNU General Public License for more details.\n");
    fflush(stdout);
}


/*
 * Compress files in `book' and output them under `out_path'.
 * If succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
zip_book(book, out_path)
    EB_Book *book;
    const char *out_path;
{
    EB_Subbook *sub;
    EB_Font *font;
    char in_filename[PATH_MAX + 1];
    char out_filename[PATH_MAX + 1];
    char in_directory[PATH_MAX + 1];
    char out_directory[PATH_MAX + 1];
    size_t out_path_length;
    mode_t out_dirmode;
    int i, j;

    out_path_length = strlen(out_path);
    out_dirmode = 0777 ^ get_umask();

    for (i = 0; i < sub_count; i++) {
	sub = book->subbooks + sub_list[i];

	/*
	 * Make directories for start/honmon files.
	 */
	sprintf(in_directory, "%s/%s", book->path, sub->directory);
	sprintf(out_directory, "%s/%s", out_path, sub->directory);
	fix_filename(in_directory + book->path_length, book->case_code,
	    EB_SUFFIX_NONE);
	fix_filename(out_directory + out_path_length, filename_case,
	    EB_SUFFIX_NONE);
	if (!test_flag
	    && make_missing_directory(out_directory, out_dirmode) < 0)
	    return -1;

	if (book->disc_code == EB_DISC_EPWING) {
	    sprintf(in_directory, "%s/%s/%s", book->path, sub->directory,
		EB_DIRNAME_DATA);
	    sprintf(out_directory, "%s/%s/%s", out_path, sub->directory,
		EB_DIRNAME_DATA);
	    fix_filename(in_directory + book->path_length, book->case_code,
		EB_SUFFIX_NONE);
	    fix_filename(out_directory + out_path_length, filename_case,
		EB_SUFFIX_NONE);
	    if (!test_flag 
		&& make_missing_directory(out_directory, out_dirmode) < 0)
		return -1;
	}

	/*
	 * Compress a start/honmon file.
	 */
	if (book->disc_code == EB_DISC_EB) {
	    sprintf(in_filename, "%s/%s", in_directory, EB_FILENAME_START);
	    sprintf(out_filename, "%s/%s.EBZ", out_directory,
		EB_FILENAME_START);
	} else {
	    sprintf(in_filename, "%s/%s", in_directory, EB_FILENAME_HONMON);
	    sprintf(out_filename, "%s/%s.EBZ", out_directory,
		EB_FILENAME_HONMON);
	}
	fix_filename(in_filename + book->path_length, book->case_code,
	    book->suffix_code);
	fix_filename(out_filename + out_path_length, filename_case,
	    filename_suffix);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(in_filename);
	eb_canonicalize_filename(out_filename);
#endif
	zip_file(in_filename, out_filename);

	if (book->disc_code == EB_DISC_EPWING) {
	    int is_same;

	    /*
	     * Make a directory for font files.
	     */
	    sprintf(in_directory, "%s/%s/%s", book->path, sub->directory,
		EB_DIRNAME_GAIJI);
	    sprintf(out_directory, "%s/%s/%s", out_path, sub->directory,
		EB_DIRNAME_GAIJI);
	    fix_filename(in_directory + book->path_length, book->case_code,
		EB_SUFFIX_NONE);
	    fix_filename(out_directory + out_path_length, filename_case,
		EB_SUFFIX_NONE);
#ifdef DOS_FILE_PATH
	    eb_canonicalize_filename(in_filename);
	    eb_canonicalize_filename(out_filename);
#endif
	    if (!test_flag
		&& make_missing_directory(out_directory, out_dirmode) < 0)
		return -1;

	    is_same = is_same_file(in_directory, out_directory);

	    for (j = 0; j < sub->font_count; j++) {
		/*
		 * Compress a font file.
		 */
		font = sub->fonts + j;
		sprintf(in_filename, "%s/%s", in_directory, font->filename);
		sprintf(out_filename, "%s/%s.EBZ", out_directory,
		    font->filename);
		fix_filename(in_filename + book->path_length, book->case_code,
		    book->suffix_code);
		fix_filename(out_filename + out_path_length, filename_case,
		    filename_suffix);
#ifdef DOS_FILE_PATH
		eb_canonicalize_filename(in_filename);
		eb_canonicalize_filename(out_filename);
#endif
		zip_file(in_filename, out_filename);
	    }
	}
    }

    /*
     * Compress a language file.
     */
    if (book->disc_code == EB_DISC_EB) {
	sprintf(in_filename, "%s/%s", book->path, EB_FILENAME_LANGUAGE);
	sprintf(out_filename, "%s/%s.EBZ", out_path, EB_FILENAME_LANGUAGE);
	fix_filename(in_filename + book->path_length, book->case_code,
	    book->suffix_code);
	fix_filename(out_filename + out_path_length, filename_case,
	    filename_suffix);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(in_filename);
	eb_canonicalize_filename(out_filename);
#endif
	zip_file(in_filename, out_filename);
    }

    /*
     * Copy a catalog/catalogs file.
     */
    if (book->disc_code == EB_DISC_EB) {
	sprintf(in_filename, "%s/%s", book->path, EB_FILENAME_CATALOG);
	sprintf(out_filename, "%s/%s", out_path, EB_FILENAME_CATALOG);
    } else {
	sprintf(in_filename, "%s/%s", book->path, EB_FILENAME_CATALOGS);
	sprintf(out_filename, "%s/%s", out_path, EB_FILENAME_CATALOGS);
    }
    fix_filename(in_filename + book->path_length, book->case_code,
	book->suffix_code);
    fix_filename(out_filename + out_path_length, filename_case,
	filename_suffix);
#ifdef DOS_FILE_PATH
    eb_canonicalize_filename(in_filename);
    eb_canonicalize_filename(out_filename);
#endif
    if (!is_same_file(in_directory, out_directory)
	|| book->case_code != filename_case
	|| book->suffix_code != filename_suffix)
	copy_file(in_filename, out_filename);

    return 0;
}


/*
 * Uncompress files in `book' and output them under `out_path'.
 * If succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
unzip_book(book, out_path)
    EB_Book *book;
    const char *out_path;
{
    EB_Subbook *sub;
    EB_Font *font;
    char in_filename[PATH_MAX + 1];
    char out_filename[PATH_MAX + 1];
    char in_directory[PATH_MAX + 1];
    char out_directory[PATH_MAX + 1];
    size_t out_path_length;
    mode_t out_dirmode;
    int i, j;

    out_path_length = strlen(out_path);
    out_dirmode = 0777 ^ get_umask();

    for (i = 0; i < sub_count; i++) {
	sub = book->subbooks + sub_list[i];

	/*
	 * Make directories for start/honmon files.
	 */
	sprintf(in_directory, "%s/%s", book->path, sub->directory);
	sprintf(out_directory, "%s/%s", out_path, sub->directory);
	fix_filename(in_directory + book->path_length, book->case_code,
	    EB_SUFFIX_NONE);
	fix_filename(out_directory + out_path_length, filename_case,
	    EB_SUFFIX_NONE);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(in_filename);
	eb_canonicalize_filename(out_filename);
#endif
	if (!test_flag
	    && make_missing_directory(out_directory, out_dirmode) < 0)
	    return -1;

	if (book->disc_code == EB_DISC_EPWING) {
	    sprintf(in_directory, "%s/%s/%s", book->path, sub->directory,
		EB_DIRNAME_DATA);
	    sprintf(out_directory, "%s/%s/%s", out_path, sub->directory,
		EB_DIRNAME_DATA);
	    fix_filename(in_directory + book->path_length, book->case_code,
		EB_SUFFIX_NONE);
	    fix_filename(out_directory + out_path_length, filename_case,
		EB_SUFFIX_NONE);
#ifdef DOS_FILE_PATH
	    eb_canonicalize_filename(in_filename);
	    eb_canonicalize_filename(out_filename);
#endif
	    if (!test_flag
		&& make_missing_directory(out_directory, out_dirmode) < 0)
		return -1;
	}

	/*
	 * Uncompress a start/honmon file.
	 */
	if (book->disc_code == EB_DISC_EB) {
	    sprintf(in_filename, "%s/%s.EBZ", in_directory, EB_FILENAME_START);
	    sprintf(out_filename, "%s/%s", out_directory, EB_FILENAME_START);
	} else {
	    sprintf(in_filename, "%s/%s.EBZ", in_directory,
		EB_FILENAME_HONMON);
	    sprintf(out_filename, "%s/%s", out_directory, EB_FILENAME_HONMON);
	}
	fix_filename(in_filename + book->path_length, book->case_code,
	    book->suffix_code);
	fix_filename(out_filename + out_path_length, filename_case,
	    filename_suffix);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(in_filename);
	eb_canonicalize_filename(out_filename);
#endif
	unzip_file(in_filename, out_filename);

	if (book->disc_code == EB_DISC_EPWING) {
	    int is_same;

	    /*
	     * Make a directory for font files.
	     */
	    sprintf(in_directory, "%s/%s/%s", book->path, sub->directory,
		EB_DIRNAME_GAIJI);
	    sprintf(out_directory, "%s/%s/%s", out_path, sub->directory,
		EB_DIRNAME_GAIJI);
	    fix_filename(in_directory + book->path_length, book->case_code,
		EB_SUFFIX_NONE);
	    fix_filename(out_directory + out_path_length, filename_case,
		EB_SUFFIX_NONE);
#ifdef DOS_FILE_PATH
	    eb_canonicalize_filename(in_filename);
	    eb_canonicalize_filename(out_filename);
#endif
	    if (!test_flag
		&& make_missing_directory(out_directory, out_dirmode) < 0)
		return -1;

	    is_same = is_same_file(in_directory, out_directory);

	    for (j = 0; j < sub->font_count; j++) {
		/*
		 * Uncompress a font file.
		 */
		font = sub->fonts + j;
		sprintf(in_filename, "%s/%s.EBZ", in_directory,
		    font->filename);
		sprintf(out_filename, "%s/%s", out_directory, font->filename);
		fix_filename(in_filename + book->path_length, book->case_code,
		    book->suffix_code);
		fix_filename(out_filename + out_path_length, filename_case,
		    filename_suffix);
#ifdef DOS_FILE_PATH
		eb_canonicalize_filename(in_filename);
		eb_canonicalize_filename(out_filename);
#endif
		unzip_file(in_filename, out_filename);
	    }
	}
    }

    /*
     * Uncompress a language file.
     */
    if (book->disc_code == EB_DISC_EB) {
	sprintf(in_filename, "%s/%s.EBZ", book->path, EB_FILENAME_LANGUAGE);
	sprintf(out_filename, "%s/%s", out_path, EB_FILENAME_LANGUAGE);
	fix_filename(in_filename + book->path_length, book->case_code,
	    book->suffix_code);
	fix_filename(out_filename + out_path_length, filename_case,
	    filename_suffix);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(in_filename);
	eb_canonicalize_filename(out_filename);
#endif
	unzip_file(in_filename, out_filename);
    }

    /*
     * Copy a catalog/catalogs file.
     */
    if (book->disc_code == EB_DISC_EB) {
	sprintf(in_filename, "%s/%s", book->path, EB_FILENAME_CATALOG);
	sprintf(out_filename, "%s/%s", out_path, EB_FILENAME_CATALOG);
    } else {
	sprintf(in_filename, "%s/%s", book->path, EB_FILENAME_CATALOGS);
	sprintf(out_filename, "%s/%s", out_path, EB_FILENAME_CATALOGS);
    }
    fix_filename(in_filename + book->path_length, book->case_code,
	book->suffix_code);
    fix_filename(out_filename + out_path_length, filename_case,
	filename_suffix);
#ifdef DOS_FILE_PATH
    eb_canonicalize_filename(in_filename);
    eb_canonicalize_filename(out_filename);
#endif
    if (!is_same_file(in_directory, out_directory)
	|| book->case_code != filename_case
	|| book->suffix_code != filename_suffix)
	copy_file(in_filename, out_filename);

    return 0;
}


/*
 * List compressed book information.
 * If succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
zipinfo_book(book)
    EB_Book *book;
{
    EB_Subbook *sub;
    EB_Font *font;
    char orig_filename[PATH_MAX + 1];
    char ebz_filename[PATH_MAX + 1];
    int i, j;

    for (i = 0; i < sub_count; i++) {
	sub = book->subbooks + sub_list[i];

	/*
	 * Inspect `start' and `honmon' files.
	 */
	if (book->disc_code == EB_DISC_EB) {
	    sprintf(orig_filename, "%s/%s/%s", book->path, sub->directory,
		EB_FILENAME_START);
	} else {
	    sprintf(orig_filename, "%s/%s/%s/%s", book->path,
		sub->directory, EB_DIRNAME_DATA, EB_FILENAME_HONMON);
	}
	strcpy(ebz_filename, orig_filename);
	strcat(ebz_filename, ".EBZ");
	fix_filename(orig_filename + book->path_length, book->case_code,
	    book->suffix_code);
	fix_filename(ebz_filename + book->path_length, book->case_code,
	    book->suffix_code);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(in_filename);
	eb_canonicalize_filename(out_filename);
#endif
	zipinfo_file(ebz_filename, orig_filename);

	/*
	 * Inspect font files.
	 */
	if (book->disc_code == EB_DISC_EPWING) {
	    for (j = 0; j < sub->font_count; j++) {
		font = sub->fonts + j;
		sprintf(orig_filename, "%s/%s/%s/%s", book->path,
		    sub->directory, EB_DIRNAME_GAIJI, font->filename);
		strcpy(ebz_filename, orig_filename);
		strcat(ebz_filename, ".EBZ");
		fix_filename(orig_filename + book->path_length,
		    book->case_code, book->suffix_code);
		fix_filename(ebz_filename + book->path_length,
		    book->case_code, book->suffix_code);
#ifdef DOS_FILE_PATH
		eb_canonicalize_filename(in_filename);
		eb_canonicalize_filename(out_filename);
#endif
		zipinfo_file(ebz_filename, orig_filename);
	    }
	}
    }

    /*
     * Inspect language files.
     */
    if (book->disc_code == EB_DISC_EB) {
	sprintf(orig_filename, "%s/%s", book->path, EB_FILENAME_LANGUAGE);
	strcpy(ebz_filename, orig_filename);
	strcat(ebz_filename, ".EBZ");
	fix_filename(orig_filename + book->path_length, book->case_code,
	    book->suffix_code);
	fix_filename(ebz_filename + book->path_length, book->case_code,
	    book->suffix_code);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(in_filename);
	eb_canonicalize_filename(out_filename);
#endif
	zipinfo_file(ebz_filename, orig_filename);
    }

    /*
     * Inspect a catalog/catalogs file.
     */
    if (book->disc_code == EB_DISC_EB)
	sprintf(orig_filename, "%s/%s", book->path, EB_FILENAME_CATALOG);
    else
	sprintf(orig_filename, "%s/%s", book->path, EB_FILENAME_CATALOGS);
    fix_filename(orig_filename + book->path_length, book->case_code,
	book->suffix_code);
    fix_filename(ebz_filename + book->path_length, book->case_code,
	book->suffix_code);
#ifdef DOS_FILE_PATH
    eb_canonicalize_filename(in_filename);
    eb_canonicalize_filename(out_filename);
#endif
    zipinfo_file(ebz_filename, orig_filename);

    return 0;
}


/*
 * Adjust case and suffix of a filename.
 *
 * `filename' is a filename to be adjusted.
 * Upon return, `filename' are converted to upper or lower case,
 * and add a suffix according to `fcase' and 'fsuffix'.
 */
static void
fix_filename(filename, fcase, fsuffix)
    char *filename;
    EB_Case_Code fcase;
    EB_Suffix_Code fsuffix;
{
    char *p;
    char *rslash;
    int have_dot;

    /*
     * Check whether `filename' contains `.' in its filename, or not.
     */
    rslash = strrchr(filename, '/');
    if (rslash == NULL)
	rslash = filename;
    have_dot = (strchr(rslash, '.') != NULL);

    /*
     * Add a suffix.
     */
    if (!have_dot && (fsuffix == EB_SUFFIX_DOT || fsuffix == EB_SUFFIX_BOTH))
	strcat(filename, ".");
    if (fsuffix == EB_SUFFIX_VERSION || fsuffix == EB_SUFFIX_BOTH)
	strcat(filename, ";1");

    /*
     * Convert characters in the filename to upper or lower.
     */
    if (fcase == EB_CASE_UPPER) {
	for (p = filename; *p != '\0'; p++) {
	    if (islower(*p))
		*p = toupper(*p);
	}
    } else {
	for (p = filename; *p != '\0'; p++) {
	    if (isupper(*p))
		*p = tolower(*p);
	}
    }
}


/*
 * Compress the file `in_filename'.
 * Comressed data are written to the file `out_filename'.
 * If succeeded, 0 is returned.  Otherwise -1 is returned.
 */
static int
zip_file(in_filename, out_filename)
    const char *out_filename;
    const char *in_filename;
{
    EB_Zip zip;
    unsigned char *in_buffer = NULL, *out_buffer = NULL;
    size_t in_total_length, out_total_length;
    int in_file = -1, out_file = -1;
    ssize_t in_length;
    size_t out_length;
    struct stat in_st, out_st;
    off_t zip_location, next_location;
    size_t index_length;
    int status_interval;
    int slices;
    int i;

    /*
     * Output information.
     */
    if (!quiet_flag) {
	printf("==> compress %s <==\noutput to %s\n",
	    in_filename,  out_filename);
	fflush(stdout);
    }

    /*
     * Check whether input file exists.
     */
    if (stat(in_filename, &in_st) != 0) {
	fprintf(stderr, "%s: %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }
    if (!S_ISREG(in_st.st_mode)) {
	fprintf(stderr, "%s: not a regular file, %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }

    /*
     * Initialize `zip'.
     */
    zip.code = EB_ZIP_MODE1;
    zip.slice_size = EB_SIZE_PAGE << zip_level;
    zip.file_size = in_st.st_size;
    zip.crc = 1;
    zip.mtime = in_st.st_mtime;

    if (zip.file_size < 1 << 16)
	zip.index_width = 2;
    else if (zip.file_size < 1 << 24)
	zip.index_width = 3;
    else 
	zip.index_width = 4;

    /*
     * Allocate memories for in/out buffers.
     */
    in_buffer = (unsigned char *)malloc(zip.slice_size);
    if (in_buffer == NULL) {
	fprintf(stderr, "%s: memory exhausted\n", invoked_name);
	goto failed;
    }

    out_buffer = (unsigned char *)malloc(zip.slice_size + EB_SIZE_ZIP_MARGIN);
    if (out_buffer == NULL) {
	fprintf(stderr, "%s: memory exhausted\n", invoked_name);
	goto failed;
    }
    
    /*
     * If the file `out_filename' already exists, confirm and unlink it.
     */
    if (!test_flag
	&& stat(out_filename, &out_st) == 0 && S_ISREG(out_st.st_mode)) {
	if (overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!quiet_flag) {
		fputs("already exists, skip the file\n\n", stderr);
		fflush(stderr);
	    }
	    return 0;
	} else if (overwrite_mode == EBZIP_OVERWRITE_QUERY) {
	    int y_or_n;

	    fprintf(stderr, "\nthe file already exists: %s\n", out_filename);
	    y_or_n = query_y_or_n("do you wish to overwrite (y or n)? ");
	    fputc('\n', stderr);
	    fflush(stderr);
	    if (!y_or_n)
		return 0;
        }
	if (unlink(out_filename) < 0) {
	    fprintf(stderr, "%s: cannot unlink the file: %s\n", invoked_name,
		out_filename);
	    goto failed;
	}
    }

    /*
     * Open files.
     */
    in_file = open(in_filename, O_RDONLY | O_BINARY);
    if (in_file < 0) {
	fprintf(stderr, "%s: cannot open the file, %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }

    if (!test_flag) {
	trap_filename = out_filename;
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
	out_file = open(out_filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	    0666 ^ get_umask());
#else
	out_file = creat(out_filename, 0666 ^ get_umask());
#endif
	if (out_file < 0) {
	    fprintf(stderr, "%s: cannot open the file, %s: %s\n",
		invoked_name, strerror(errno), out_filename);
	    goto failed;
	}
	trap_file = out_file;
    }

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
     *     slices = N = (file_size + slice_size - 1) / slice_size
     *     index_length = (N + 1) * index_width
     */
    slices = (zip.file_size + zip.slice_size - 1) / zip.slice_size;
    index_length = (slices + 1) * zip.index_width;
    memset(out_buffer, '\0', zip.slice_size);

    if (!test_flag) {
	for (i = index_length + EB_SIZE_ZIP_HEADER;
	     zip.slice_size <= i; i -= zip.slice_size) {
	    if (write(out_file, out_buffer, zip.slice_size)
		!= zip.slice_size) {
		fprintf(stderr, "%s: cannot write to the file: %s\n",
		    invoked_name, out_filename);
		goto failed;
	    }
	}
	if (0 < i) {
	    if (write(out_file, out_buffer, i) != i) {
		fprintf(stderr, "%s: cannot write to the file: %s\n",
		    invoked_name, out_filename);
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
    status_interval = STATUS_INTERVAL_FACTOR >> zip_level;
    for (i = 0; i < slices; i++) {
	/*
	 * Read a slice from the original file.
	 */
	in_length = read(in_file, in_buffer, zip.slice_size);
	if (in_length < 0) {
	    fprintf(stderr, "%s: cannot read from the file, %s: %s\n",
		invoked_name, strerror(errno), in_filename);
	    goto failed;
	} else if (in_length == 0) {
	    fprintf(stderr, "%s: unexpected EOF: %s\n", 
		invoked_name, in_filename);
	    goto failed;
	} else if (in_length != zip.slice_size
	    && in_total_length + in_length != zip.file_size) {
	    fprintf(stderr, "%s: unexpected EOF: %s\n", 
		invoked_name, in_filename);
	    goto failed;
	}

	/*
	 * Update CRC.  (Calculate adler32 again.)
	 */
	zip.crc = adler32((uLong)zip.crc, (Bytef *)in_buffer, (uInt)in_length);

	/*
	 * If this is last slice and its length is shorter than
	 * `slice_size', fill `\0'.
	 */
	if (in_length < zip.slice_size) {
	    memset(in_buffer + in_length, '\0', zip.slice_size - in_length);
	    in_length = zip.slice_size;
	}

	/*
	 * Compress the slice.
	 */
	if (zip_mode1((char *)out_buffer, &out_length, (char *)in_buffer,
	    zip.slice_size) < 0) {
	    fprintf(stderr, "%s: memory exhausted\n", invoked_name);
	    goto failed;
	}
	if (zip.slice_size <= out_length) {
	    memcpy(out_buffer, in_buffer, zip.slice_size);
	    out_length = zip.slice_size;
	}

	/*
	 * Write the slice to the zip file.
	 * If the length of the zipped slice is not shorter than
	 * original, write orignal slice.
	 */
	if (!test_flag) {
	    zip_location = lseek(out_file, 0, SEEK_END);
	    if (zip_location < 0) {
		fprintf(stderr, "%s: cannot seek the file, %s: %s\n",
		    invoked_name, strerror(errno), out_filename);
		goto failed;
	    }
	    if (write(out_file, out_buffer, out_length) != out_length) {
		fprintf(stderr, "%s: cannot write to the file: %s\n",
		    invoked_name, out_filename);
		goto failed;
	    }
	}

	/*
	 * Write an index for the slice.
	 */
	next_location = zip_location + out_length;
	switch (zip.index_width) {
	case 2:
	    out_buffer[0] = (zip_location >> 8) & 0xff;
	    out_buffer[1] = zip_location & 0xff;
	    out_buffer[2] = (next_location >> 8) & 0xff;
	    out_buffer[3] = next_location & 0xff;
	    break;
	case 3:
	    out_buffer[0] = (zip_location >> 16) & 0xff;
	    out_buffer[1] = (zip_location >> 8) & 0xff;
	    out_buffer[2] = zip_location & 0xff;
	    out_buffer[3] = (next_location >> 16) & 0xff;
	    out_buffer[4] = (next_location >> 8) & 0xff;
	    out_buffer[5] = next_location & 0xff;
	    break;
	case 4:
	    out_buffer[0] = (zip_location >> 24) & 0xff;
	    out_buffer[1] = (zip_location >> 16) & 0xff;
	    out_buffer[2] = (zip_location >> 8) & 0xff;
	    out_buffer[3] = zip_location & 0xff;
	    out_buffer[4] = (next_location >> 24) & 0xff;
	    out_buffer[5] = (next_location >> 16) & 0xff;
	    out_buffer[6] = (next_location >> 8) & 0xff;
	    out_buffer[7] = next_location & 0xff;
	    break;
	}

	if (!test_flag) {
	    if (lseek(out_file, EB_SIZE_ZIP_HEADER + i * zip.index_width,
		SEEK_SET) < 0) {
		fprintf(stderr, "%s: cannot seek the file, %s: %s\n",
		    invoked_name, strerror(errno), out_filename);
		goto failed;
	    }
	    if (write(out_file, out_buffer, zip.index_width * 2)
		!= zip.index_width * 2) {
		fprintf(stderr, "%s: cannot write to the file, %s: %s\n",
		    invoked_name, strerror(errno), out_filename);
		goto failed;
	    }
	}

	in_total_length += in_length;
	out_total_length += out_length + zip.index_width;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!quiet_flag
	    && i % status_interval + 1 == status_interval) {
	    printf("%4.1f%% done: %lu -> %lu bytes (%4.1f%%)\n",
		(double)(i + 1) * 100.0 / (double)slices,
		(unsigned long)in_total_length,
		(unsigned long)out_total_length,
		(double)out_total_length * 100.0 / (double)in_total_length);
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
    out_buffer[ 5] = (EB_ZIP_MODE1 << 4) + (zip_level & 0x0f);
    out_buffer[ 6] = 0;
    out_buffer[ 7] = 0;
    out_buffer[ 8] = 0;
    out_buffer[ 9] = 0;
    out_buffer[10] = (zip.file_size >> 24) & 0xff;
    out_buffer[11] = (zip.file_size >> 16) & 0xff;
    out_buffer[12] = (zip.file_size >> 8) & 0xff;
    out_buffer[13] = zip.file_size & 0xff;
    out_buffer[14] = (zip.crc >> 24) & 0xff;
    out_buffer[15] = (zip.crc >> 16) & 0xff;
    out_buffer[16] = (zip.crc >> 8) & 0xff;
    out_buffer[17] = zip.crc & 0xff;
    out_buffer[18] = (zip.mtime >> 24) & 0xff;
    out_buffer[19] = (zip.mtime >> 16) & 0xff;
    out_buffer[20] = (zip.mtime >> 8) & 0xff;
    out_buffer[21] = zip.mtime & 0xff;

    if (!test_flag) {
	if (lseek(out_file, 0, SEEK_SET) < 0) {
	    fprintf(stderr, "%s: cannot seek the file, %s: %s\n",
		invoked_name, strerror(errno), out_filename);
	    goto failed;
	}
	if (write(out_file, out_buffer, EB_SIZE_ZIP_HEADER)
	    != EB_SIZE_ZIP_HEADER) {
	    fprintf(stderr, "%s: cannot write to the file, %s: %s\n",
		invoked_name, strerror(errno), out_filename);
	    goto failed;
	}
    }

    /*
     * Output the result information unless quiet mode.
     */
    out_total_length += EB_SIZE_ZIP_HEADER + zip.index_width;
    if (!quiet_flag) {
	printf("total: %lu -> %lu bytes ",
	    (unsigned long)in_total_length, (unsigned long)out_total_length);
	if (in_total_length == 0)
	    fputs("(empty input file)\n\n", stdout);
	else {
	    printf("(%4.1f%%)\n\n", (double)out_total_length * 100.0
		/ (double)in_total_length);
	}
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
	trap_filename = NULL;
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
    if (!test_flag  && !keep_flag && unlink(in_filename) < 0) {
	fprintf(stderr, "%s: cannot unlink the file: %s\n", invoked_name,
	    in_filename);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and utime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
    if (!test_flag) {
	struct utimbuf utim;

#if defined(HAVE_CHOWN)
	chown(out_filename, in_st.st_uid, in_st.st_gid);
#endif
#if defined(HAVE_CHMOD)
	chmod(out_filename, in_st.st_mode);
#endif
#if defined(HAVE_UTIME)
	utim.actime = in_st.st_atime;
	utim.modtime = in_st.st_mtime;
	utime(out_filename, &utim);
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
	trap_filename = NULL;
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
 * Uncompress the file `in_filename'.
 * Comressed data are written to the file `out_filename'.
 * If succeeded, 0 is returned.  Otherwise -1 is returned.
 */
static int
unzip_file(in_filename, out_filename)
    const char *out_filename;
    const char *in_filename;
{
    EB_Zip zip;
    unsigned char *in_buffer = NULL, *out_buffer = NULL;
    size_t in_total_length, out_total_length;
    int in_file = -1, out_file = -1;
    size_t in_length, out_length;
    struct stat in_st, out_st;
    off_t zip_location, next_location;
    unsigned int crc = 1;
    int status_interval;
    int slices;
    int i;

    /*
     * Output filename information.
     */
    if (!quiet_flag) {
	printf("==> uncompress %s <==\noutput to %s\n",
	    in_filename, out_filename);
	fflush(stdout);
    }

    /*
     * Check whether the input file exists.
     */
    if (stat(in_filename, &in_st) != 0) {
	fprintf(stderr, "%s: %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }
    if (!S_ISREG(in_st.st_mode)) {
	fprintf(stderr, "%s: not a regular file, %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }

    /*
     * Allocate memories for in/out buffers.
     */
    in_buffer = (unsigned char *)malloc(EB_SIZE_PAGE << EB_MAX_ZIP_LEVEL);
    if (in_buffer == NULL) {
	fprintf(stderr, "%s: memory exhausted\n", invoked_name);
	goto failed;
    }

    out_buffer = (unsigned char *)malloc(EB_SIZE_PAGE << EB_MAX_ZIP_LEVEL);
    if (out_buffer == NULL) {
	fprintf(stderr, "%s: memory exhausted\n", invoked_name);
	goto failed;
    }
    
    /*
     * If the file `out_filename' already exists, confirm and unlink it.
     */
    if (!test_flag
	&& stat(out_filename, &out_st) == 0 && S_ISREG(out_st.st_mode)) {
	if (overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!quiet_flag) {
		fputs("already exists, skip the file\n\n", stderr);
		fflush(stderr);
	    }
	    return 0;
	} else if (overwrite_mode == EBZIP_OVERWRITE_QUERY) {
	    int y_or_n;

	    fprintf(stderr, "\nthe file already exists: %s\n", out_filename);
	    y_or_n = query_y_or_n("do you wish to overwrite (y or n)? ");
	    fputc('\n', stderr);
	    fflush(stderr);
	    if (!y_or_n)
		return 0;
        }
	if (unlink(out_filename) < 0) {
	    fprintf(stderr, "%s: cannot unlink the file: %s\n", invoked_name,
		out_filename);
	    goto failed;
	}
    }

    /*
     * Open files.
     */
    in_file = open(in_filename, O_RDONLY | O_BINARY);
    if (in_file < 0) {
	fprintf(stderr, "%s: cannot open the file, %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }
    if (!test_flag) {
	trap_filename = out_filename;
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
	out_file = open(out_filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	    0666 ^ get_umask());
#else
	out_file = creat(out_filename, 0666 ^ get_umask());
#endif
	if (out_file < 0) {
	    fprintf(stderr, "%s: cannot open the file, %s: %s\n",
		invoked_name, strerror(errno), out_filename);
	    goto failed;
	}
	trap_file = out_file;
    }

    /*
     * Read header part of the zipped file.
     */
    if (read(in_file, in_buffer, EB_SIZE_ZIP_HEADER) != EB_SIZE_ZIP_HEADER) {
	fprintf(stderr, "%s: cannot read from the file, %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }
    zip.code = eb_uint1(in_buffer + 5) >> 4;
    zip.slice_size = EB_SIZE_PAGE << (eb_uint1(in_buffer + 5) & 0x0f);
    zip.file_size = eb_uint4(in_buffer + 10);
    zip.crc = eb_uint4(in_buffer + 14);
    zip.mtime = eb_uint4(in_buffer + 18);

    if (zip.file_size < 1 << 16)
	zip.index_width = 2;
    else if (zip.file_size < 1 << 24)
	zip.index_width = 3;
    else
	zip.index_width = 4;

    /*
     * Check contents of the zip header.
     */
    if (memcmp(in_buffer, "EBZip", 5) != 0
	|| zip.code != EB_ZIP_MODE1
	|| zip.index_width <= 0
	|| EB_SIZE_PAGE << EB_MAX_ZIP_LEVEL < zip.slice_size) {
	fprintf(stderr, "%s: not ebzip format: %s\n", invoked_name,
	    in_filename);
        goto failed;
    }

    /*
     * Read a slice from the input file, uncompress it if required,
     * and then write it to the output file.
     */
    in_total_length = 0;
    out_total_length = 0;
    slices = (zip.file_size + zip.slice_size - 1) / zip.slice_size;
    status_interval = STATUS_INTERVAL_FACTOR >> zip_level;
    for (i = 0; i < slices; i++) {
	/*
	 * Get location and size of the slice from index table in `file'.
	 */
	if (lseek(in_file, i * zip.index_width + EB_SIZE_ZIP_HEADER,
	    SEEK_SET) < 0) {
	    fprintf(stderr, "%s: cannot seek the file, %s: %s\n",
		invoked_name, strerror(errno), in_filename);
	    goto failed;
	}
	if (read(in_file, in_buffer, zip.index_width * 2)
	    != zip.index_width * 2) {
	    fprintf(stderr, "%s: cannot read the file, %s: %s\n",
		invoked_name, strerror(errno), in_filename);
	    goto failed;
	}

	switch (zip.index_width) {
	case 2:
	    zip_location = eb_uint2(in_buffer);
	    next_location = eb_uint2(in_buffer + 2);
	    break;
	case 3:
	    zip_location = eb_uint3(in_buffer);
	    next_location = eb_uint3(in_buffer + 3);
	    break;
	case 4:
	    zip_location = eb_uint4(in_buffer);
	    next_location = eb_uint4(in_buffer + 4);
	    break;
	default:
	    fprintf(stderr, "%s: inconsistent ebzip data: %s\n",
		invoked_name, in_filename);
	    goto failed;
	}
	in_length = next_location - zip_location;

	if (next_location <= zip_location || zip.slice_size < in_length) {
	    fprintf(stderr, "%s: inconsistent ebzip data: %s\n",
		invoked_name, in_filename);
	    goto failed;
	}

	/*
	 * Read the slice from `file' and unzip it, if it is zipped.
	 * We assumes the slice is not compressed if its length is
	 * equal to `slice_size'.
	 */
	if (lseek(in_file, zip_location, SEEK_SET) < 0) {
	    fprintf(stderr, "%s: cannot seek the file, %s: %s\n",
		invoked_name, strerror(errno), in_filename);
	    goto failed;
	}
	if (read(in_file, in_buffer, in_length) != in_length) {
	    fprintf(stderr, "%s: cannot read the file, %s: %s\n",
		invoked_name, strerror(errno), in_filename);
	    goto failed;
	}
	if (in_length == zip.slice_size)
	    memcpy(out_buffer, in_buffer, in_length);
	else if (eb_unzip_mode1((char *)out_buffer, zip.slice_size,
	    (char *)in_buffer, in_length) < 0) {
	    fprintf(stderr, "%s: inconsistent ebzip data: %s\n",
		invoked_name, in_filename);
	    goto failed;
	}

	/*
	 * Calcurate the length of the data to be output.
	 * If this is the last block, NULs may be padded in the tail
	 * of the block data.
	 */
	if (out_total_length + zip.slice_size <= zip.file_size)
	    out_length = zip.slice_size;
	else
	    out_length = zip.file_size % zip.slice_size;

	/*
	 * Update CRC.  (Calculate adler32 again.)
	 */
	crc = adler32((uLong)crc, (Bytef *)out_buffer, (uInt)out_length);

	/*
	 * Write the slice to `out_file'.
	 */
	if (!test_flag) {
	    if (write(out_file, out_buffer, out_length) != out_length) {
		fprintf(stderr, "%s: cannot write to the file, %s: %s\n",
		    invoked_name, strerror(errno), out_filename);
		goto failed;
	    }
	}

	in_total_length += in_length + zip.index_width;
	out_total_length += out_length;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!quiet_flag
	    && i % status_interval + 1 == status_interval) {
	    printf("%4.1f%% done: %lu -> %lu bytes\n",
		(double)(i + 1) * 100.0 / (double)slices,
		(unsigned long)in_total_length,
		(unsigned long)out_total_length);
	    fflush(stdout);
	}
    }

    /*
     * Output the result unless quiet mode.
     */
    if (!quiet_flag) {
	printf("total: %lu -> %lu bytes\n\n",
	    (unsigned long)in_st.st_size, (unsigned long)out_total_length);
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
	trap_filename = NULL;
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
     * Check CRC.
     */
    if (crc != zip.crc) {
	fprintf(stderr, "%s: CRC error: %s\n", invoked_name, out_filename);
	goto failed;
    }

    /*
     * Delete an original file unless `keep_flag' is set.
     */
    if (!test_flag  && !keep_flag && unlink(in_filename) < 0) {
	fprintf(stderr, "%s: cannot unlink the file: %s\n", invoked_name,
	    in_filename);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and mtime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
    if (!test_flag) {
	struct utimbuf utim;

#if defined(HAVE_UTIME)
	utim.actime = in_st.st_atime;
	utim.modtime = in_st.st_mtime;
	utime(out_filename, &utim);
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
	trap_filename = NULL;
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
 * Output status of the compressed file `ebz_filename'.
 * If succeeded, 0 is returned.  Otherwise -1 is returned.
 */
static int
zipinfo_file(ebz_filename, orig_filename)
    const char *ebz_filename;
    const char *orig_filename;
{
    char buffer[EB_SIZE_ZIP_HEADER];
    int file = -1;
    off_t orig_file_size;
    struct stat ebz_st, orig_st;
    int zip_level;

    /*
     * Check whether the file `orig_filename' exists or not.
     */
    if (stat(orig_filename, &orig_st) == 0 && S_ISREG(orig_st.st_mode)) {
	printf("==> %s <==\n", orig_filename);
	printf("not compressed: %lu bytes\n\n",
	    (unsigned long)orig_st.st_size);
	fflush(stdout);
	return 0;
    }

    /*
     * Output filename information.
     */
    if (!quiet_flag) {
	printf("==> %s <==\n", ebz_filename);
	fflush(stdout);
    }

    /*
     * Check whether the file `ebz_filename' exists or not.
     */
    if (stat(ebz_filename, &ebz_st) != 0) {
	fprintf(stderr, "%s: not found the file\n", ebz_filename);
	goto failed;
    }

    /*
     * Check whether the file `ebz_filename' is regular file.
     */
    if (!S_ISREG(ebz_st.st_mode)) {
	fprintf(stderr, "%s: not a regular file, %s: %s\n",
	    invoked_name, strerror(errno), ebz_filename);
	goto failed;
    }

    /*
     * Open the file.
     */
    file = open(ebz_filename, O_RDONLY | O_BINARY);
    if (file < 0) {
	fprintf(stderr, "%s: cannot open the file, %s: %s\n",
	    invoked_name, strerror(errno), ebz_filename);
	goto failed;
    }

    /*
     * Read data from the file.
     */
    if (read(file, buffer, EB_SIZE_ZIP_HEADER) != EB_SIZE_ZIP_HEADER) {
	fprintf(stderr, "%s: cannot read the file, %s: %s\n",
	    invoked_name, strerror(errno), ebz_filename);
	goto failed;
    }
    orig_file_size = eb_uint4(buffer + 10);
    zip_level = eb_uint1(buffer + 5) & 0x0f;

    /*
     * Close the file.
     */
    close(file);

    /*
     * Output the result unless quiet mode.
     */
    if (!quiet_flag) {
	printf("compressed: %lu -> %lu bytes ",
	    (unsigned long)orig_file_size, (unsigned long)ebz_st.st_size);
	if (orig_file_size == 0)
	    fputs("(empty original file)\n", stdout);
	else {
	    printf("(%4.1f%%)\n", (double)ebz_st.st_size * 100.0
		/ (double)orig_file_size);
	}
	printf("compresssion level: %d\n\n", zip_level);
	fflush(stdout);
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file)
	close(file);

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}


/*
 * Copy a file.
 * If succeeded, 0 is returned.  Otherwise -1 is returned.
 */
static int
copy_file(in_filename, out_filename)
    const char *out_filename;
    const char *in_filename;
{
    unsigned char buffer[EB_SIZE_PAGE];
    size_t total_length;
    struct stat in_st, out_st;
    int in_file = -1, out_file = -1;
    size_t in_length;
    int status_interval;
    int slices;
    int i;

    /*
     * Output filename information.
     */
    if (!quiet_flag) {
	printf("==> copy %s <==\noutput to %s\n", in_filename, out_filename);
	fflush(stdout);
    }

    /*
     * Check whether the file `in_filename' exists or not.
     */
    if (stat(in_filename, &in_st) != 0) {
	fprintf(stderr, "%s: not found the file\n", in_filename);
	goto failed;
    }

    /*
     * Check whether the file `in_filename' is regular file.
     */
    if (!S_ISREG(in_st.st_mode)) {
	fprintf(stderr, "%s: not a regular file, %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
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
	&& stat(out_filename, &out_st) == 0 && S_ISREG(out_st.st_mode)) {
	if (overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!quiet_flag) {
		fputs("already exists, skip the file\n\n", stderr);
		fflush(stderr);
	    }
	    return 0;
	}
	if (overwrite_mode == EBZIP_OVERWRITE_QUERY) {
	    int y_or_n;

	    fprintf(stderr, "\nthe file already exists: %s\n", out_filename);
	    y_or_n = query_y_or_n("do you wish to overwrite (y or n)? ");
	    fputc('\n', stderr);
	    fflush(stderr);
	    if (!y_or_n)
		return 0;
        }
	if (unlink(out_filename) < 0) {
	    fprintf(stderr, "%s: cannot unlink the file: %s\n", invoked_name,
		out_filename);
	    goto failed;
	}
    }

    /*
     * Open files.
     */
    in_file = open(in_filename, O_RDONLY | O_BINARY);
    if (in_file < 0) {
	fprintf(stderr, "%s: cannot open the file, %s: %s\n",
	    invoked_name, strerror(errno), in_filename);
	goto failed;
    }
    if (!test_flag) {
	trap_filename = out_filename;
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
	out_file = open(out_filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	    0666 ^ get_umask());
#else
	out_file = creat(out_filename, 0666 ^ get_umask());
#endif
	if (out_file < 0) {
	    fprintf(stderr, "%s: cannot open the file, %s: %s\n",
		invoked_name, strerror(errno), out_filename);
	    goto failed;
	}
	trap_file = out_file;
    }

    /*
     * Read data from the input file, compress the data, and then
     * write them to the output file.
     */
    total_length = 0;
    slices = (in_st.st_size + EB_SIZE_PAGE - 1) / EB_SIZE_PAGE;
    status_interval = STATUS_INTERVAL_FACTOR;
    for (i = 0; i < slices; i++) {
	/*
	 * Read data from `in_file', and write them to `out_file'.
	 */
	in_length = read(in_file, buffer, EB_SIZE_PAGE);
	if (in_length < 0) {
	    fprintf(stderr, "%s: cannot read from the file, %s: %s\n",
		invoked_name, strerror(errno), in_filename);
	    goto failed;
	} else if (in_length == 0) {
	    fprintf(stderr, "%s: unexpected EOF: %s\n", 
		invoked_name, in_filename);
	    goto failed;
	} else if (in_length != EB_SIZE_PAGE
	    && total_length + in_length != in_st.st_size) {
	    fprintf(stderr, "%s: unexpected EOF: %s\n", 
		invoked_name, in_filename);
	    goto failed;
	}

	/*
	 * Write decoded data to `out_file'.
	 */
	if (!test_flag) {
	    if (write(out_file, buffer, in_length) != in_length) {
		fprintf(stderr, "%s: cannot write to the file, %s: %s\n",
		    invoked_name, strerror(errno), out_filename);
		goto failed;
	    }
	}

	total_length += in_length;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!quiet_flag
	    && i % status_interval + 1 == status_interval) {
	    printf("%4.1f%% done: %lu bytes\n", (double)(i + 1) * 100.0
		/ (double)slices, (unsigned long)total_length);
	    fflush(stdout);
	}
    }

    /*
     * Output the result unless quiet mode.
     */
    if (!quiet_flag) {
	printf("total: %lu bytes\n\n", (unsigned long)total_length);
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
	trap_filename = NULL;
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
    if (!test_flag  && !keep_flag && unlink(in_filename) < 0) {
	fprintf(stderr, "%s: cannot unlink the file: %s\n", invoked_name,
	    in_filename);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and mtime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
    if (!test_flag) {
	struct utimbuf utim;

#if defined(HAVE_UTIME)
	utim.actime = in_st.st_atime;
	utim.modtime = in_st.st_mtime;
	utime(out_filename, &utim);
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
trap(sig)
    int sig;
{
    if (0 <= trap_file)
	close(trap_file);
    if (trap_filename != NULL)
	unlink(trap_filename);
    
    exit(1);

    /* not reached */
#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}
