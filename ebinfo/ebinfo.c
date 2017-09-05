/*
 * Copyright (c) 1997, 1998  Motoyuki Kasahara
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
#include <syslog.h>

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

#include "eb/eb.h"
#include "eb/error.h"
#include "eb/font.h"

#include "fakelog.h"
#include "getopt.h"

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

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
 * Unexported functions.
 */
static int output_information EB_P((const char *, int));
static void output_version EB_P((void));
static void output_help EB_P((void));
static void output_try_help EB_P((void));
static void output_multi_information EB_P((EB_Book *));

/*
 * Command line options.
 */
static const char *short_options = "hmv";
static struct option long_options[] = {
  {"help",         no_argument, NULL, 'h'},
  {"multi-search", no_argument, NULL, 'm'},
  {"version",      no_argument, NULL, 'v'},
  {NULL, 0, NULL, 0}
};

/*
 * Generic name of the program.
 */
const char *program_name = "ebinfo";

/*
 * Program version.
 */
const char *program_version = VERSION;

/*
 * Actual program name. (argv[0])
 */
const char *invoked_name;


/*
 * Default output directory
 */
#define DEFAULT_BOOK_DIRECTORY	"."

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int ch;
    char *bookdir;
    int multiflag;

    invoked_name = argv[0];

    /*
     * Set fakelog behavior.
     */
    set_fakelog_name(invoked_name);
    set_fakelog_mode(FAKELOG_TO_STDERR);
    set_fakelog_level(LOG_ERR);

    /*
     * Parse command line options.
     */
    multiflag = 0;
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

	case 'm':
	    /*
	     * Option `-m'.  Also output multi-search information.
	     */
	    multiflag = 1;
	    break;

	case 'v':
	    /*
	     * Option `-v'.  Display version number, then exit.
	     */
	    output_version();
	    exit(0);

	default:
	    output_try_help();
	    exit(1);
	}
    }

    /*
     * Check the number of rest arguments.
     */
    if (1 < argc - optind) {
	fprintf(stderr, "%s: too many arguments\n", invoked_name);
	output_try_help();
	exit(1);
    }

    /*
     * Output information about the book.
     */
    if (argc == optind)
	bookdir = DEFAULT_BOOK_DIRECTORY;
    else
	bookdir = argv[optind];

    if (output_information(bookdir, multiflag) < 0)
	exit(1);

    return 0;
}


/*
 * Output information about the book at `path'.
 * If `multiflag' is enabled, multi-search information are also output.
 */
