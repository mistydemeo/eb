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

#include "eb/eb.h"
#include "eb/error.h"
#include "eb/font.h"

#include "fakelog.h"
#include "getopt.h"
#include "getumask.h"
#include "makedir.h"
#include "ebutils.h"

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp()
int strncasecmp();
#endif /* not __STDC__ */
#endif /* not HAVE_STRCASECMP */

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef HAVE_STRERROR
#ifdef __STDC__
char *strerror(int);
#else /* not __STDC__ */
char *strerror();
#endif /* not __STDC__ */
#endif /* HAVE_STRERROR */

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
 * Trick for difference of path notation between UNIX and DOS.
 */
#ifndef DOS_FILE_PATH
#define F_(path1, path2) (path1)
#else
#define F_(path1, path2) (path2)
#endif

/*
 * Command line options.
 */
static const char *short_options = "df:hi:o:S:v";
static struct option long_options[] = {
  {"debug",             no_argument,       NULL, 'd'},
  {"verbose",           no_argument,       NULL, 'd'},
  {"font-height",       required_argument, NULL, 'f'},
  {"help",              no_argument,       NULL, 'h'},
  {"image-format",      required_argument, NULL, 'i'},
  {"subbook",           required_argument, NULL, 'S'},
  {"output-directory",  required_argument, NULL, 'o'},
  {"version",           no_argument,       NULL, 'v'},
  {NULL, 0, NULL, 0}
};

/*
 * Supported image formats.
 */
typedef int Image_Format_Code;

#define IMAGE_FORMAT_XBM	0
#define IMAGE_FORMAT_XPM	1
#define IMAGE_FORMAT_GIF	2

typedef struct {
    const char *name;
    const char *suffix;
    void (*function) EB_P((const char *, int, int, char *, size_t *));
} Image_Format;

static Image_Format image_formats[] = {
    {"xbm", "xbm", eb_bitmap_to_xbm},
    {"xpm", "xpm", eb_bitmap_to_xpm},
    {"gif", "gif", eb_bitmap_to_gif},
    {NULL, NULL, NULL}
};

#define MAX_IMAGE_FORMATS	3
#define MAX_LENGTH_IMAGE_NAME	3
#define MAX_LENGTH_IMAGE_SUFFIX	3

/*
 * Debug flag.
 */
static int debug_flag;

/*
 * List of target subbook names.
 */
char subbook_name_list[EB_MAX_SUBBOOKS][EB_MAX_DIRECTORY_NAME_LENGTH + 1];
int subbook_name_count = 0;

/*
 * List of target subbook codes.
 */
EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
int subbook_count = 0;

/*
 * List of target font heights.
 */
EB_Font_Code font_list[EB_MAX_FONTS];
int font_count = 0;

/*
 * Target Image formats.
 */
Image_Format_Code image_list[MAX_IMAGE_FORMATS];
int image_count = 0;

/*
 * Defaults and limitations.
 */
#define DEFAULT_FONT_HEIGHT		"16"
#define DEFAULT_IMAGE_FORMAT		"xbm"
#define DEFAULT_BOOK_DIRECTORY		"."
#define DEFAULT_OUTPUT_DIRECTORY	"."

#define MAX_LENGTH_FONT_NAME		2
#define MAX_LENGTH_STRING		255

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
static int parse_image_argument EB_P((const char *, int *,
    Image_Format_Code *));
static int parse_font_argument EB_P((const char *, int *, EB_Font_Code *));
static void output_help EB_P((void));
static int make_book_fonts EB_P((EB_Book *, const char *, EB_Subbook_Code *,
    int, EB_Font_Code *, int, Image_Format_Code *, int));
static int make_subbook_fonts EB_P((EB_Book *, const char *, EB_Font_Code *,
    int, Image_Format_Code *, int));
static int make_subbook_size_fonts EB_P((EB_Book *, const char *,
    Image_Format_Code *, int));
static int make_subbook_size_image_fonts EB_P((EB_Book *, const char *,
    Image_Format_Code));
