/*
 * Copyright (c) 2000, 01  
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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "internal.h"

/*
 * Initialize the library.
 */
EB_Error_Code
eb_initialize_library()
{
    EB_Error_Code error_code;

    eb_initialize_default_hookset();
#ifdef ENABLE_NLS
    bindtextdomain(EB_TEXT_DOMAIN_NAME, LOCALEDIR);
#endif
    if (zio_initialize_library() < 0) {
	error_code = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
}


/*
 * Finalize the library.
 */
void
eb_finalize_library()
{
    zio_finalize_library();
}
