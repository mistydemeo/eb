/*
 * Copyright (c) 1997, 1998, 1999  Motoyuki Kasahara
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

#ifndef HAVE_STRCASECMP
#ifdef __STDC__
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#else /* not __STDC__ */
int strcasecmp()
int strncasecmp();
#endif /* not __STDC__ */
#endif /* not HAVE_STRCASECMP */

#include "eb/eb.h"
#include "eb/error.h"
#include "eb/font.h"

#include "fakelog.h"
#include "getopt.h"
#include "getumask.h"
#include "makedir.h"

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
    size_t (*function) EB_P((char *, const char *, int, int));
} Image_Format;

static Image_Format image_formats[] = {
    {"xbm", "xbm", eb_bitmap_to_xbm},
    {"xpm", "xpm", eb_bitmap_to_xpm},
    {"gif", "gif", eb_bitmap_to_gif},
    {NULL, NULL, NULL}
};

#define MAX_IMAGE_FORMATS	3
#define MAXLEN_IMAGE_NAME	3
#define MAXLEN_IMAGE_SUFFIX	3

/*
 * Generic name of the program.
 */
const char *program_name = "ebfont";

/*
 * Program version.
 */
const char *program_version = VERSION;

/*
 * Actual program name (argv[0])
 */
const char *invoked_name;

/*
 * Debug flag.
 */
static int debug_flag;

/*
 * Defaults and limitations.
 */
#define DEFAULT_FONT_HEIGHT		"16"
#define DEFAULT_IMAGE_FORMAT		"xbm"
#define DEFAULT_BOOK_DIRECTORY		"."
#define DEFAULT_OUTPUT_DIRECTORY	"."

#define MAXLEN_FONT_NAME		2
#define MAXLEN_STRING			255

/*
 * Unexported functions.
 */
static int parse_image_argument EB_P((const char *, int *,
    Image_Format_Code *));
static int parse_font_argument EB_P((const char *, int *, EB_Font_Code *));
static int parse_subname_argument EB_P((const char *, int *,
    char subname_list[][EB_MAXLEN_BASENAME + 1]));
