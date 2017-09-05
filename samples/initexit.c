/*                                                            -*- C -*-
 * Copyright (c) 2001
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

/*
 * 使用方法:
 *     initexit
 * 例:
 *     initexit
 * 説明:
 *     EB ライブラリの初期化、後始末をしてみます。
 *     プログラムの外側から見れば、これは何の意味もない動作です。
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
    EB_Error_Code error_code;

    /* EB ライブラリを初期化。*/
    error_code = eb_initialize_library();
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: failed to initialize EB Library, %s: %s\n",
	    argv[0], eb_error_message(error_code), argv[1]);
	exit(1);
    }

    /* EB ライブラリの利用を終了。*/
    eb_finalize_library();
    exit(0);
}
