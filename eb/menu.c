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

#include "eb.h"
#include "error.h"


/*
 * Examine whether the current subbook in `book' supports `MENU SEARCH'
 * or not.
 */
int
eb_have_menu(book)
    EB_Book *book;
{
    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return 0;
    }

    if (book->sub_current->menu.page == 0) {
	eb_error = EB_ERR_NO_SUCH_SEARCH;
	return 0;
    }

    return 1;
}


/*
 * Menu.
 */
int
eb_menu(book, position)
    EB_Book *book;
    EB_Position *position;
{
    int page;

    /*
     * Current subbook must have been set.
     */
    if (book->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_SUB;
	return -1;
    }

    /*
     * Check for the page number of MENU SEARCH.
     */
    page = book->sub_current->menu.page;
    if (page == 0) {
	eb_error = EB_ERR_NO_SUCH_SEARCH;
	return -1;
    }

    position->page = page;
    position->offset = 0;

    return 0;
}


