/*
 * Copyright (c) 1999  Motoyuki Kasahara
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

/*
 * Filename:
 *     disctype.c
 *
 * Usage:
 *     disctype book-path
 *
 * Example:
 *     disctype /cdrom
 *
 * Description:
 *     This program shows disc type (EB/EBG/EBXA or EPWING) of a
 *     CD-ROM book.  `book-path' points to the top directory of
 *     the CD-ROM book where the file CATALOG or CATALOGS resides.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <eb/eb.h>
#include <eb/error.h>

int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Book book;
    EB_Disc_Code disc_code;

    /*
     * Check for command line arguments.
     */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s book-path\n", argv[0]);
	exit(1);
    }

    /*
     * Initialize `book'.
     */
    eb_initialize(&book);

    /*
     * Bind a book.  Exit if it fails.
     */
    if (eb_bind(&book, argv[1]) == -1) {
	fprintf(stderr, "%s: failed to bind the book: %s\n",
	    argv[0], argv[1]);
	exit(1);
    }

    /*
     * Show disc type.
     */
    disc_code = eb_disc_type(&book);
    fputs("disc type: ", stdout);
    if (disc_code == EB_DISC_EB) {
	fputs("EB/EBG/EBXA", stdout);
    } else if (disc_code == EB_DISC_EPWING) {
	fputs("WPING", stdout);
    } else {
	fputs("unknown", stdout);
    }
    fputc('\n', stdout);

    /*
     * Clear the book.
     */
    eb_clear(&book);

    exit(0);
}
