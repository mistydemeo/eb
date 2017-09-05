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
 *     subbook.c
 *
 * 使用方法:
 *     subbook book-path
 *
 * 例:
 *     subbook /cdrom
 *
 * 説明:
 *     このプログラムは一冊の CD-ROM 書籍に含まれている副本の題名を表
 *     示します。`book-path' は書籍のトップディレクトリ、つまり
 *     CATALOG または CATALOGS ファイルの存在するディレクトリを指すよ
 *     うにします。
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
    EB_Subbook_Code sublist[EB_MAX_SUBBOOKS];
    int subcount;
    const char *title;
    int i;

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
     * 書籍を `book' に結び付ける。
     */
    if (eb_bind(&book, argv[1]) == -1) {
	fprintf(stderr, "%s: failed to bind the book, %s: %s\n",
	    argv[0], eb_error_message(), argv[1]);
	exit(1);
    }

    /*
     * 副本の一覧を取得。
     */
    subcount = eb_subbook_list(&book, sublist);
    if (subcount < 0) {
	fprintf(stderr, "%s: failed to get the subbbook list, %s\n",
	    argv[0], eb_error_message());
	eb_clear(&book);
	exit(1);
    }

    /*
     * 書籍に含まれている副本の題名を出力。
     */
    for (i = 0; i < subcount; i++) {
	title = eb_subbook_title2(&book, sublist[i]);
	if (title == NULL) {
	    fprintf(stderr, "%s: failed to get the title, %s:\n",
		argv[0], eb_error_message());
	    continue;
	}
	printf("%d: %s\n", i, title);
    }

    /*
     * 書籍の利用を終了。
     */
    eb_clear(&book);

    exit(0);
}
