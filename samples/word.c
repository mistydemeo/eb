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
 *     word.c
 *
 * Usage:
 *     word book-path word subbook-index
 *
 * Example:
 *     word /cdrom apple 0
 *
 * Description:
 *     This program searches a subbook in a book for `word'.
 *     It outputs headings of all hit entires to standard out.
 *     `book-path' points to the top directory of the CD-ROM book
 *     where the file CATALOG or CATALOGS resides.
 *     `subbook-index' is the index of the subbook to be searched.
 *     The index of the first subbook in a book is `0', and the
 *     next is `1'.
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
#include <eb/text.h>

#define MAX_HITS 50
#define MAXLEN_HEADING 127

int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Book book;
    EB_Subbook_Code sublist[EB_MAX_SUBBOOKS];
    EB_Hit hits[MAX_HITS];
    char heading[MAXLEN_HEADING + 1];
    int subcount;
    int subindex;
    int hitcount;
    size_t len;
    int i;

    /*
     * Check for command line arguments.
     */
    if (argc != 4) {
        fprintf(stderr, "Usage: %s book-path subbook-index word\n",
            argv[0]);
        exit(1);
    }

    /*
     * Initialize `book'.
     */
    eb_initialize(&book);

    /*
     * Bind a book to `book'.
     */
    if (eb_bind(&book, argv[1]) == -1) {
        fprintf(stderr, "%s: failed to bind the book, %s: %s\n",
            argv[0], eb_error_message(), argv[1]);
        goto failed;
    }

    /*
     * Get the subbook list.
     */
    subcount = eb_subbook_list(&book, sublist);
    if (subcount < 0) {
        fprintf(stderr, "%s: failed to get the subbbook list, %s\n",
            argv[0], eb_error_message());
        goto failed;
    }

    /*
     * Get the subbook-index.
     */
    subindex = atoi(argv[2]);
    if (subindex < 0 || subindex >= subcount) {
        fprintf(stderr, "%s: invalid subbbook-index: %s\n",
            argv[0], argv[2]);
        goto failed;
    }

    /*
     * Set the current subbook.
     */
    if (eb_set_subbook(&book, sublist[subindex]) < 0) {
        fprintf(stderr, "%s: failed to set the current subbook, %s\n",
            argv[0], eb_error_message());
        goto failed;
    }

    /*
     * Send a word search request.
     */
    if (eb_search_word(&book, argv[3]) < 0) {
        fprintf(stderr, "%s: failed to search for the word, %s: %s\n",
            argv[0], eb_error_message(), argv[3]);
        goto failed;
    }

    for (;;) {
        /*
         * Get remained hit entries.
         */
        hitcount = eb_hit_list(&book, hits, MAX_HITS);
        if (hitcount == 0) {
            break;
        } else if (hitcount < 0) {
            fprintf(stderr, "%s: failed to get hit entries, %s\n",
                argv[0], eb_error_message());
            goto failed;
        }

        for (i = 0; i < hitcount; i++) {
            /*
             * Seek to the location where a heading resides.
             */
            if (eb_seek(&book, &(hits[i].heading)) < 0) {
                fprintf(stderr, "%s: failed to seek the subbook, %s\n",
                    argv[0], eb_error_message());
                goto failed;
            }

            /*
             * Get the heading.
             */
            len = eb_heading(&book, NULL, NULL, heading, MAXLEN_HEADING);
            if (len < 0) {
                fprintf(stderr, "%s: failed to read the subbook, %s\n",
                    argv[0], eb_error_message());
                goto failed;
            }

            /*
             * Output the heading.
             */
            printf("%s\n", heading);
        }
    }
        
    /*
     * Clear the book.
     */
    eb_clear(&book);

    exit(0);

    /*
     * Error occurs.
     */
  failed:
    eb_clear(&book);
    exit(1);
}
