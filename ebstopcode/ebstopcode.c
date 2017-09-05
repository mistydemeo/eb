/*                                                            -*- C -*-
 * Copyright (c) 2003
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
#include <sys/types.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>
#endif

#ifndef HAVE_STRTOL
#ifdef __STDC__
long strtol(const char *, char **, int);
#else /* not __STDC__ */
long strtol();
#endif /* not __STDC__ */
#endif /* not HAVE_STRTOL */

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
#include "eb/text.h"
#include "eb/appendix.h"

#include "getopt.h"
#include "ebutils.h"

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

/*
 * Character type tests and conversions.
 */
#define isdigit(c) ('0' <= (c) && (c) <= '9')
#define isxdigit(c) \
 (isdigit(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))

/*
 * Default maximum length of text.
 */
#define DEFAULT_MAX_TEXT_LENGTH		EB_SIZE_PAGE

/*
 * Default book directory.
 */
#define DEFAULT_BOOK_DIRECTORY		"."

/*
 * Dummy stop code.
 *
 * Application cannot order EB Library not to use stop code.
 * EB Library guesses stop code automatically when appendix is not
 * given by application.
 * 
 * Instead, we use dummy stop code. The code is never used in text
 * of CD-ROM book.
 */
#define DUMMY_STOP_CODE0		0x1f00
#define DUMMY_STOP_CODE1		0x0000

/*
 * Command line options.
 */
static const char *short_options = "c:hl:nv";
static struct option long_options[] = {
    {"code",              required_argument, NULL, 'c'},
    {"help",              no_argument,       NULL, 'h'},
    {"text-length",       required_argument, NULL, 'l'},
    {"--no-candidates",   no_argument,       NULL, 'n'},
    {"version",           no_argument,       NULL, 'v'},
    {NULL, 0, NULL, 0}
};

/*
 * Program name and version.
 */
static const char *program_name = "ebstopcode";
static const char *program_version = VERSION;
static const char *invoked_name;

/*
 * Unexported functions.
 */
static int parse_stop_code_argument EB_P((const char *, unsigned int *,
    unsigned int *));
static int parse_text_length_argument EB_P((const char *, ssize_t *));
static void output_help EB_P((void));
static int scan_subbook_text EB_P((const char *, const char *, ssize_t,
    int, unsigned int, unsigned int));
static EB_Error_Code hook_stop_code EB_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));