static int save_image_file EB_P((const char *, const char *, size_t));


int
main(argc, argv)
    int argc;
    char *argv[];
{
    const char *book_path;
    char out_path[PATH_MAX + 1];
    EB_Error_Code error_code;
    EB_Book book;
    int ch;
    
    program_name = "ebfont";
    invoked_name = argv[0];
    debug_flag = 0;
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
	case 'd':
	    /*
	     * Option `-d'.  Debug mode.
	     */
	    debug_flag = 1;
	    set_fakelog_level(FAKELOG_DEBUG);
	    break;

	case 'f':
	    /*
	     * Option `-f'.  Generate fonts with HEIGHT.
	     */
	    if (parse_font_argument(optarg, &font_count, font_list) < 0) {
		output_try_help();
		goto die;
	    }
	    break;

	case 'h':
	    /*
	     * Option `-h'.  Display help message, then exit.
	     */
	    output_help();
	    exit(0);

	case 'i':
	    /*
	     * Option `-i'.  Generate fonts as FORMAT.
	     */
	    if (parse_image_argument(optarg, &image_count, image_list) < 0) {
		output_try_help();
		goto die;
	    }
	    break;

	case 'o':
	    /*
	     * Option `-o'.  Output fonts under DIRECTORY.
	     * The file names of fonts are:
	     *     "<path>/<subbook>/fonts/<height>/{narrow,wide}/
	     *         <charno>.<suffix>"
	     *
	     * <path>:    is `optarg'.
	     * <subbook>: require EB_MAX_DIRECTORY_NAME_LENGTH characters.
	     * fonts:     require 5 characters.
	     * <height>:  require 2 characters ("16", "24", "32", or "48").
	     * {narrow,wide}:
	     *            requre 6 characters (= strlen("narrow")).
	     * <charno>:  requre 4 characters. ("0000" ... "ffff")
	     * <suffix>:  requre MAX_LENGTH_IMAGE_SUFFIX characters.
	     */
	    if (PATH_MAX < strlen(optarg)) {
		fprintf(stderr, _("%s: too long output directory path\n"),
		    invoked_name);
		goto die;
	    }
	    strcpy(out_path, optarg);
	    canonicalize_path(out_path);
	    if (PATH_MAX < strlen(out_path) + 1 + EB_MAX_DIRECTORY_NAME_LENGTH
		+ 1 + 5 + 1 + 2 + 1 + 6 + 1 + 4 + 1
		+ MAX_LENGTH_IMAGE_SUFFIX) {
		fprintf(stderr, _("%s: too long output directory path\n"),
		    invoked_name);
		goto die;
	    }
	    break;

        case 'S':
            /*
             * Option `-S'.  Specify target subbooks.
             */
            if (parse_subbook_name_argument(optarg, subbook_name_list,
		&subbook_name_count) < 0) {
		output_try_help();
		goto die;
	    }
            break;

	case 'v':
	    /*
	     * Option `-v'.  Display version number, then exit.
	     */
	    output_version();
	    exit(0);

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

    if (image_count == 0)
	parse_image_argument(DEFAULT_IMAGE_FORMAT, &image_count, image_list);

    if (font_count == 0)
	parse_font_argument(DEFAULT_FONT_HEIGHT, &font_count, font_list);

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
    error_code = eb_bind(&book, book_path);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: %s\n", invoked_name,
	    eb_error_message(error_code));
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
	    goto die;
	}
    } else {
	int i;
	EB_Subbook_Code sub;

	for (i = 0; i < subbook_name_count; i++) {
	    error_code = find_subbook(&book, subbook_name_list[i], &sub);
	    if (error_code != EB_SUCCESS) {
		fprintf(stderr, _("%s: unknown subbook name `%s'\n"),
		    invoked_name, subbook_name_list[i]);
	    }
	    subbook_list[subbook_count++] = sub;
	}
    }

    /*
     * Make image files for fonts in the book.
     */
    if (make_book_fonts(&book, out_path, subbook_list, subbook_count,
	font_list, font_count, image_list, image_count) < 0)
	goto die;

    /*
     * Dispose the book.
     */
    eb_finalize_book(&book);
    eb_finalize_library();

    return 0;

    /*
     * A critical error occurs...
     */
  die:
    fflush(stderr);
    eb_finalize_book(&book);
    eb_finalize_library();
    exit(1);
}


