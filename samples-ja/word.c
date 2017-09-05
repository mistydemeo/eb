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
 *     word.c
 *
 * 使用方法:
 *     word book-path word subbook-index
 *
 * 例:
 *     word /cdrom apple 0
 *
 * 説明:
 *     このプログラムは一冊の CD-ROM 書籍のある副本の中から `word'
 *     を探し出します。ヒットしたすべてのエントリの見出しを標準出力
 *     へ出力します。`book-path' は書籍のトップディレクトリ、つまり
 *     CATALOG または CATALOGS ファイルの存在するディレクトリを指す
 *     ようにします。
 *     `subbook-index' は検索対象の副本のインデックスです。書籍の中
 *     の最初の副本のインデックスが `0'、次が `1' になります。
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
#include <eb/text.h>

#define MAX_HITS 50
#define MAXLEN_HEADING 127

int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Book book;
    EB_Subbook_Code sublist[EB_MAX_SUBBOOKS];
    EB_Hit hits[MAX_HITS];
    char heading[MAXLEN_HEADING + 1];
    int subcount;
    int subindex;
    int hitcount;
    size_t len;
    int i;

    /*
     * コマンド行引数をチェック。
     */
    if (argc != 4) {
        fprintf(stderr, "Usage: %s book-path subbook-index word\n",
            argv[0]);
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
        goto failed;
    }

    /*
     * 副本の一覧を取得。
     */
    subcount = eb_subbook_list(&book, sublist);
    if (subcount < 0) {
        fprintf(stderr, "%s: failed to get the subbbook list, %s\n",
            argv[0], eb_error_message());
        goto failed;
    }

    /*
     * 副本のインデックスを取得。
     */
    subindex = atoi(argv[2]);
    if (subindex < 0 || subindex >= subcount) {
        fprintf(stderr, "%s: invalid subbbook-index: %s\n",
            argv[0], argv[2]);
        goto failed;
    }

    /*
     * 「現在の副本 (current subbook)」を設定。
     */
    if (eb_set_subbook(&book, sublist[subindex]) < 0) {
        fprintf(stderr, "%s: failed to set the current subbook, %s\n",
            argv[0], eb_error_message());
        goto failed;
    }

    /*
     * 単語検索のリクエストを送出。
     */
    if (eb_search_word(&book, argv[3]) < 0) {
        fprintf(stderr, "%s: failed to search for the word, %s: %s\n",
            argv[0], eb_error_message(), argv[3]);
        goto failed;
    }

    for (;;) {
        /*
         * 残っているヒットエントリを取得。
         */
        hitcount = eb_hit_list(&book, hits, MAX_HITS);
        if (hitcount == 0) {
            break;
        } else if (hitcount < 0) {
            fprintf(stderr, "%s: failed to get hit entries, %s\n",
                argv[0], eb_error_message());
            goto failed;
        }

        for (i = 0; i < hitcount; i++) {
            /*
             * 見出しの位置へ移動。
             */
            if (eb_seek(&book, &(hits[i].heading)) < 0) {
                fprintf(stderr, "%s: failed to seek the subbook, %s\n",
                    argv[0], eb_error_message());
                goto failed;
            }

            /*
             * 見出しを取得。
             */
            len = eb_heading(&book, NULL, NULL, heading, MAXLEN_HEADING);
            if (len < 0) {
                fprintf(stderr, "%s: failed to read the subbook, %s\n",
                    argv[0], eb_error_message());
                goto failed;
            }

            /*
             * 見出しを出力。
             */
            printf("%s\n", heading);
        }
    }
        
    /*
     * 書籍の利用を終了。
     */
    eb_clear(&book);

    exit(0);

    /*
     * エラーが発生。
     */
  failed:
    eb_clear(&book);
    exit(1);
}
