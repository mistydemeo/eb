/*
 * Copyright (c) 1997, 2000, 01  
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
#include "appendix.h"
#include "text.h"
#include "build-post.h"

/*
 * Examine whether the current subbook in `appendix' has a stop-code.
 */
int
eb_have_stop_code(appendix)
    EB_Appendix *appendix;
{
    eb_lock(&appendix->lock);
    LOG(("in: eb_have_stop_code(appendix=%d)", (int)appendix->code));

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL)
	goto failed;

    if (appendix->subbook_current->stop_code0 == 0)
	goto failed;

    LOG(("out: eb_have_stop_code() = %d", 1));
    eb_unlock(&appendix->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
    LOG(("out: eb_have_stop_code() = %d", 0));
    eb_unlock(&appendix->lock);
    return 0;
}


/*
 * Return the stop-code of the current subbook in `appendix'.
 */
EB_Error_Code
eb_stop_code(appendix, stop_code)
    EB_Appendix *appendix;
    int *stop_code;
{
    EB_Error_Code error_code;

    eb_lock(&appendix->lock);
    LOG(("in: eb_stop_code(appendix=%d)", (int)appendix->code));

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    if (appendix->subbook_current->stop_code0 == 0) {
	error_code = EB_ERR_NO_STOPCODE;
	goto failed;
    }

    stop_code[0] = appendix->subbook_current->stop_code0;
    stop_code[1] = appendix->subbook_current->stop_code1;

    LOG(("out: eb_stop_code(stop_code=%d,%d) = %s",
	stop_code[0], stop_code[1], eb_error_string(EB_SUCCESS)));
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    stop_code[0] = -1;
    stop_code[1] = -1;
    LOG(("out: eb_stop_code() = %s", eb_error_string(error_code)));
    eb_unlock(&appendix->lock);
    return error_code;
}