int
main(argc, argv)
    int argc;
    char *argv[];
{
    const char *book_directory;
    ssize_t max_text_length;
    const char *subbook_name;
    int show_stop_code_flag;
    unsigned int stop_code0, stop_code1;
    int ch;

    invoked_name = argv[0];
    max_text_length = DEFAULT_MAX_TEXT_LENGTH;
    show_stop_code_flag = 1;
    stop_code0 = DUMMY_STOP_CODE0;
    stop_code1 = DUMMY_STOP_CODE1;

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
     * Parse command line options.
     */
    for (;;) {
	ch = getopt_long(argc, argv, short_options, long_options, NULL);
	if (ch == -1)
	    break;

	switch (ch) {
        case 'c':
            /*
             * Option `-c'.  Specify stop code manually.
             */
	    if (parse_stop_code_argument(optarg, &stop_code0, &stop_code1) < 0)
		goto die;
	    max_text_length = 0;
            break;

	case 'h':
	    /*
	     * Option `-h'.  Display help message, then exit.
	     */
	    output_help();
	    exit(0);

        case 'l':
            /*
             * Option `-l'.  Specify maximum length of text.
             */
	    if (parse_text_length_argument(optarg, &max_text_length) < 0)
		goto die;
	    stop_code0 = DUMMY_STOP_CODE0;
	    stop_code1 = DUMMY_STOP_CODE1;
            break;

	case 'n':
	    /*
	     * Option `-n'.  Do not output stop code candidates.
	     */
	    show_stop_code_flag = 0;
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
    if (argc - optind < 2) {
	fprintf(stderr, _("%s: too few argument\n"), invoked_name);
	output_try_help(invoked_name);
	goto die;
    }
    if (2 < argc - optind) {
	fprintf(stderr, _("%s: too many arguments\n"), invoked_name);
	output_try_help(invoked_name);
	goto die;
    }
    if (argc - optind == 2) {
	book_directory = argv[optind];
	subbook_name = argv[optind + 1];
    } else {
	book_directory = DEFAULT_BOOK_DIRECTORY;
	subbook_name = argv[optind];
    }

    /*
     * Scan stop code in text.
     */
    if (scan_subbook_text(book_directory, subbook_name, max_text_length,
	show_stop_code_flag, stop_code0, stop_code1) < 0)
	goto die;

    return 0;

  die:
    fflush(stdout);
    exit(1);
}


/*
 * Parse stop code given as argument of the `-c' option.
 */
static int
parse_stop_code_argument(argument, stop_code0, stop_code1)
    const char *argument;
    unsigned int *stop_code0;
    unsigned int *stop_code1;
{
    const char *p = argument;
    const char *foundp;

    /*
     * Parse stop_code0.
     */
    while (*p == ' ' || *p == '\t')
	p++;
    if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X'))
	p += 2;

    foundp = p;
    while (isxdigit(*p))
	p++;

    if (p == foundp || (*p != ' ' && *p != '\t'))
	goto failed;
    *stop_code0 = strtol(foundp, NULL, 16);
    if (*stop_code0 != 0x1f09 && *stop_code0 != 0x1f41)
	goto failed;

    /*
     * Parse stop_code1.
     */
    while (*p == ' ' || *p == '\t')
	p++;
    if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X'))
	p += 2;

    foundp = p;
    while (isxdigit(*p))
	p++;

    if (p == foundp || (*p != ' ' && *p != '\t' && *p != '\0'))
	goto failed;
    *stop_code1 = strtol(foundp, NULL, 16);
    if (0xffff < *stop_code1)
	goto failed;

    while (*p == ' ' || *p == '\t')
	p++;
    if (*p != '\0')
	goto failed;

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fprintf(stderr, _("%s: invalid stop code `%s'\n"), invoked_name,
	argument);
    return -1;
}


/*
 * Parse stop code given as argument of the `-t' option.
 */
static int
parse_text_length_argument(argument, text_length)
    const char *argument;
    ssize_t *text_length;
{
    const char *p = argument;
    const char *foundp;

    while (*p == ' ' || *p == '\t')
	p++;

    foundp = p;
    while (isdigit(*p))
	p++;
    if (p == foundp)
	goto failed;
    while (*p == ' ' || *p == '\t')
	p++;
    if (*p != '\0')
	goto failed;

    *text_length = strtol(foundp, NULL, 10);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    fprintf(stderr, _("%s: invalid text length `%s'\n"), invoked_name,
	argument);
    return -1;
}


/*
 * Output help message to standard out, then exit.
 */
static void
output_help()
{
    printf(_("Usage: %s [option...] [book-directory] subbook\n"),
	program_name);
    printf(_("Options:\n"));
    printf(_("  -c CODE, --code CODE\n"));
    printf(_("                             set stop code manually\n"));
    printf(_("  -h  --help                 display this help, then exit\n"));
    printf(_("  -l LENGTH, --text-length LENGTH\n"));
    printf(_("                             maximum length of output text\n"));
    printf(_("                             (default: %d)\n"),
	DEFAULT_MAX_TEXT_LENGTH);
    printf(_("  -n  --no-candidates        suppress stop code candidates\n"));
    printf(_("  -v  --version              display version number, then exit\n"));
    printf(_("\nArgument:\n"));
    printf(_("  book-directory             top directory of a CD-ROM book\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_BOOK_DIRECTORY);
    printf(_("\nReport bugs to %s.\n"), MAILING_ADDRESS);
    fflush(stdout);
}


/*
 * Scan stop code in text.
 */
static int
scan_subbook_text(book_directory, subbook_name, max_text_length,
    show_stop_code_flag, stop_code0, stop_code1)
    const char *book_directory;
    const char *subbook_name;
    ssize_t max_text_length;
    int show_stop_code_flag;
    unsigned int stop_code0;
    unsigned int stop_code1;
{
    EB_Error_Code error_code;
    EB_Book book;
    EB_Hookset hookset;
    EB_Hook hook;
    EB_Appendix appendix;
    EB_Appendix_Subbook appendix_subbook;
    EB_Subbook_Code subbook_code;
    EB_Position text_position;
    char text[EB_SIZE_PAGE];
    ssize_t text_length;

    /* 
     * Initialize EB Library, `book', `appendix' and `hookset'.
     */
    eb_initialize_library();
    eb_initialize_book(&book);
    eb_initialize_appendix(&appendix);
    eb_initialize_hookset(&hookset);

    /*
     * Bind `book'.
     */
    error_code = eb_bind(&book, book_directory);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, _("%s: failed to bind the book, %s: %s\n"),
            program_name, eb_error_message(error_code), book_directory);
        goto die;
    }

    /*
     * Get a subbook code from the subbook name.
     */
    error_code = find_subbook(&book, subbook_name, &subbook_code);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: %s: %s\n",
            program_name, eb_error_message(error_code), subbook_name);
        goto die;
    }

    /*
     * Set current subbook.
     */
    if (eb_set_subbook(&book, subbook_code) < 0) {
        fprintf(stderr, _("%s: failed to set the current subbook, %s\n"),
            program_name, eb_error_message(error_code));
        goto die;
    }

    /*
     * Set stop-code manually.
     * (we hack `appendix' directly.)
     */
    appendix.code            = subbook_code;
    appendix.subbook_current = &appendix_subbook;
    appendix_subbook.code       = subbook_code;
    appendix_subbook.stop_code0 = stop_code0;
    appendix_subbook.stop_code1 = stop_code1;

    /*
     * Set text hooks (for 0x1f41 and 0x1f09).
     */
    if (show_stop_code_flag) {
	hook.code = EB_HOOK_BEGIN_KEYWORD;
	hook.function = hook_stop_code;
	eb_set_hook(&hookset, &hook);

	hook.code = EB_HOOK_SET_INDENT;
	hook.function = hook_stop_code;
	eb_set_hook(&hookset, &hook);
    }

    /*
     * Get a position where the text data starts.
     */
    error_code = eb_text(&book, &text_position);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, _("%s: failed to get text information, %s\n"),
            program_name, eb_error_message(error_code));
        goto die;
    }

    /*
     * Read text.
     */
    error_code = eb_seek_text(&book, &text_position);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: %s\n",
	    program_name, eb_error_message(error_code));
        goto die;
    }

    text_length = 0;
    while (max_text_length == 0 || text_length < max_text_length) {
	ssize_t result_length;
	size_t read_length;

	if (max_text_length == 0
	    || sizeof(text) - 1 <= max_text_length - text_length) {
	    read_length = sizeof(text) - 1;
	} else {
	    read_length = max_text_length - text_length;
	}
	error_code = eb_read_text(&book, &appendix, &hookset, NULL,
	    read_length, text, &result_length);
	if (error_code != EB_SUCCESS) {
            fprintf(stderr, _("%s: failed to read text, %s\n"),
                program_name, eb_error_message(error_code));
            goto die;
        }
	if (result_length <= 0)
	    break;
	fputs(text, stdout);
	text_length += result_length;
    }
        
    /*
     * Finalize `hookset', `appendix', `book' and EB Library.
     */
    eb_finalize_hookset(&hookset);
    eb_finalize_appendix(&appendix);
    eb_finalize_book(&book);
    eb_finalize_library();
    return 0;

    /*
     * An error occurs...
     */
  die:
    eb_finalize_hookset(&hookset);
    eb_finalize_appendix(&appendix);
    eb_finalize_book(&book);
    eb_finalize_library();
    return -1;
}


/*
 * Hook function for EB_HOOK_BEGIN_KEYWORD and EB_HOK_SET_INDENT.
 */
static EB_Error_Code
hook_stop_code(book, appendix, container, code, argc, argv)
    EB_Book *book;
    EB_Appendix *appendix;
    void *container;
    EB_Hook_Code code;
    int argc;
    const unsigned int *argv;
{
    char string[EB_SIZE_PAGE];

    if (0 < book->text_context.printable_count) {
	sprintf(string, _("\n=== stop-code?: 0x%04x 0x%04x ===\n"),
	    argv[0], argv[1]);
	eb_write_text_string(book, string);
    }

    return EB_SUCCESS;
}
