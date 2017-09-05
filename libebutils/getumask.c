/*
 * Copyright (c) 1998  Motoyuki Kasahara
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
 * Requirements for Autoconf:
 *   AC_TYPE_MODE_T
 *   AC_CHECK_HEADERS(unistd.h)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*
 * Get the current umask.
 */
mode_t
get_umask()
{
    mode_t um;

    um = umask(022);
    umask(um);

    return um;
}
