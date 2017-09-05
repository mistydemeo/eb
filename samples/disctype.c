/*                                                            -*- C -*-
 * Copyright (c) 1999, 2000, 01
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
 *     disctype <book-path>
 * 例:
 *     disctype /cdrom
 * 説明:
 *     <book-path> で指定された CD-ROM 書籍の種類が、EB か EPWING か
 *     を調べて表示します。
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
    EB_Book book;
    EB_Disc_Code disc_code;

    /* コマンド行引数をチェック。*/
    if (argc != 2) {
	fprintf(stderr, "Usage: %s book-path\n", argv[0]);
	exit(1);
    }

    /* EB ライブラリと `book' を初期化。*/
    error_code = eb_initialize_library();
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: failed to initialize EB Library, %s: %s\n",
	    argv[0], eb_error_message(error_code), argv[1]);
	goto die;
    }
    eb_initialize_book(&book);

    /* `book' を書籍に結び付ける。失敗したら終了。*/
    error_code = eb_bind(&book, argv[1]);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: failed to bind the book, %s: %s\n",
	    argv[0], eb_error_message(error_code), argv[1]);
	goto die;
    }

    /* 書籍の種類を調べて表示。*/
    error_code = eb_disc_type(&book, &disc_code);
    if (error_code != EB_SUCCESS) {
	fprintf(stderr, "%s: failed to get disc type, %s: %s\n",
	    argv[0], eb_error_message(error_code), argv[1]);
	goto die;
    }

    if (disc_code == EB_DISC_EB) {
	fputs("EB\n", stdout);
    } else if (disc_code == EB_DISC_EPWING) {
	fputs("EPWING\n", stdout);
    } else {
	fputs("unknown\n", stdout);
    }

    /* 書籍と EB ライブラリの利用を終了。*/
    eb_finalize_book(&book);
    eb_finalize_library();
    exit(0);

    /* エラー発生で終了するときの処理。*/
  die:
    eb_finalize_book(&book);
    eb_finalize_library();
    exit(1);
}