/*
 * Parse an argument to option `--font-height (-f)'
 */
static int
parse_font_argument(argument, font_count, font_list)
    const char *argument;
    int *font_count;
    EB_Font_Code *font_list;
{
    const char *argument_p = argument;
    char font_name[MAX_LENGTH_FONT_NAME + 1];
    char *font_name_p;
    EB_Font_Code font_code;
    int i;

    while (*argument_p != '\0') {
	/*
	 * Take a next element in the argument.
	 */
	i = 0;
	font_name_p = font_name;
	while (*argument_p != ',' && *argument_p != '\0'
	    && i < MAX_LENGTH_FONT_NAME) {
	    *font_name_p = *argument_p;
	    i++;
	    font_name_p++;
	    argument_p++;
	}
	*font_name_p = '\0';
	if (*argument_p == ',')
	    argument_p++;
	else if (*argument_p != '\0') {
	    fprintf(stderr, _("%s: unknown font height `%s...'\n"),
		invoked_name, font_name);
	    goto failed;
	}

	/*
	 * Check for the font name.
	 */
	if (strcmp(font_name, "16") == 0)
	    font_code = EB_FONT_16;
	else if (strcmp(font_name, "24") == 0)
	    font_code = EB_FONT_24;
	else if (strcmp(font_name, "30") == 0)
	    font_code = EB_FONT_30;
	else if (strcmp(font_name, "48") == 0)
	    font_code = EB_FONT_48;
	else {
	    fprintf(stderr, _("%s: unknown font height `%s'\n"),
		invoked_name, font_name);
	    fflush(stderr);
	    goto failed;
	}

	/*
	 * If the font name is not found in `font_list', it is added to
	 * the list.
	 */
	for (i = 0; i < *font_count; i++) {
	    if (*(font_list + i) == font_code)
		break;
	}
	if (*font_count <= i) {
	    *(font_list + i) = font_code;
	    (*font_count)++;
	}
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stderr);
    return -1;
}


/*
 * Parse an argument to option `--font-image-format (-i)'
 */
static int
parse_image_argument(argument, image_count, image_list)
    const char *argument;
    int *image_count;
    Image_Format_Code *image_list;
{
    const char *argument_p = argument;
    char image_name[MAX_LENGTH_IMAGE_NAME + 1];
    char *image_name_p;
    int image_code;
    int i;

    while (*argument_p != '\0') {
	/*
	 * Take a next element in the argument.
	 */
	i = 0;
	image_name_p = image_name;
	while (*argument_p != ',' && *argument_p != '\0'
	    && i < MAX_LENGTH_IMAGE_NAME) {
	    *image_name_p = *argument_p;
	    i++;
	    image_name_p++;
	    argument_p++;
	}
	*image_name_p = '\0';
	if (*argument_p == ',')
	    argument_p++;
	else if (*argument_p != '\0') {
	    fprintf(stderr, _("%s: unknown image format name `%s...'\n"),
		invoked_name, image_name);
	    goto failed;
	}

	/*
	 * Check for the image-format name.
	 */
	for (i = 0; i < MAX_IMAGE_FORMATS; i++) {
	    if (strcasecmp((image_formats + i)->name, image_name) == 0)
		break;
	}
	if (MAX_IMAGE_FORMATS <= i) {
	    fprintf(stderr, _("%s: unknown image format name `%s'\n"),
		invoked_name, image_name);
	    goto failed;
	}
	image_code = i;

	/*
	 * If the image-format name is not found in `image_list', it
	 * is added to the list.
	 */
	for (i = 0; i < *image_count; i++) {
	    if (*(image_list + i) == image_code)
		break;
	}
	if (*image_count <= i) {
	    *(image_list + i) = image_code;
	    (*image_count)++;
	}
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stderr);
    return -1;
}


