/*
 * Copyright (c) 1999  Motoyuki Kasahara
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
 * ファイル名:
 *     disctype.c
 *
 * 使用方法:
 *     disctype book-path
 *
 * 例:
 *     disctype /cdrom
 *
 * 説明:
 *     このプログラムは一冊の CD-ROM 書籍の種類 (EB/EBG/EBXA か EPWING
 *     か) を表示します。`book-path' は書籍のトップディレクトリ、つま
 *     り CATALOG または CATALOGS ファイルの存在するディレクトリを指す
 *     ようにします。
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
    EB_Book book;
    EB_Disc_Code disc_code;

    /*
     * コマンド行引数をチェック。
     */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s book-path\n", argv[0]);
	exit(1);
    }

    /*
     * `book' を初期化。
     */
    eb_initialize(&book);

    /*
     * 書籍に結び付ける。失敗したら終了する。
     */
    if (eb_bind(&book, argv[1]) == -1) {
	fprintf(stderr, "%s: failed to bind the book: %s\n",
	    argv[0], argv[1]);
	exit(1);
    }

    /*
     * 書籍の種類を表示。
     */
    disc_code = eb_disc_type(&book);
    fputs("disc type: ", stdout);
    if (disc_code == EB_DISC_EB) {
	fputs("EB/EBG/EBXA", stdout);
    } else if (disc_code == EB_DISC_EPWING) {
	fputs("WPING", stdout);
    } else {
	fputs("unknown", stdout);
    }
    fputc('\n', stdout);

    /*
     * 書籍の利用を終了。
     */
    eb_clear(&book);

    exit(0);
}