static int
output_information(path, multiflag)
    const char *path;
    int multiflag;
{
    EB_Book book;
    EB_Disc_Code disc;
    EB_Character_Code char_code;
    EB_Subbook_Code sub_list[EB_MAX_SUBBOOKS];
    EB_Font_Code font_list[EB_MAX_FONTS];
    const char *str;
    int start, end;
    int sub_count;
    int font_count;
    int i, j;

    /*
     * Start to use a book.
     */
    eb_initialize(&book);
    if (eb_bind(&book, path) < 0)
	goto failed;

    /*
     * Output disc type.
     */
    disc = eb_disc_type(&book);
    if (disc < 0)
	goto failed;
    if (disc == EB_DISC_EB)
	printf("disc type: EB/EBG/EBXA\n");
    else
	printf("disc type: EPWING\n");

    /*
     * Output character code.
     */
    char_code = eb_character_code(&book);
    if (char_code < 0)
	goto failed;
    if (char_code == EB_CHARCODE_JISX0208)
	printf("character code: JIS X 0208\n");
    else 
	printf("character code: ISO 8859-1\n");

    /*
     * Output the number of subbooks in the book.
     */
    sub_count = eb_subbook_list(&book, sub_list);
    if (sub_count < 0)
	goto failed;
    printf("the number of subbooks: %d\n\n", sub_count);

    /*
     * Output information about each subbook.
     */
    for (i = 0; i < sub_count; i++) {
	printf("subbook %d:\n", i + 1);

	/*
	 * Output a title of the subbook.
	 */
	str = eb_subbook_title2(&book, sub_list[i]);
	if (str == NULL)
	    goto failed;
	printf("  title: %s\n", str);

	/*
	 * Output a directory name of the subbook.
	 */
	str = eb_subbook_directory2(&book, sub_list[i]);
	if (str == NULL)
	    goto failed;
	printf("  directory: %s\n", str);

	/*
	 * Set the current subbook to `i'.
	 */
	if (eb_set_subbook(&book, sub_list[i]) < 0) {
	    fprintf(stderr, "%s: %s\n\n", invoked_name, eb_error_message());
	    fflush(stderr);
	    continue;
	}

	/*
	 * Output supported methods.
	 */
	printf("  search methods: ");
	if (eb_have_word_search(&book))
	    fputs("word ", stdout);
	if (eb_have_endword_search(&book))
	    fputs("endword ", stdout);
	if (eb_have_keyword_search(&book))
	    fputs("keyword ", stdout);
	if (eb_have_multi_search(&book))
	    fputs("multi ", stdout);
	if (eb_have_graphic_search(&book))
	    fputs("graphic ", stdout);
	if (eb_have_menu(&book))
	    fputs("menu ", stdout);
	if (eb_have_copyright(&book))
	    fputs("copyright ", stdout);
	fputc('\n', stdout);

	/*
	 * Output a font list.
	 */
	fputs("  font sizes: ", stdout);
	font_count = eb_font_list(&book, font_list);
	for (j = 0; j < font_count; j++)
	    printf("%d ", (int)font_list[j]);
	fputc('\n', stdout);

	/*
	 * Output characters information about the font.
	 */
	if (eb_have_narrow_font(&book)) {
	    if (eb_set_font(&book, font_list[0]) < 0)
		goto failed;
	    start = eb_narrow_font_start(&book);
	    if (start < 0)
		goto failed;
	    end = eb_narrow_font_end(&book);
	    if (end < 0)
		goto failed;
	    printf("  narrow font characters: 0x%04x -- 0x%04x\n", start, end);
	} else
	    fputs("  narrow font characters: \n", stdout);

	if (eb_have_wide_font(&book)) {
	    if (eb_set_font(&book, font_list[0]) < 0)
		goto failed;
	    start = eb_wide_font_start(&book);
	    if (start < 0)
		goto failed;
	    end = eb_wide_font_end(&book);
	    if (end < 0)
		goto failed;
	    printf("  wide font characters: 0x%04x -- 0x%04x\n", start, end);
	} else
	    fputs("  wide font characters: \n", stdout);

	if (multiflag)
	    output_multi_information(&book);
	fputc('\n', stdout);
    }
    fflush(stdout);

    /*
     * End to use the book.
     */
    eb_clear(&book);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stdout);
    fprintf(stderr, "%s: %s\n", invoked_name, eb_error_message());
    fflush(stderr);
    eb_clear(&book);

    return -1;
}


/*
 * Output information about multi searches.
 */
static void
output_multi_information(book)
    EB_Book *book;
{
    EB_Multi_Search_Code multi_list[EB_MAX_MULTI_SEARCHES];
    EB_Multi_Entry_Code entry_list[EB_MAX_MULTI_ENTRIES];
    int multi_count, entry_count;
    const char *label;
    int i, j;

    multi_count = eb_multi_search_list(book, multi_list);
    for (i = 0; i < multi_count; i++) {
	printf("  multi search %d:\n", i + 1);
	entry_count = eb_multi_entry_list(book, multi_list[i], entry_list);

	for (j = 0; j < entry_count; j++) {
	    label = eb_multi_entry_label(book, multi_list[i], entry_list[j]);
	    if (label == NULL) {
		fprintf(stderr, "%s: %s\n\n", invoked_name,
		    eb_error_message());
		fflush(stderr);
		continue;
	    }

	    printf("    label %d: %s\n", j + 1, label);
	    fputs("    search methods: ", stdout);
	    if (eb_multi_entry_have_word_search(book, multi_list[i],
		entry_list[j]))
		fputs("word ", stdout);
	    if (eb_multi_entry_have_endword_search(book, multi_list[i],
		entry_list[j]))
		fputs("endword ", stdout);
	    if (eb_multi_entry_have_keyword_search(book, multi_list[i],
		entry_list[j]))
		fputs("keyword ", stdout);
	    fputc('\n', stdout);
	}
    }
    fflush(stdout);
}


/*
 * Output version number to stdandard out.
 */
static void
output_version()
{
    printf("%s (EB Library) version %s\n", program_name, program_version);
    printf("Copyright (c) 1997, 1998  Motoyuki Kasahara\n\n");
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
 * Output help message to stdandard out.
 */
static void
output_help()
{
    printf("Usage: %s [option...] [book-directory]\n",
	program_name);
    printf("Options:\n");
    printf("  -h  --help                 display this help, then exit\n");
    printf("  -m  --multi-search         also output multi-search information\n");
    printf("  -v  --version              display version number, then exit\n");
    printf("\nArgument:\n");
    printf("  book-directory             top directory of a CD-ROM book\n");
    printf("                             (default: %s)\n",
	DEFAULT_BOOK_DIRECTORY);
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

