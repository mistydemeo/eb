/*                                                            -*- C -*-
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
 *   AC_C_CONST
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * If the files `filename1' and `filename2' are identical (i.e. the are
 * written on a same device and a same i-node is assigned to them), 1 is
 * returned.  Otherwise, 0 is returned.
 */
int
is_same_file(filename1, filename2)
    const char *filename1;
    const char *filename2;
{
    struct stat st1, st2;

    if (stat(filename1, &st1) || stat(filename2, &st2) != 0)
	return 0;

#ifndef DOS_FILE_PATH
    if (st1.st_dev != st2.st_dev || st1.st_ino != st2.st_ino)
	return 0;

#else /* DOS_FILE_PATH */
    /* Can't rely on st_ino and st_dev, use other fields: */
    if (st1.st_mode != st2.st_mode
	|| st1.st_uid != st2.st_uid
	|| st1.st_gid != st2.st_gid
	|| st1.st_size != st2.st_size
	|| st1.st_atime != st2.st_atime
	|| st1.st_mtime != st2.st_mtime
	|| st1.st_ctime != st2.st_ctime)
	return 0;
#endif /* DOS_FILE_PATH */

    return 1;
}
