/*
 * Copyright (c) 1997  Motoyuki Kasahara
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
#include "appendix.h"
#include "error.h"
#include "text.h"

/*
 * Examine whether the current subbook in `appendix' has a stop-code.
 */
int
eb_have_stopcode(appendix)
    EB_Appendix *appendix;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return 0;
    }

    return (appendix->sub_current->stop0 != 0);
}


/*
 * Return the stop-code of the current subbook in `appendix'.
 */
int
eb_stopcode(appendix)
    EB_Appendix *appendix;
{
    /*
     * Current subbook must have been set.
     */
    if (appendix->sub_current == NULL) {
	eb_error = EB_ERR_NO_CUR_APPSUB;
	return -1;
    }

    return (appendix->sub_current->stop0 << 16) + appendix->sub_current->stop1;
}


