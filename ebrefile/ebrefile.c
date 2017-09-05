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

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>
#endif

#include "ebutils.h"

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

#define DEFAULT_BOOK_DIRECTORY		"."
#define DEFAULT_OUTPUT_DIRECTORY	"."

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
 * Unexported functions.
 */
static void output_help EB_P((void));
static int refile_book EB_P((const char *, const char *, 
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));
static int refile_catalog EB_P((const char *, const char *, EB_Disc_Code,
    char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1], int));
static RETSIGTYPE trap EB_P((int));
static int find_subbook_name EB_P((char [][EB_MAX_DIRECTORY_NAME_LENGTH + 1],
    int, const char *));

/*
 * Command line options.
 */
static const char *short_options = "ho:S:v";
static struct option long_options[] = {
    {"help",              no_argument,       NULL, 'h'},
    {"output-directory",  required_argument, NULL, 'o'},
    {"subbook",           required_argument, NULL, 'S'},
    {"version",           no_argument,       NULL, 'v'},
    {NULL, 0, NULL, 0}
};

/*
 * Program name and version.
 */
const char *program_name = "ebrefile";
const char *program_version = VERSION;
const char *invoked_name;

/*
 * File name to be deleted and file to be closed when signal is received.
 */
static const char *trap_file_name = NULL;
static int trap_file = -1;

int
main(argc, argv)
    int argc;
    char *argv[];
{
    char out_path[PATH_MAX + 1];
    char book_path[PATH_MAX + 1];
    char subbook_name_list[EB_MAX_SUBBOOKS][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_name_count = 0;
    int ch;

    invoked_name = argv[0];
    strcpy(out_path, DEFAULT_OUTPUT_DIRECTORY);

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
     * Initialize `book'.
     */
    eb_initialize_library();

    /*
     * Parse command line options.
     */
    for (;;) {
        ch = getopt_long(argc, argv, short_options, long_options, NULL);
        if (ch == EOF)
            break;
        switch (ch) {
        case 'h':
            /*
             * Option `-h'.  Display help message, then exit.
             */
            output_help();
            exit(0);

        case 'o':
            /*
             * Option `-o'.  Output files under DIRECOTRY.
	     * The length of the file name
	     *    "<out_path>/catalogs.bak;1"
	     * must not exceed PATH_MAX.
             */
            if (PATH_MAX < strlen(optarg)) {
                fprintf(stderr, _("%s: too long output directory path\n"),
                    invoked_name);
                exit(1);
            }
            strcpy(out_path, optarg);
	    canonicalize_path(out_path);
	    if (PATH_MAX
		< strlen(out_path) + (1 + EB_MAX_DIRECTORY_NAME_LENGTH + 6)) {
		fprintf(stderr, _("%s: too long output directory path\n"),
		    invoked_name);
		goto die;
	    }
	    break;

        case 'S':
            /*
             * Option `-S'.  Specify target subbooks.
             */
            if (parse_subbook_name_argument(invoked_name, optarg,
		subbook_name_list, &subbook_name_count) < 0)
                exit(1);
            break;

        case 'v':
            /*
             * Option `-v'.  Display version number, then exit.
             */
            output_version(program_name, program_version);
            exit(0);

        default:
            output_try_help(invoked_name);
	    goto die;
	}
    }

    /*
     * Check the number of rest arguments.
     */
    if (1 < argc - optind) {
        fprintf(stderr, _("%s: too many arguments\n"), invoked_name);
        output_try_help(invoked_name);
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

    if (PATH_MAX
	< strlen(book_path) + (1 + EB_MAX_DIRECTORY_NAME_LENGTH + 6)) {
	fprintf(stderr, _("%s: too long book directory path\n"),
	    invoked_name);
	goto die;
    }

    /*
     * Refile a catalog.
     */
    refile_book(book_path, out_path, subbook_name_list, subbook_name_count);

    eb_finalize_library();

    return 0;

    /*
     * A critical error occurs...
     */
  die:
    eb_finalize_library();
    exit(1);
}


/*
 * Output help message to stdandard out.
 */
