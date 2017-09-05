/*                                                            -*- C -*-
 * Copyright (c) 1997, 1998, 1999  Motoyuki Kasahara
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

#ifndef EB_DEFS_H
#define EB_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/*
 * Library version.
 */
#define EB_VERSION_MAJOR		2
#define EB_VERSION_MINOR		3

/*
 * Disc code
 */
#define EB_DISC_EB			0
#define EB_DISC_EPWING			1

/*
 * Case of filenames (upper or lower).
 */
#define EB_CASE_UPPER			0
#define EB_CASE_LOWER			1

/*
 * Suffix to be added to filenames (none, `.', or `.;1').
 */
#define EB_SUFFIX_NONE			0
#define EB_SUFFIX_DOT			1
#define EB_SUFFIX_PERIOD		1
#define EB_SUFFIX_VERSION		2
#define EB_SUFFIX_BOTH			3

/*
 * Character codes.
 */
#define EB_CHARCODE_ISO8859_1		1
#define EB_CHARCODE_JISX0208		2
#define EB_CHARCODE_JISX0208_GB2312	3

/*
 * Word types to search.
 */
#define EB_WORD_ALPHA			0
#define EB_WORD_KANA			1
#define EB_WORD_OTHER			2
#define EB_WORD_ERROR			-1

/*
 * Index Style flags.
 */
#define EB_INDEX_STYLE_CONVERT		0
#define EB_INDEX_STYLE_ASIS		1
#define EB_INDEX_STYLE_DELETE		2

/*
 * Zip codes.
 */
#define EB_ZIP_EPWING			-1
#define EB_ZIP_NONE			0
#define EB_ZIP_EBZIP1			1

/*
 * Size and limitation.
 */
#define EB_SIZE_PAGE			2048
#define EB_SIZE_EB_CATALOG		40
#define EB_SIZE_EPWING_CATALOG		164
#define EB_SIZE_EBZIP_HEADER		22
#define EB_SIZE_EBZIP_MARGIN		1024

#define EB_MAXLEN_WORD			255
#define EB_MAXLEN_EB_TITLE		30
#define EB_MAXLEN_EPWING_TITLE		80
#define EB_MAXLEN_TITLE			80
#define EB_MAXLEN_BASENAME		8
#define EB_MAXLEN_LANGNAME		15
#define EB_MAXLEN_MESSAGE		31
#define EB_MAXLEN_MULTI_LABEL		30
#define EB_MAXLEN_ALTERNATION_TEXT	31

#define EB_MAX_FONTS			4
#define EB_MAX_SEARCHES			6
#define EB_MAX_SUBBOOKS			50
#define EB_MAX_LANGUAGES		20
#define EB_MAX_MESSAGES			32
#define EB_MAX_INDEX_DEPTH		6
#define EB_MAX_KEYWORDS			5
#define EB_MAX_MULTI_SEARCHES		8
#define EB_MAX_MULTI_ENTRIES		5
#define EB_MAX_ALTERNATION_CACHE	16
#define EB_MAX_EBZIP_LEVEL		3

/*
 * File and directory names.
 */
#define EB_FILENAME_START		"START"
#define EB_FILENAME_SOUND		"SOUND"
#define EB_FILENAME_CATALOG		"CATALOG"
#define EB_FILENAME_LANGUAGE		"LANGUAGE"
#define EB_FILENAME_VTOC		"VTOC"
#define EB_FILENAME_WELCOME		"WELCOME"
#define EB_FILENAME_CATALOGS		"CATALOGS"
#define EB_FILENAME_HONMON		"HONMON"
#define EB_FILENAME_HONMON2		"HONMON2"
#define EB_FILENAME_APPENDIX		"APPENDIX"
#define EB_FILENAME_FUROKU		"FUROKU"

#define EB_DIRNAME_DATA			"DATA"
#define EB_DIRNAME_GAIJI		"GAIJI"

/*
 * Types for various codes.
 */
typedef int EB_Book_Code;
typedef int EB_Disc_Code;
typedef int EB_Case_Code;
typedef int EB_Suffix_Code;
typedef int EB_Character_Code;
typedef int EB_Font_Code;
typedef int EB_Word_Code;
typedef int EB_Language_Code;
typedef int EB_Message_Code;
typedef int EB_Subbook_Code;
typedef int EB_Index_Style_Code;
typedef int EB_Multi_Search_Code;
typedef int EB_Multi_Entry_Code;
typedef int EB_Zip_Code;