/*
 * Output help message to standard out, then exit.
 */
static void
output_help()
{
    printf(_("Usage: %s [option...] [book-directory]\n"), program_name);
    printf(_("Options:\n"));
    printf(_("  -d  --debug  --verbose     degug mode\n"));
    printf(_("  -f HEIGHT[,HEIGHT...]  --font-height HEIGHT[,HEIGHT...]\n"));
    printf(_("                             generate fonts with HEIGHT; 16, 24, 30 or 48\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_FONT_HEIGHT);
    printf(_("  -h  --help                 display this help, then exit\n"));
    printf(_("  -i FORMAT[,FORMAT...]  --image-format FORMAT[,FORMAT...]\n"));
    printf(_("                             generate fonts as FORMAT; xbm, xpm, or gif\n"));
    printf(_("                             (default: %s)\n"),
	DEFAULT_IMAGE_FORMAT);
    printf(_("  -o DIRECTORY  --output-directory DIRECTORY\n"));
    printf(_("                             output fonts under DIRECTORY\n"));
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
 * Make font-files in the `book_path'.
 */
static int
make_book_fonts(book, out_path, subbook_list, subbook_count, font_list,
    font_count, image_list, image_count)
    EB_Book *book;
    const char *out_path;
    EB_Subbook_Code *subbook_list;
    int subbook_count;
    EB_Font_Code *font_list;
    int font_count;
    Image_Format_Code *image_list;
    int image_count;
{
    EB_Error_Code error_code;
    char subbook_path[PATH_MAX + 1];
    char subbook_directory[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int i;

    /*
     * If `book_path' represents "/", replace it to an empty string,
     * or `file_name' starts with double slashes.
     */
    if (strcmp(out_path, "/") == 0)
	out_path++;

    for (i = 0; i < subbook_count; i++) {
	/*
	 * Set the current subbook to `subbook_list[i]'.
	 */
	error_code = eb_set_subbook(book, subbook_list[i]);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s: subbook=%d\n", invoked_name,
		eb_error_message(error_code), subbook_list[i]);
	    goto failed;
	}

	/*
	 * Get directory name of the subbook.
	 */
	error_code = eb_subbook_directory(book, subbook_directory);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s: subbook=%d\n", invoked_name,
		eb_error_message(error_code), subbook_list[i]);
	    goto failed;
	}

	/*
	 * Output debug information.
	 */
	if (debug_flag) {
	    fprintf(stderr, "%s: debug: subbook %s\n", invoked_name,
		subbook_directory);
	}

	/*
	 * Make a directory for the subbook.
	 */
	sprintf(subbook_path, F_("%s/%s", "%s\\%s"),
	    out_path, subbook_directory);
	if (make_missing_directory(subbook_path, 0777 ^ get_umask()) < 0)
	    goto failed;

	/*
	 * Make fonts in the subbook.
	 */
	if (make_subbook_fonts(book, subbook_path, font_list, font_count,
	    image_list, image_count) < 0)
	    goto failed;
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stderr);
    return -1;
}


/*
 * Make font-files in the current subbook.
 */
