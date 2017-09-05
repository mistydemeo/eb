/*
 * Copyright (c) 1997, 2000  Motoyuki Kasahara
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

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "appendix.h"
#include "text.h"

/*
 * Examine whether the current subbook in `appendix' has a stop-code.
 */
int
eb_have_stop_code(appendix)
    EB_Appendix *appendix;
{
    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL)
	goto failed;

    if (appendix->subbook_current->stop0 == 0)
	goto failed;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return 1;

    /*
     * An error occurs...
     */
  failed:
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

    /*
     * Lock the appendix.
     */
    eb_lock(&appendix->lock);

    /*
     * Current subbook must have been set.
     */
    if (appendix->subbook_current == NULL) {
	error_code = EB_ERR_NO_CUR_APPSUB;
	goto failed;
    }

    *stop_code = (appendix->subbook_current->stop0 << 16)
	+ appendix->subbook_current->stop1;

    /*
     * Unlock the appendix.
     */
    eb_unlock(&appendix->lock);

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *stop_code = -1;
    eb_unlock(&appendix->lock);
    return error_code;
}


