/*
 * Copyright (c) 1997, 98, 99, 2000  Motoyuki Kasahara
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
#include "ebutils.h"

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>
#endif

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
 * Unexported functions.
 */
static void output_error_message EB_P((EB_Error_Code));
static int output_information EB_P((const char *, int));
static void output_help EB_P((void));
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
 * Default output directory
 */
#define DEFAULT_BOOK_DIRECTORY	"."

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int ch;
    char *book_path;
    int multi_flag;

    program_name = "ebinfo";
    invoked_name = argv[0];

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
     * Set fakelog behavior.
     */
    set_fakelog_name(invoked_name);
    set_fakelog_mode(FAKELOG_TO_STDERR);
    set_fakelog_level(FAKELOG_ERR);

    /*
     * Parse command line options.
     */
    multi_flag = 0;
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
	    multi_flag = 1;
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
	fprintf(stderr, _("%s: too many arguments\n"), invoked_name);
	output_try_help();
	exit(1);
    }

    /*
     * Output information about the book.
     */
    if (argc == optind)
	book_path = DEFAULT_BOOK_DIRECTORY;
    else
	book_path = argv[optind];

    if (output_information(book_path, multi_flag) < 0)
	exit(1);

    return 0;
}


/*
 * Output an error message to standard error.
 */
static void
output_error_message(error_code)
    EB_Error_Code error_code;
{
    fprintf(stderr, "%s: %s\n", invoked_name, eb_error_message(error_code));
    fflush(stderr);
}


/*
 * Output information about the book at `path'.
 * If `multi_flag' is enabled, multi-search information are also output.
 */
static int
output_information(book_path, multi_flag)
    const char *book_path;
    int multi_flag;
{
    EB_Book book;
    EB_Error_Code error_code;
    EB_Disc_Code disc_code;
    EB_Character_Code character_code;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    EB_Font_Code font_list[EB_MAX_FONTS];
    char title[EB_MAX_TITLE_LENGTH + 1];
    char directory[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int font_height, font_start, font_end;
    int subbook_count;
    int font_count;
    int i, j;

    /*
     * Start to use a book.
     */
    eb_initialize_library();
    eb_initialize_book(&book);
    error_code = eb_bind(&book, book_path);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * Output disc type.
     */
    error_code = eb_disc_type(&book, &disc_code);
    if (error_code != EB_SUCCESS)
	goto failed;
    printf(_("disc type: "));
    if (disc_code == EB_DISC_EB)
	printf("EB/EBG/EBXA/EBXA-C/S-EBXA\n");
    else
	printf("EPWING\n");

    /*
     * Output character code.
     */
    error_code = eb_character_code(&book, &character_code);
    if (error_code != EB_SUCCESS)
	goto failed;
    printf(_("character code: "));
    switch (character_code) {
    case EB_CHARCODE_ISO8859_1:
	printf("ISO 8859-1\n");
	break;
    case EB_CHARCODE_JISX0208:
	printf("JIS X 0208\n");
	break;
    case EB_CHARCODE_JISX0208_GB2312:
	printf("JIS X 0208 + GB 2312\n");
	break;
    default:
	printf(_("unknown\n"));
	break;
    }

    /*
     * Output the number of subbooks in the book.
     */
    error_code = eb_subbook_list(&book, subbook_list, &subbook_count);
    if (error_code != EB_SUCCESS)
	goto failed;
    printf(_("the number of subbooks: %d\n\n"), subbook_count);

    /*
     * Output information about each subbook.
     */
    for (i = 0; i < subbook_count; i++) {
	printf(_("subbook %d:\n"), i + 1);

	/*
	 * Output a title of the subbook.
	 */
	error_code = eb_subbook_title2(&book, subbook_list[i], title);
	if (error_code != EB_SUCCESS)
	    continue;
	printf(_("  title: %s\n"), title);

	/*
	 * Output a directory name of the subbook.
	 */
	error_code = eb_subbook_directory2(&book, subbook_list[i], directory);
	if (error_code != EB_SUCCESS)
	    continue;
	printf(_("  directory: %s\n"), directory);

	/*
	 * Set the current subbook to `i'.
	 */
	error_code = eb_set_subbook(&book, subbook_list[i]);
	if (error_code != EB_SUCCESS) {
	    output_error_message(error_code);
	    continue;
	}

	/*
	 * Output supported methods.
	 */
	printf(_("  search methods: "));
	if (eb_have_word_search(&book))
	    fputs(_("word "), stdout);
	if (eb_have_endword_search(&book))
	    fputs(_("endword "), stdout);
	if (eb_have_keyword_search(&book))
	    fputs(_("keyword "), stdout);
	if (eb_have_multi_search(&book))
	    fputs(_("multi "), stdout);
	if (eb_have_graphic_search(&book))
	    fputs(_("graphic "), stdout);
	if (eb_have_menu(&book))
	    fputs(_("menu "), stdout);
	if (eb_have_copyright(&book))
	    fputs(_("copyright "), stdout);
	fputc('\n', stdout);

	/*
	 * Output a font list.
	 */
	fputs(_("  font sizes: "), stdout);
	error_code = eb_font_list(&book, font_list, &font_count);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s\n\n", invoked_name,
		eb_error_message(error_code));
	    fflush(stderr);
	} else {
	    for (j = 0; j < font_count; j++) {
		error_code = eb_font_height2(font_list[j], &font_height);
		if (error_code == EB_SUCCESS)
		    printf("%d ", font_height);
	    }
	    fputc('\n', stdout);
	}

	/*
	 * Output character range of the narrow font.
	 */
        fputs(_("  narrow font characters: "), stdout);
	if (eb_have_narrow_font(&book)) {
	    do {
		error_code = eb_set_font(&book, font_list[0]);
		if (error_code != EB_SUCCESS) {
		    output_error_message(error_code);
		    break;
		}
		error_code = eb_narrow_font_start(&book, &font_start);
		if (error_code != EB_SUCCESS) {
		    output_error_message(error_code);
		    break;
		}
		error_code = eb_narrow_font_end(&book, &font_end);
		if (error_code != EB_SUCCESS) {
		    output_error_message(error_code);
		    break;
		}
		printf("0x%04x -- 0x%04x\n", font_start, font_end);
	    } while (0);
	} else {
	    fputc('\n', stdout);
	}

	/*
	 * Output character range of the wide font.
	 */
	printf(_("  wide font characters: "));
	if (eb_have_wide_font(&book)) {
	    do {
		error_code = eb_set_font(&book, font_list[0]);
		if (error_code != EB_SUCCESS) {
		    output_error_message(error_code);
		    break;
		}
		error_code = eb_wide_font_start(&book, &font_start);
		if (error_code != EB_SUCCESS) {
		    output_error_message(error_code);
		    break;
		}
		error_code = eb_wide_font_end(&book, &font_end);
		if (error_code != EB_SUCCESS) {
		    output_error_message(error_code);
		    break;
		}
		printf("0x%04x -- 0x%04x\n", font_start, font_end);
	    } while (0);
	} else {
	    fputc('\n', stdout);
	}

	if (multi_flag)
	    output_multi_information(&book);
	fputc('\n', stdout);
    }
    fflush(stdout);

    /*
     * End to use the book.
     */
    eb_finalize_book(&book);
    eb_finalize_library();

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stdout);
    fflush(stderr);
    output_error_message(error_code);
    eb_finalize_book(&book);
    eb_finalize_library();

    return -1;
}