static int
make_subbook_fonts(book, subbook_path, font_list, font_count, image_list,
    image_count)
    EB_Book *book;
    const char *subbook_path;
    EB_Font_Code *font_list;
    int font_count;
    Image_Format_Code *image_list;
    int image_count;
{
    EB_Error_Code error_code;
    char subbook_directory[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char font_path[PATH_MAX + 1];
    int font_height;
    int i;

    /*
     * Get the current subbook name.
     */
    error_code = eb_subbook_directory(book, subbook_directory);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: %s\n", invoked_name,
	    eb_error_message(error_code));
	goto failed;
    }

    if (debug_flag) {
	fprintf(stderr, "%s: debug: subbook %s:\n", invoked_name,
	    subbook_directory);
    }

    for (i = 0; i < font_count; i++) {
	/*
	 * Set the current font to `font_list[i]'.
	 */
	if (!eb_have_font(book, font_list[i]))
	    continue;
	error_code = eb_set_font(book, font_list[i]);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d\n", invoked_name,
		eb_error_message(error_code), subbook_directory, i);
	    goto failed;
	}

	/*
	 * Output debug information.
	 */
	if (debug_flag) {
	    fprintf(stderr, "%s: debug: font %d\n", invoked_name, 
		font_list[i]);
	}

	/*
	 * Make a directory for the font.
	 */
	eb_font_height2(font_list[i], &font_height);
	sprintf(font_path, F_("%s/%d", "%s\\%d"), subbook_path, font_height);
	if (make_missing_directory(font_path, 0777 ^ get_umask()) < 0)
	    goto failed;

	/*
	 * Make font-files with the size.
	 */
	if (make_subbook_size_fonts(book, font_path, image_list, image_count)
	    < 0)
	    goto failed;
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stderr);
    return -1;
}


/*
 * Make font-files of the current font.
 */