/*
 * EB_Huffman_Node -- Node of static Huffman tree.
 */
typedef struct eb_huffman_node {
    int type;
    int value;
    int frequency;
    struct eb_huffman_node *left;
    struct eb_huffman_node *right;
} EB_Huffman_Node;

/*
 * EB_Zip -- Compression information.
 */
typedef struct {
    EB_Zip_Code code;
    off_t offset;
    off_t file_size;
    size_t slice_size;

    /*
     * The following members are used in EBZIP compression only.
     */
    int zip_level;
    int index_width;
    unsigned int crc;
    time_t mtime;

    /*
     * The following members are used in EPWING compression only.
     */
    off_t index_location;
    size_t index_length;
    off_t frequencies_location;
    size_t frequencies_length;
    EB_Huffman_Node *huffman_nodes;
    EB_Huffman_Node *huffman_root;
} EB_Zip;

/*
 * EB_Alternation_Cache -- An Aternation cache entry.
 */
typedef struct {
    int charno;
    char text[EB_MAXLEN_ALTERNATION_TEXT + 1];
} EB_Alternation_Cache;

/*
 * EB_Appendix_Subbook -- An appendix for a subbook.
 */
typedef struct {
    /*
     * Initialization flag.
     */
    int initialized;

    /*
     * Subbook ID.
     */
    EB_Subbook_Code code;

    /*
     * Directory name.
     */
    char directory[EB_MAXLEN_BASENAME + 1];

    /*
     * File descriptor for the appendix file.
     */
    int sub_file;

    /*
     * Character code of the book.
     */
    EB_Character_Code char_code;

    /*
     * Start number of the character code in the narrow/wide font.
     */
    int narw_start;
    int wide_start;

    /*
     * End number of the character code in the narrow/wide font.
     */
    int narw_end;
    int wide_end;

    /*
     * Start page number of the narrow/wide font.
     */
    int narw_page;
    int wide_page;

    /*
     * Stop code.
     */
    int stop0;
    int stop1;

    /*
     * Compression Information.
     */
    EB_Zip zip;
} EB_Appendix_Subbook;

/*
 * EB_Appendix -- Additional resources for a book.
 */
typedef struct {
    /*
     * Path of the book.
     */
    char *path;

    /*
     * The length of the path.
     */
    size_t path_length;

    /*
     * Disc type.  EB(EB/EBG/EBXA) or EPWING.
     */
    EB_Disc_Code disc_code;

    /*
     * Cases of the filenames; upper or lower.
     */
    EB_Case_Code case_code;

    /*
     * Suffix to be added to filenames. (None, ".", or ".;1")
     */
    EB_Suffix_Code suffix_code;

    /*
     * The number of subbooks the book has.
     */
    int sub_count;

    /*
     * Subbook list.
     */
    EB_Appendix_Subbook *subbooks;

    /*
     * Current subbook.
     */
    EB_Appendix_Subbook *sub_current;

    /*
     * Cache table for alternation text.
     */
    EB_Alternation_Cache narw_cache[EB_MAX_ALTERNATION_CACHE];
    EB_Alternation_Cache wide_cache[EB_MAX_ALTERNATION_CACHE];
} EB_Appendix;

/*
 * EB_Font -- A font in a subbook.
 */
typedef struct {
    /* 
     * Width and height.
     */
    int width;
    int height;

    /*
     * Character numbers of the start and end of the font.
     */
    int start;
    int end;

    /*
     * Page number of the start page of the font data.
     * (EB/EBG/EBXA only.  In EPWING, it is alyways 1).
     */
    int page;

    /*
     * File descriptor of the font file. (EPWING only)
     */
    int font_file;

    /*
     * Filename of the font. (EPWING only)
     */
    char filename[EB_MAXLEN_BASENAME + 1];

    /*
     * Compression Information.
     */
    EB_Zip zip;
} EB_Font;

/*
 * EB_Language -- A language in a book. (EB/EBG/EBXA only)
 */
typedef struct {
    /*
     * Language ID.
     */
    EB_Language_Code code;

    /*
     * Offset of the messages in the language file.
     */
    off_t offset;

    /*
     * The number of messages the language file has.
     */
    int msg_count;

    /*
     * Language name.
     */
    char name[EB_MAXLEN_LANGNAME + 1];
} EB_Language;