/*
 * Output information about multi searches.
 */
static void
output_multi_information(book)
    EB_Book *book;
{
    EB_Error_Code error_code;
    EB_Multi_Search_Code multi_list[EB_MAX_MULTI_SEARCHES];
    EB_Multi_Entry_Code entry_list[EB_MAX_MULTI_ENTRIES];
    int multi_count;
    int entry_count;
    char entry_label[EB_MAX_MULTI_LABEL_LENGTH + 1];
    int i, j;

    error_code = eb_multi_search_list(book, multi_list, &multi_count);
    if (error_code != EB_SUCCESS) {
	output_error_message(error_code);
	return;
    }
    for (i = 0; i < multi_count; i++) {
	printf(_("  multi search %d:\n"), i + 1);
	error_code = eb_multi_entry_list(book, multi_list[i], entry_list,
	    &entry_count);
	if (error_code != EB_SUCCESS) {
	    output_error_message(error_code);
	    continue;
	}
	for (j = 0; j < entry_count; j++) {
	    error_code = eb_multi_entry_label(book, multi_list[i],
		entry_list[j], entry_label);
	    if (error_code != EB_SUCCESS) {
		output_error_message(error_code);
		continue;
	    }

	    printf(_("    label %d: %s\n"), j + 1, entry_label);
	    fputs(_("      candidates: "), stdout);
	    if (eb_multi_entry_have_candidates(book, multi_list[i],
		entry_list[j]))
		fputs(_("exist\n"), stdout);
	    else 
		fputs(_("not-exist\n"), stdout);
	}
    }
    fflush(stdout);
}


/*
 * Output help message to stdandard out.
 */
static void
output_help()
{
    printf(_("Usage: %s [option...] [book-directory]\n"),
	program_name);
    printf(_("Options:\n"));
    printf(_("  -h  --help                 display this help, then exit\n"));
    printf(_("  -m  --multi-search         also output multi-search information\n"));
    printf(_("  -v  --version              display version number, then exit\n"));
    printf(_("\nArgument:\n"));
    printf(_("  book-directory             top directory of a CD-ROM book\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_BOOK_DIRECTORY);
    printf(_("\nReport bugs to %s.\n"), MAILING_ADDRESS);
    fflush(stdout);
}