static void
output_help()
{
    printf(_("Usage: %s [option...] [book-directory]\n"), program_name);
    printf(_("Options:\n"));
    printf(_("  -h  --help                 display this help, then exit\n"));
    printf(_("  -o DIRECTORY  --output-directory DIRECTORY\n"));
    printf(_("                             ouput files under DIRECTORY\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_OUTPUT_DIRECTORY);
    printf(_("  -S SUBBOOK[,SUBBOOK...]  --subbook SUBBOOK[,SUBBOOK...]\n"));
    printf(_("                             target subbook\n"));
    printf(_("                             (default: all subbooks)\n"));
    printf(_("  -v  --version              display version number, then exit\n"));
    printf(_("\nArgument:\n"));
    printf(_("  book-directory             top directory of a CD-ROM book\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_BOOK_DIRECTORY);

    printf(_("\nReport bugs to %s.\n"), MAILING_ADDRESS);
    fflush(stdout);
}


/*
 * Hints of catalog file name in book.
 */
#define HINT_INDEX_CATALOG           0
#define HINT_INDEX_CATALOGS          1

static const char *catalog_hint_list[] = {
    "catalog", "catalogs", NULL
};

static int
refile_book(out_path, in_path, subbook_name_list, subbook_name_count)
    const char *out_path;
    const char *in_path;
    char subbook_name_list[][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_name_count;
{
    char in_path_name[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];
    char in_base_name[EB_MAX_FILE_NAME_LENGTH + 1];
    EB_Disc_Code disc_code;
    int hint_index;

    /*
     * Find a catalog file.
     */
    eb_find_file_name(in_path, catalog_hint_list, in_base_name,
        &hint_index);

    switch (hint_index) {
    case HINT_INDEX_CATALOG:
	disc_code = EB_DISC_EB;
        break;

    case HINT_INDEX_CATALOGS:
	disc_code = EB_DISC_EPWING;
        break;

    default:
	fprintf(stderr, _("%s: no catalog file: %s\n"), invoked_name, in_path);
        return -1;
    }

    /*
     * Set input and output file name. 
     */
    eb_compose_path_name(in_path, in_base_name, in_path_name);
    eb_compose_path_name(out_path, in_base_name, out_path_name);
    eb_fix_path_name_suffix(out_path_name, ".new");

    refile_catalog(out_path_name, in_path_name, disc_code, 
	subbook_name_list, subbook_name_count);

    return 0;
}


static int
refile_catalog(out_catalog_name, in_catalog_name, disc_code,
    subbook_name_list, subbook_name_count)
    const char *out_catalog_name;
    const char *in_catalog_name;
    EB_Disc_Code disc_code;
    char subbook_name_list[][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_name_count;
{
    char buffer[EB_SIZE_PAGE];
    char directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int in_subbook_count;
    int out_subbook_count;
    int in_file = -1;
    int out_file = -1;
    int subbook_name_index;
    int subbook_name_found_flags[EB_MAX_SUBBOOKS];
    off_t out_file_offset;
    size_t catalog_size;
    size_t title_size;
    size_t page_fragment;
    int i;

    if (disc_code == EB_DISC_EB) {
	catalog_size = EB_SIZE_EB_CATALOG;
	title_size = EB_MAX_EB_TITLE_LENGTH;
    } else {
	catalog_size = EB_SIZE_EPWING_CATALOG;
	title_size = EB_MAX_EPWING_TITLE_LENGTH;
    }

    /*
     * Open input file.
     */
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
    in_file = open(in_catalog_name, O_RDONLY | O_BINARY);
    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_catalog_name);
	goto failed;
    }

    /*
     * Open output file.
     */
    out_file_offset = 0;
#ifdef O_CREAT
    out_file = open(out_catalog_name, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	0666 ^ get_umask());
#else
    out_file = creat(out_catalog_name, 0666 ^ get_umask());
#endif
    if (out_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), out_catalog_name);
	goto failed;
    }

    trap_file_name = out_catalog_name;
    trap_file = out_file;

    /*
     * Read header.
     */
    if (read(in_file, buffer, 16) != 16) {
	fprintf(stderr, _("%s: failed to read the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_catalog_name);
        goto failed;
    }
    in_subbook_count = eb_uint2(buffer);

    /*
     * Write header.
     */
    if (write(out_file, buffer, 16) != 16) {
	fprintf(stderr, _("%s: failed to write the file, %s: %s\n"),
	    invoked_name, strerror(errno), out_catalog_name);
        goto failed;
    }
    out_file_offset += 16;

    for (i = 0; i < subbook_name_count; i++)
	subbook_name_found_flags[i] = 0;

    out_subbook_count = 0;

    for (i = 0; i < in_subbook_count; i++) {
	/*
	 * Read subbook entry.
	 */
	if (read(in_file, buffer, catalog_size) != catalog_size) {
	    fprintf(stderr, _("%s: failed to read the file, %s: %s\n"),
		invoked_name, strerror(errno), in_catalog_name);
	    goto failed;
	}

	/*
	 * Check whether `subbook_name_list' has a directory name of
	 * this subbook.
	 */
        strncpy(directory_name, buffer + 2 + title_size,
	    EB_MAX_DIRECTORY_NAME_LENGTH);
        directory_name[EB_MAX_DIRECTORY_NAME_LENGTH] = '\0';

	if (0 < subbook_name_count) {
	    subbook_name_index = find_subbook_name(subbook_name_list,
		subbook_name_count, directory_name);
	    if (subbook_name_index < 0)
		continue;
	    subbook_name_found_flags[subbook_name_index] = 1;
	}

	/*
	 * Write the subbook entry.
	 */
	if (write(out_file, buffer, catalog_size) != catalog_size) {
	    fprintf(stderr, _("%s: failed to write the file, %s: %s\n"),
		invoked_name, strerror(errno), out_catalog_name);
	    goto failed;
	}

	out_file_offset += catalog_size;
	out_subbook_count++;
    }

    /*
     * Check whether all subbooks in `subbook_name_list' are found.
     */
    for (i = 0; i < subbook_name_count; i++) {
	if (!subbook_name_found_flags[i]) {
	    fprintf(stderr, _("%s: warning: no such subbook: %s\n"),
		invoked_name, subbook_name_list[i]);
	}
    }

    /*
     * Copy rest of the file.
     */
    for (;;) {
	ssize_t read_length;

	read_length = read(in_file, buffer, EB_SIZE_PAGE);
	if (read_length == 0) {
	    break;
	} else if (read_length < 0) {
	    fprintf(stderr, _("%s: failed to read the file, %s: %s\n"),
		invoked_name, strerror(errno), in_catalog_name);
	    goto failed;
	}
	if (write(out_file, buffer, read_length) != read_length) {
	    fprintf(stderr, _("%s: failed to write the file, %s: %s\n"),
		invoked_name, strerror(errno), out_catalog_name);
	    goto failed;
	}
	out_file_offset += read_length;
    }

    /*
     * Fill the current page with 0.
     */
    page_fragment = out_file_offset % EB_SIZE_PAGE;
    if (0 < page_fragment) {
	memset(buffer, 0, EB_SIZE_PAGE);
	if (write(out_file, buffer, EB_SIZE_PAGE - page_fragment)
	    != EB_SIZE_PAGE - page_fragment) {
	    fprintf(stderr, _("%s: failed to write the file, %s: %s\n"),
		invoked_name, strerror(errno), out_catalog_name);
	    goto failed;
	}
    }

    /*
     * Fix the number of subbook.
     */
    buffer[0] = (out_subbook_count >> 8) & 0xff;
    buffer[1] =  out_subbook_count       & 0xff;
    if (lseek(out_file, 0, SEEK_SET) < 0) {
	fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
	    invoked_name, strerror(errno), out_catalog_name);
	goto failed;
    }
    if (write(out_file, buffer, 2) != 2) {
	fprintf(stderr, _("%s: failed to write the file, %s: %s\n"),
	    invoked_name, strerror(errno), out_catalog_name);
	goto failed;
    }

    /*
     * Close files.
     */
    close(in_file);
    in_file = -1;

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

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= in_file)
	close(in_file);
    if (0 <= out_file)
	close(out_file);

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

static int
find_subbook_name(subbook_name_list, subbook_name_count, pattern)
    char subbook_name_list[][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int subbook_name_count;
    const char *pattern;
{
    char canonicalized_pattern[EB_MAX_FILE_NAME_LENGTH + 1];
    char *space;
    int i;

    strcpy(canonicalized_pattern, pattern);
    space = strchr(canonicalized_pattern, ' ');
    if (space != NULL)
	*space = '\0';

    for (i = 0; i < subbook_name_count; i++) {
	if (strcasecmp(subbook_name_list[i], canonicalized_pattern) == 0)
	    return i;
    }

    return -1;
}