static int
make_subbook_size_fonts(book, font_path, image_list, image_count)
    EB_Book *book;
    const char *font_path;
    Image_Format_Code *image_list;
    int image_count;
{
    EB_Error_Code error_code;
    char subbook_directory[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    int i;

    /*
     * Get the current subbook name.
     */
    error_code = eb_subbook_directory(book, subbook_directory);
    if (subbook_directory == NULL) {
	fprintf(stderr, "%s: %s\n", invoked_name,
	    eb_error_message(error_code));
	goto failed;
    }

    for (i = 0; i < image_count; i++) {
	/*
	 * Output debug information.
	 */
	if (debug_flag) {
	    fprintf(stderr, "%s: debug: image %s\n", invoked_name,
		image_formats[image_list[i]].name);
	}

	/*
	 * Make font-files as the image format.
	 */
	if (make_subbook_size_image_fonts(book, font_path, image_list[i]) < 0)
	    goto failed;
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stderr);
    return -1;
}


/*
 * Make font-files of the current font as the image format.
 */
static int
make_subbook_size_image_fonts(book, image_path, image)
    EB_Book *book;
    const char *image_path;
    Image_Format_Code image;
{
    EB_Error_Code error_code;
    char subbook_directory[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char type_path[PATH_MAX + 1];
    char file_name[PATH_MAX + 1];
    char bitmap_data[EB_SIZE_WIDE_FONT_48];
    char image_data[EB_SIZE_FONT_IMAGE];
    size_t image_size;
    int image_width;
    int image_height;
    int character_number;

    /*
     * Get the current subbook name.
     */
    error_code = eb_subbook_directory(book, subbook_directory);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: %s\n", invoked_name,
	    eb_error_message(error_code));
	goto failed;
    }

    /*
     * Get the current font size.
     */
    error_code = eb_font_height(book, &image_height);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: %s: subbook=%s\n", invoked_name,
	    eb_error_message(error_code), subbook_directory);
	goto failed;
    }

    /*
     * Make the bitmap files of narrow fonts.
     */
    if (eb_have_narrow_font(book)) {
	/*
	 * Get narrow font information.
	 */
	error_code = eb_narrow_font_width(book, &image_width);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=narrow\n",
		invoked_name, eb_error_message(error_code), subbook_directory,
		image_height);
	    goto failed;
	}
	error_code = eb_narrow_font_start(book, &character_number);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=narrow\n",
		invoked_name, eb_error_message(error_code), subbook_directory,
		image_height);
	    goto failed;
	}

	/*
	 * Make a directory for the narrow font.
	 */
	sprintf(type_path, F_("%s/narrow", "%s\\narrow"), image_path);
	if (make_missing_directory(type_path, 0777 ^ get_umask()) < 0)
	    goto failed;

	while (0 <= character_number) {
	    /*
	     * Output debug information.
	     */
	    if (debug_flag) {
		fprintf(stderr, "%s: debug: character %04x\n", invoked_name,
		    character_number);
	    }

	    /*
	     * Generate a bitmap file for the character `character_number'.
	     */
	    sprintf(file_name, F_("%s/%04x.%s", "%s\\%04x.%s"),
		type_path, character_number, image_formats[image].suffix);
	    error_code = eb_narrow_font_character_bitmap(book,
		character_number, bitmap_data);
	    if (error_code != EB_SUCCESS) {
		fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=narrow, \
character=0x%04x\n",
		    invoked_name, eb_error_message(error_code),
		    subbook_directory, image_height, character_number);
		goto failed;
	    }
	    (image_formats[image].function)(bitmap_data, image_width,
		image_height, bitmap_data, &image_size);
	    if (save_image_file(file_name, image_data, image_size) < 0)
		goto failed;

	    /*
	     * Toward next charahacter.
	     */
	    eb_forward_narrow_font_character(book, 1, &character_number);
	}
    }

    /*
     * Make the bitmap files of wide fonts.
     */
    if (eb_have_wide_font(book)) {
	/*
	 * Get wide font information.
	 */
	error_code = eb_wide_font_width(book, &image_width);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=wide\n",
		invoked_name, eb_error_message(error_code), subbook_directory,
		image_height);
	    goto failed;
	}
	error_code = eb_wide_font_start(book, &character_number);
	if (error_code != EB_SUCCESS) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=wide\n",
		invoked_name, eb_error_message(error_code), subbook_directory,
		image_height);
	    goto failed;
	}

	/*
	 * Make a directory for the wide font.
	 */
	sprintf(type_path, F_("%s/wide", "%s\\wide"), image_path);
	if (make_missing_directory(type_path, 0777 ^ get_umask()) < 0)
	    goto failed;

	while (0 <= character_number) {
	    /*
	     * Output debug information.
	     */
	    if (debug_flag) {
		fprintf(stderr, "%s: debug: character %04x\n", invoked_name,
		    character_number);
	    }

	    /*
	     * Generate a bitmap file for the character `character_number'.
	     */
	    sprintf(file_name, F_("%s/%04x.%s", "%s\\%04x.%s"),
		type_path, character_number, image_formats[image].suffix);
	    error_code = eb_wide_font_character_bitmap(book, character_number,
		bitmap_data);
	    if (error_code != EB_SUCCESS) {
		fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=wide, \
character=0x%04x\n",
		    invoked_name, eb_error_message(error_code),
		    subbook_directory, image_height, character_number);
		goto failed;
	    }
	    (image_formats[image].function)(bitmap_data, image_width,
		image_height, bitmap_data, &image_size);
	    if (save_image_file(file_name, image_data, image_size) < 0)
		goto failed;

	    /*
	     * Toward next charahacter.
	     */
	    eb_forward_wide_font_character(book, 1, &character_number);
	}
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    fflush(stderr);
    return -1;
}


/*
 * Save an image file.
 */
static int
save_image_file(file_name, image_data, image_size)
    const char *file_name;
    const char *image_data;
    size_t image_size;
{
    int file = -1;

#ifdef O_CREAT
    file = open(file_name, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	0666 ^ get_umask());
#else
    file = creat(file_name, 0666 ^ get_umask());
#endif

    if (file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), file_name);
	goto failed;
    }
    if (write(file, image_data, image_size) != image_size) {
	fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
	    invoked_name, strerror(errno), file_name);
	goto failed;
    }
    if (close(file) < 0) {
	fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
	    invoked_name, strerror(errno), file_name);
	file = -1;
	goto failed;
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file && close(file) < 0) {
	fprintf(stderr, _("%s: failed to close the file, %s: %s\n"),
	    invoked_name, strerror(errno), file_name);
    }

    return -1;
}

