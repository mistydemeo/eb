/*                                                            -*- C -*-
 * Copyright (c) 2002
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
 *     font <book-path> <subbook-index>
 * 例:
 *     font /cdrom 0
 * 説明:
 *     <book-path> で指定した CD-ROM 書籍から特定の副本を選び、その
 *     副本が定義している半角外字 (高さ 16 ピクセル) をすべてアスキー
 *     アートで表示します。
 *
 *     その副本が、高さ 16 ピクセルの半角外字を定義していないと、エ
 *     ラーになります。
 *
 *     <subbook-index> には、検索対象の副本のインデックスを指定しま
 *     す。インデックスは、書籍の最初の副本から順に 0、1、2 ... に
 *     なります。
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
#include <eb/font.h>

int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Error_Code error_code;
    EB_Book book;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    int subbook_count;
    int subbook_index;
    int font_start;
    unsigned char bitmap[EB_SIZE_NARROW_FONT_16];
    int i, j;

    /* コマンド行引数をチェック。*/
    if (argc != 3) {
        fprintf(stderr, "Usage: %s book-path subbook-index\n",
            argv[0]);
        exit(1);
    }

    /* EB ライブラリと `book' を初期化。*/
    eb_initialize_library();
    eb_initialize_book(&book);

    /* 書籍を `book' に結び付ける。*/
    error_code = eb_bind(&book, argv[1]);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: failed to bind the book, %s: %s\n",
            argv[0], eb_error_message(error_code), argv[1]);
        goto die;
    }

    /* 副本の一覧を取得。*/
    error_code = eb_subbook_list(&book, subbook_list, &subbook_count);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: failed to get the subbbook list, %s\n",
            argv[0], eb_error_message(error_code));
        goto die;
    }

    /* 副本のインデックスを取得。*/
    subbook_index = atoi(argv[2]);

    /*「現在の副本 (current subbook)」を設定。*/
    if (eb_set_subbook(&book, subbook_list[subbook_index]) < 0) {
        fprintf(stderr, "%s: failed to set the current subbook, %s\n",
            argv[0], eb_error_message(error_code));
        goto die;
    }

    /*「現在の外字の大きさ」を設定。*/
    if (eb_set_font(&book, EB_FONT_16) < 0) {
        fprintf(stderr, "%s: failed to set the font size, %s\n",
            argv[0], eb_error_message(error_code));
        goto die;
    }

    /* 外字の開始位置を取得。*/
    error_code = eb_narrow_font_start(&book, &font_start);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: failed to get font information, %s\n",
            argv[0], eb_error_message(error_code));
        goto die;
    }

    i = font_start;
    for (;;) {
        /* 外字のビットマップデータを取得。*/
	error_code = eb_narrow_font_character_bitmap(&book, i,
	    (char *)bitmap);
	if (error_code != EB_SUCCESS) {
            fprintf(stderr, "%s: failed to get font data, %s\n",
                argv[0], eb_error_message(error_code));
            goto die;
        }

	/* ビットマップをアスキーアートにして出力。*/
	printf("code point=%04x\n", i);
	for (j = 0; j < 16; j++) {
	    fputc((bitmap[j] & 0x80) ? '*' : ' ', stdout);
	    fputc((bitmap[j] & 0x40) ? '*' : ' ', stdout);
	    fputc((bitmap[j] & 0x20) ? '*' : ' ', stdout);
	    fputc((bitmap[j] & 0x10) ? '*' : ' ', stdout);
	    fputc((bitmap[j] & 0x08) ? '*' : ' ', stdout);
	    fputc((bitmap[j] & 0x04) ? '*' : ' ', stdout);
	    fputc((bitmap[j] & 0x02) ? '*' : ' ', stdout);
	    fputc((bitmap[j] & 0x01) ? '*' : ' ', stdout);
	    fputc('\n', stdout);
	}
	fputs("--------\n", stdout);

        /* 外字の文字番号を一つ進める。*/
	error_code = eb_forward_narrow_font_character(&book, 1, &i);
	if (error_code != EB_SUCCESS)
	    break;
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
