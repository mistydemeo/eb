/*
 * Copyright (c) 1997, 98, 2000, 01  
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

#include "build-pre.h"
#include "eb.h"
#include "error.h"
#include "build-post.h"

/*
 * Examine whether the current subbook in `book' have a copyright
 * notice or not.
 */
int
eb_have_copyright(book)
    EB_Book *book;
{
    eb_lock(&book->lock);
    LOG(("in: eb_have_copyright(book=%d)", (int)book->code));

    /*
     * Check for the current status.
     */
    if (book->subbook_current == NULL)
	goto failed;

    /*
     * Check for the index page of copyright notice.
     */
    if (book->subbook_current->copyright.start_page == 0)
	goto failed;

    LOG(("out: eb_have_copyright() = %d", 1));
    eb_unlock(&book->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_have_copyright() = %d", 0));
    eb_unlock(&book->lock);
    return 0;
}


/*
 * Get a position of copyright notice.
 */
EB_Error_Code
eb_copyright(book, position)
    EB_Book *book;
    EB_Position *position;
{
    EB_Error_Code error_code;
    int page;

    eb_lock(&book->lock);
    LOG(("in: eb_copyright(book=%d)", (int)book->code));

    /*
     * Check for the current status.
     */
    if (book->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_SUB;
	goto failed;
    }

    /*
     * Check for the page number of COPYRIGHT NOTICE.
     */
    page = book->subbook_current->copyright.start_page;
    if (page == 0) {
	error_code = EB_ERR_NO_SUCH_SEARCH;
	goto failed;
    }

    /*
     * Copy the position to `position'.
     */
    position->page = page;
    position->offset = 0;

    LOG(("out: eb_copyright(position={%d,%d}) = %s",
	position->page, position->offset, eb_error_string(EB_SUCCESS)));
    eb_unlock(&book->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_copyright() = %s", eb_error_string(error_code)));
    eb_unlock(&book->lock);
    return error_code;
}