/*
 * EB_Multi_Entry -- A multi-search entry in a subbook.
 */
typedef struct {
    /*
     * Page number of the start page of a multi search table.
     */
    char label[EB_MAXLEN_MULTI_LABEL + 1];

    /*
     * The top page of search methods.
     */
    int page_word_asis;
    int page_endword_asis;
    int page_keyword;
} EB_Multi_Entry;

/*
 * EB_Search -- Search methods in a subbook.
 */
typedef struct {
    /*
     * Page number of the start page of an index.
     */
    int page;

    /*
     * Length of pages.
     */
    int length;

    /*
     * Index style flags.
     */
    EB_Index_Style_Code katakana;
    EB_Index_Style_Code lower;
    EB_Index_Style_Code mark;
    EB_Index_Style_Code long_vowel;
    EB_Index_Style_Code double_consonant;
    EB_Index_Style_Code contracted_sound;
    EB_Index_Style_Code voiced_consonant;
    EB_Index_Style_Code small_vowel;
    EB_Index_Style_Code p_sound;
    EB_Index_Style_Code space;

    /*
     * The number of entries the multi search has.
     */
    int entry_count;

    /*
     * List of Word entry information.
     */
    EB_Multi_Entry entries[EB_MAX_MULTI_ENTRIES];
} EB_Search;

/*
 * EB_Subbook -- A subbook in the book.
 */
typedef struct {
    /*
     * Whether information about the subbook have been loaded in the
     * structure.
     */
    int initialized;

    /*
     * Index page;
     */
    int index_page;

    /*
     * Subbook ID.
     */
    EB_Subbook_Code code;

    /*
     * File descriptor for the subbook file.
     */
    int sub_file;

    /*
     * Title of the subbook.
     */
    char title[EB_MAXLEN_TITLE + 1];

    /*
     * Directory name.
     */
    char directory[EB_MAXLEN_BASENAME + 1];

    /*
     * The top page of search methods.
     */
    EB_Search word_alpha;
    EB_Search word_asis;
    EB_Search word_kana;
    EB_Search endword_alpha;
    EB_Search endword_asis;
    EB_Search endword_kana;
    EB_Search keyword;
    EB_Search menu;
    EB_Search graphic;
    EB_Search copyright;
    EB_Search multi[EB_MAX_MULTI_SEARCHES];

    /*
     * The number of multi-search types the subbook has.
     */
    int multi_count;

    /*
     * The number of fonts the subbook has.
     */
    int font_count;

    /*
     * Font list.
     */
    EB_Font fonts[EB_MAX_FONTS * 2];

    /*
     * Current narrow and wide fonts.
     */
    EB_Font *narw_current;
    EB_Font *wide_current;

    /*
     * Compression Information.
     */
    EB_Zip zip;
} EB_Subbook;

/*
 * EB_Book -- A book.
 */
typedef struct {
    /*
     * Book ID.
     */
    EB_Book_Code code;

    /*
     * Disc type.  EB(EB/EBG/EBXA) or EPWING.
     */
    EB_Disc_Code disc_code;

    /*
     * Character code of the book.
     */
    EB_Character_Code char_code;

    /*
     * Path of the book.
     */
    char *path;

    /*
     * The length of the path.
     */
    size_t path_length;

    /*
     * Cases of the filenames; upper or lower.
     */
    EB_Case_Code case_code;

    /*
     * Suffix to be added to filenames. (None, ".", or ".;1")
     */
    EB_Suffix_Code suffix_code;

    /*
     * The number of subbooks the book has.
     */
    int sub_count;

    /*
     * Subbook list.
     */
    EB_Subbook *subbooks;

    /*
     * Current subbook.
     */
    EB_Subbook *sub_current;

    /*
     * The number of languages the book supports.
     */
    int lang_count;

    /*
     * Language list.
     */
    EB_Language *languages;

    /*
     * Current language.
     */
    EB_Language *lang_current;

    /*
     * Messages in the current language.
     */
    char *messages;
} EB_Book;

/*
 * EB_Position -- A pair of page and offset.
 */
typedef struct {
    int page;
    int offset;
} EB_Position;

/*
 * EB_Hit -- In a word search, heading and text locations of a matched
 * entry are stored.
 */
typedef struct {
    EB_Position heading;
    EB_Position text;
} EB_Hit;

#ifdef __cplusplus
}
#endif

#endif /* not EB_DEFS_H */