static EB_Subbook_Code find_subbook EB_P((EB_Book *, const char *));
static void output_version EB_P((void));
static void output_help EB_P((void));
static void output_try_help EB_P((void));
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
    EB_Book book;
    char subname_list[EB_MAX_SUBBOOKS][EB_MAXLEN_BASENAME + 1];
    int subname_count = 0;
    EB_Subbook_Code sub_list[EB_MAX_SUBBOOKS];
    int sub_count = 0;
    EB_Font_Code font_list[EB_MAX_FONTS];
    int font_count = 0;
    Image_Format_Code image_list[MAX_IMAGE_FORMATS];
    int image_count = 0;
    int ch;
    
    invoked_name = argv[0];
    debug_flag = 0;
    strcpy(out_path, DEFAULT_OUTPUT_DIRECTORY);

    /*
     * Initialize `book'.
     */
    eb_initialize(&book);

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
	     * The filenames of fonts are:
	     *     "<path>/<subbook>/fonts/<height>/{narrow,wide}/
	     *         <charno>.<suffix>"
	     *
	     * <path>:    is `optarg'.
	     * <subbook>: require EB_MAXLEN_BASENAME characters.
	     * fonts:     require 5 characters.
	     * <height>:  require 2 characters ("16", "24", "32", or "48").
	     * <image>:   require MAXLEN_IMAGE_NAME characters.
	     * {narrow,wide}:
	     *            requre 6 characters (= strlen("narrow")).
	     * <charno>:  requre 4 characters. ("0000" ... "ffff")
	     * <suffix>:  requre MAXLEN_IMAGE_SUFFIX characters.
	     */
	    if (PATH_MAX < strlen(optarg)) {
		fprintf(stderr, "%s: too long output directory path\n",
		    invoked_name);
		goto die;
	    }
	    strcpy(out_path, optarg);
	    eb_canonicalize_filename(out_path);
	    if (PATH_MAX < strlen(out_path) + (1 + EB_MAXLEN_BASENAME + 1
		+ 5 + 1 + 2 + 1 + 6 + 1 + 4 + 1 + MAXLEN_IMAGE_SUFFIX)) {
		fprintf(stderr, "%s: too long output directory path\n",
		    invoked_name);
		goto die;
	    }
	    break;

        case 'S':
            /*
             * Option `-S'.  Specify target subbooks.
             */
            if (parse_subname_argument(optarg, &subname_count, subname_list)
		< 0) {
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
	fprintf(stderr, "%s: too many arguments\n", invoked_name);
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
	int i;
	EB_Subbook_Code sub;

	for (i = 0; i < subname_count; i++) {
	    sub = find_subbook(&book, subname_list[i]);
	    if (sub < 0) {
		fprintf(stderr, "%s: unknown subbook name `%s'\n",
		    invoked_name, subname_list[i]);
	    }
	    sub_list[sub_count++] = sub;
	}
    }

    /*
     * Make image files for fonts in the book.
     */
    if (make_book_fonts(&book, out_path, sub_list, sub_count,
	font_list, font_count, image_list, image_count) < 0)
	goto die;

    /*
     * Dispose the book.
     */
    eb_clear(&book);

    return 0;

    /*
     * A critical error occurs...
     */
  die:
    fflush(stderr);
    eb_clear(&book);
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
    const char *arg = argument;
    char fontname[MAXLEN_FONT_NAME + 1], *name;
    EB_Font_Code font_code;
    int i;

    while (*arg != '\0') {
	/*
	 * Take a next element in the argument.
	 */
	for (i = 0, name = fontname;
	     *arg != ',' && *arg != '\0' && i < MAXLEN_FONT_NAME;
	      i++, name++, arg++)
	    *name = *arg;
	*name = '\0';
	if (*arg == ',')
	    arg++;
	else if (*arg != '\0') {
	    fprintf(stderr, "%s: unknown font height `%s...'\n", invoked_name,
		fontname);
	    goto failed;
	}

	/*
	 * Check for the font name.
	 */
	if (strcmp(fontname, "16") == 0)
	    font_code = 16;
	else if (strcmp(fontname, "24") == 0)
	    font_code = 24;
	else if (strcmp(fontname, "30") == 0)
	    font_code = 30;
	else if (strcmp(fontname, "48") == 0)
	    font_code = 48;
	else {
	    fprintf(stderr, "%s: unknown font height `%s'\n", invoked_name,
		fontname);
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
    const char *arg = argument;
    char imagename[MAXLEN_IMAGE_NAME + 1], *name;
    int image_code;
    int i;

    while (*arg != '\0') {
	/*
	 * Take a next element in the argument.
	 */
	for (i = 0, name = imagename;
	     *arg != ',' && *arg != '\0' && i < MAXLEN_IMAGE_NAME;
	      i++, name++, arg++)
	    *name = *arg;
	*name = '\0';
	if (*arg == ',')
	    arg++;
	else if (*arg != '\0') {
	    fprintf(stderr, "%s: unknown image format name `%s...'\n",
		invoked_name, imagename);
	    goto failed;
	}

	/*
	 * Check for the image-format name.
	 */
	for (i = 0; i < MAX_IMAGE_FORMATS; i++) {
	    if (strcasecmp((image_formats + i)->name, imagename) == 0)
		break;
	}
	if (MAX_IMAGE_FORMATS <= i) {
	    fprintf(stderr, "%s: unknown image format name `%s'\n",
		invoked_name, imagename);
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
 * Parse an argument to option `--subbook (-S)'
 */
static int
parse_subname_argument(argument, subname_count, subname_list)
    const char *argument;
    int *subname_count;
    char subname_list[][EB_MAXLEN_BASENAME + 1];
{
    const char *arg = argument;
    char subname[EB_MAXLEN_BASENAME + 1], *name;
    char (*list)[EB_MAXLEN_BASENAME + 1];
    int i;

    while (*arg != '\0') {
	/*
	 * Check current `subname_count'.
	 */
	if (EB_MAX_SUBBOOKS <= *subname_count) {
	    fprintf(stderr, "%s: too many subbooks\n", invoked_name);
	    goto failed;
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
	    goto failed;
	}

	/*
	 * If the font name is not found in `font_list', it is added to
	 * the list.
	 */
	for (i = 0, list = subname_list; i < *subname_count; i++, list++) {
	    if (strcmp(subname, *list) == 0)
		break;
	}
	if (*subname_count <= i) {
	    strcpy(*list, subname);
	    (*subname_count)++;
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
 * Return a subbook-code of the subbook which contains the directory
 * name `dirname'.
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
 * Output version number to standard out.
 */
static void
output_version()
{
    printf("%s (EB Library) version %s\n", program_name, program_version);
    printf("Copyright (c) 1997, 1998, 1999  Motoyuki Kasahara\n\n");
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
 * Output help message to standard out, then exit.
 */
static void
output_help()
{
    printf("Usage: %s [option...] [book-directory]\n", program_name);
    printf("Options:\n");
    printf("  -d  --debug  --verbose     degug mode\n");
    printf("  -f HEIGHT[,HEIGHT...]  --font-height HEIGHT[,HEIGHT...]\n");
    printf("                             generate fonts with HEIGHT; 16, 24, 30 or 48\n");
    printf("                             (default: %s)\n",
	DEFAULT_FONT_HEIGHT);
    printf("  -h  --help                 display this help, then exit\n");
    printf("  -i FORMAT[,FORMAT...]  --image-format FORMAT[,FORMAT...]\n");
    printf("                             generate fonts as FORMAT; xbm, xpm, or gif\n");
    printf("                             (default: %s)\n",
	DEFAULT_IMAGE_FORMAT);
    printf("  -o DIRECTORY  --output-directory DIRECTORY\n");
    printf("                             output fonts under DIRECTORY\n");
    printf("                             (default: %s)\n",
	DEFAULT_OUTPUT_DIRECTORY);
    printf("  -S SUBBOOK[,SUBBOOK...]  --subbook SUBBOOK[,SUBBOOK...]\n");
    printf("                             target subbook\n");
    printf("                             (default: all subboks)\n");
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


/*
 * Make font-files in the `book_path'.
 */
static int
make_book_fonts(book, out_path, sub_list, sub_count, font_list, font_count,
    image_list, image_count)
    EB_Book *book;
    const char *out_path;
    EB_Subbook_Code *sub_list;
    int sub_count;
    EB_Font_Code *font_list;
    int font_count;
    Image_Format_Code *image_list;
    int image_count;
{
    int sub;
    char subpath[PATH_MAX + 1];
    const char *subdir;

    for (sub = 0; sub < sub_count; sub++) {
	/*
	 * Set the current subbook to `sub_list[sub]'.
	 */
	if (eb_set_subbook(book, sub_list[sub]) < 0) {
	    fprintf(stderr, "%s: %s: subbook=%d\n", invoked_name,
		eb_error_message(), sub_list[sub]);
	    goto failed;
	}

	/*
	 * Output debug information.
	 */
	if (debug_flag)
	    fprintf(stderr, "%s: debug: subbook %s:\n", invoked_name, subdir);

	/*
	 * Make a directory for the subbook.
	 */
	subdir = eb_subbook_directory(book);
	if (subdir == NULL) {
	    fprintf(stderr, "%s: %s: subbook=%d\n", invoked_name,
		eb_error_message(), sub_list[sub]);
	    goto failed;
	}
	sprintf(subpath, "%s/%s", out_path, subdir);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(subpath);
#endif
	if (make_missing_directory(subpath, 0777 ^ get_umask()) < 0)
	    goto failed;

	/*
	 * Make fonts in the subbook.
	 */
	if (make_subbook_fonts(book, subpath, font_list, font_count,
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
make_subbook_fonts(book, subpath, font_list, font_count, image_list,
    image_count)
    EB_Book *book;
    const char *subpath;
    EB_Font_Code *font_list;
    int font_count;
    Image_Format_Code *image_list;
    int image_count;
{
    const char *subdir;
    char fontpath[PATH_MAX + 1];
    int font;

    /*
     * Get the current subbook name.
     */
    subdir = eb_subbook_directory(book);
    if (subdir == NULL) {
	fprintf(stderr, "%s: %s\n", invoked_name, eb_error_message());
	goto failed;
    }

    if (debug_flag)
	fprintf(stderr, "%s: debug: subbook %s:\n", invoked_name, subdir);

    for (font = 0; font < font_count; font++) {
	/*
	 * Set the current font to `font_list[font]'.
	 */
	if (!eb_have_font(book, font_list[font]))
	    continue;
	if (eb_set_font(book, font_list[font]) < 0) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d\n", invoked_name,
		eb_error_message(), subdir, font);
	    goto failed;
	}

	/*
	 * Output debug information.
	 */
	if (debug_flag) {
	    fprintf(stderr, "%s: debug: font %d:\n", invoked_name, 
		font_list[font]);
	}

	/*
	 * Make a directory for the font.
	 */
	sprintf(fontpath, "%s/%d", subpath, font_list[font]);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(fontpath);
#endif
	if (make_missing_directory(fontpath, 0777 ^ get_umask()) < 0)
	    goto failed;

	/*
	 * Make font-files with the size.
	 */
	if (make_subbook_size_fonts(book, fontpath, image_list, image_count)
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
make_subbook_size_fonts(book, fontpath, image_list, image_count)
    EB_Book *book;
    const char *fontpath;
    Image_Format_Code *image_list;
    int image_count;
{
    const char *subdir;
    int image;

    /*
     * Get the current subbook name.
     */
    subdir = eb_subbook_directory(book);
    if (subdir == NULL) {
	fprintf(stderr, "%s: %s\n", invoked_name, eb_error_message());
	goto failed;
    }

    for (image = 0; image < image_count; image++) {
	/*
	 * Output debug information.
	 */
	if (debug_flag) {
	    fprintf(stderr, "%s: debug: image %s:\n", invoked_name,
		image_formats[image_list[image]].name);
	}

	/*
	 * Make font-files as the image format.
	 */
	if (make_subbook_size_image_fonts(book, fontpath, image_list[image])
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
 * Make font-files of the current font as the image format.
 */
static int
make_subbook_size_image_fonts(book, imagepath, image)
    EB_Book *book;
    const char *imagepath;
    Image_Format_Code image;
{
    const char *subdir;
    char typepath[PATH_MAX + 1];
    char filename[PATH_MAX + 1];
    char bitmap[EB_SIZE_WIDE_FONT_48];
    char imagedata[EB_SIZE_FONT_IMAGE];
    size_t imagesize;
    int width, height;
    int ch;

    /*
     * Get the current subbook name.
     */
    subdir = eb_subbook_directory(book);
    if (subdir == NULL) {
	fprintf(stderr, "%s: %s\n", invoked_name, eb_error_message());
	goto failed;
    }

    /*
     * Get the current font size.
     */
    height = eb_font(book);
    if (height < 0) {
	fprintf(stderr, "%s: %s: subbook=%s\n", invoked_name,
	    eb_error_message(), subdir);
	goto failed;
    }

    /*
     * Make the bitmap files of narrow fonts.
     */
    if (eb_have_narrow_font(book)) {
	/*
	 * Get narrow font information.
	 */
	width = eb_narrow_font_width(book);
	if (width < 0) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=narrow\n",
		invoked_name, eb_error_message(), subdir, height);
	    goto failed;
	}
	ch = eb_narrow_font_start(book);
	if (ch < 0) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=narrow\n",
		invoked_name, eb_error_message(), subdir, height);
	    goto failed;
	}

	/*
	 * Make a directory for the narrow font.
	 */
	sprintf(typepath, "%s/narrow", imagepath);
#ifdef DOS_FILE_PATH
	eb_canonicalize_filename(typepath);
#endif
	if (make_missing_directory(typepath, 0777 ^ get_umask()) < 0)
	    goto failed;

	while (0 <= ch) {
	    /*
	     * Output debug information.
	     */
	    if (debug_flag) {
		fprintf(stderr, "%s: debug: character %04x\n", invoked_name,
		    ch);
	    }

	    /*
	     * Generate a bitmap file for the character `ch'.
	     */
	    sprintf(filename, "%s/%04x.%s", typepath, ch,
		image_formats[image].suffix);
#ifdef DOS_FILE_PATH
	    eb_canonicalize_filename(filename);
#endif
	    if (eb_narrow_font_character_bitmap(book, ch, bitmap) < 0) {
		fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=narrow, \
character=0x%04x\n",
		    invoked_name, eb_error_message(), subdir, height, ch);
		goto failed;
	    }
	    imagesize = (image_formats[image].function)(imagedata, bitmap,
		width, height);
	    if (save_image_file(filename, imagedata, imagesize) < 0)
		goto failed;

	    /*
	     * Toward next charahacter.
	     */
	    ch = eb_forward_narrow_font_character(book, ch, 1);
	}
    }

    /*
     * Make the bitmap files of wide fonts.
     */
    if (eb_have_wide_font(book)) {
	/*
	 * Get wide font information.
	 */
	width = eb_wide_font_width(book);
	if (width < 0) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=wide\n",
		invoked_name, eb_error_message(), subdir, height);
	    goto failed;
	}
	ch = eb_wide_font_start(book);
	if (ch < 0) {
	    fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=wide\n",
		invoked_name, eb_error_message(), subdir, height);
	    goto failed;
	}

	/*
	 * Make a directory for the wide font.
	 */
	sprintf(typepath, "%s/wide", imagepath);
#ifdef DOS_FILE_PATH
	    eb_canonicalize_filename(typepath);
#endif
	if (make_missing_directory(typepath, 0777 ^ get_umask()) < 0)
	    goto failed;

	while (0 <= ch) {
	    /*
	     * Output debug information.
	     */
	    if (debug_flag) {
		fprintf(stderr, "%s: debug: character %04x\n", invoked_name,
		    ch);
	    }

	    /*
	     * Generate a bitmap file for the character `ch'.
	     */
	    sprintf(filename, "%s/%04x.%s", typepath, ch,
		image_formats[image].suffix);
#ifdef DOS_FILE_PATH
	    eb_canonicalize_filename(filename);
#endif
	    if (eb_wide_font_character_bitmap(book, ch, bitmap) < 0) {
		fprintf(stderr, "%s: %s: subbook=%s, font=%d, type=wide, \
character=0x%04x\n",
		    invoked_name, eb_error_message(), subdir, height, ch);
		goto failed;
	    }
	    imagesize = (image_formats[image].function)(imagedata, bitmap,
		width, height);
	    if (save_image_file(filename, imagedata, imagesize) < 0)
		goto failed;

	    /*
	     * Toward next charahacter.
	     */
	    ch = eb_forward_wide_font_character(book, ch, 1);
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
save_image_file(filename, imagedata, imagesize)
    const char *filename;
    const char *imagedata;
    size_t imagesize;
{
    int file = -1;

#ifdef O_CREAT
    file = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	0666 ^ get_umask());
#else
    file = creat(filename, 0666 ^ get_umask());
#endif

    if (file < 0) {
	fprintf(stderr, "%s: cannot open the file, %s: %s\n", invoked_name,
	    strerror(errno), filename);
	goto failed;
    }
    if (write(file, imagedata, imagesize) != imagesize) {
	fprintf(stderr, "%s: cannot write to the file, %s: %s\n",
	    invoked_name, strerror(errno), filename);
	goto failed;
    }
    if (close(file) < 0) {
	fprintf(stderr, "%s: cannot write to the file, %s: %s\n", invoked_name,
	    strerror(errno), filename);
	file = -1;
	goto failed;
    }

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= file && close(file) < 0) {
	fprintf(stderr, "%s: cannot close the file, %s: %s\n", invoked_name,
	    strerror(errno), filename);
    }

    return -1;
}

