/*                                                            -*- C -*-
 * Copyright (c) 1997, 98, 99, 2000  Motoyuki Kasahara
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

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

/*
 * Library version.
 */
#define EB_VERSION_MAJOR		3
#define EB_VERSION_MINOR		0

/*
 * Disc code
 */
#define EB_DISC_EB			0
#define EB_DISC_EPWING			1
#define EB_DISC_INVALID			-1

/*
 * Character codes.
 */
#define EB_CHARCODE_ISO8859_1		1
#define EB_CHARCODE_JISX0208		2
#define EB_CHARCODE_JISX0208_GB2312	3
#define EB_CHARCODE_INVALID		-1

/*
 * Special book ID for cache to represent "no cache data for any book".
 */
#define EB_BOOK_NONE			-1

/*
 * Special disc code, subbook code, multi search ID, and multi search
 * entry ID, for representing error state.
 */
#define EB_SUBBOOK_INVALID		-1
#define EB_MULTI_INVALID		-1
#define EB_MULTI_ENTRY_INVALID		-1

/*
 * Size of a page (The term `page' means `block' in JIS X 4081).
 */
#define EB_SIZE_PAGE			2048

/*
 * Data size of a book entry in a catalog file.
 */
#define EB_SIZE_EB_CATALOG		40

/*
 * Header size of the ebzip compression file.
 */
#define EB_SIZE_EPWING_CATALOG		164

/*
 * Header size of the ebzip compression file.
 */
#define EB_SIZE_EBZIP_HEADER		22

/*
 * Margin size for ebzip compression buffer.
 * (Since compressed data is larger than original in the worst case,
 * we must add margin to a compression buffer.)
 */
#define EB_SIZE_EBZIP_MARGIN		1024

/*
 * The maximum length of a word to be searched.
 */
#define EB_MAX_WORD_LENGTH		255

/*
 * The maximum length of an EB* book title.
 */
#define EB_MAX_EB_TITLE_LENGTH		30

/*
 * The maximum length of an EPWING book title.
 */
#define EB_MAX_EPWING_TITLE_LENGTH	80

/*
 * The maximum length of a book title.
 */
#define EB_MAX_TITLE_LENGTH		80

/*
 * The maximum length of a directory name.
 */
#define EB_MAX_DIRECTORY_NAME_LENGTH	8

/*
 * The maximum length of a file name under a certain directory.
 * prefix(8 chars) + '.' + suffix(3 chars) + ';' + digit(1 char)
 */
#define EB_MAX_FILE_NAME_LENGTH		14

/*
 * The maximum length of a language name.
 */
#define EB_MAX_LANGUAGE_NAME_LENGTH	15

/*
 * The maximum length of a message.
 */
#define EB_MAX_MESSAGE_LENGTH		31

/*
 * The maximum length of a label for multi-search entry.
 */
#define EB_MAX_MULTI_LABEL_LENGTH	30

/*
 * The maximum number of font heights that a subbok supports.
 */
#define EB_MAX_ALTERNATION_TEXT_LENGTH	31

/*
 * The maximum number of font heights in a subbok.
 */
#define EB_MAX_FONTS			4

/*
 * The maximum number of subbooks in a book.
 */
#define EB_MAX_SUBBOOKS			50

/*
 * The maximum number of languages.
 */
#define EB_MAX_LANGUAGES		20

/*
 * The maximum number of messages in a languages.
 */
#define EB_MAX_MESSAGES			32

/*
 * The maximum number of entries in a keyword search.
 */
#define EB_MAX_KEYWORDS			5

/*
 * The maximum number of multi-search types in a subbook.
 */
#define EB_MAX_MULTI_SEARCHES		8

/*
 * The maximum number of entries in a multi-search.
 */
#define EB_MAX_MULTI_ENTRIES		5

/*
 * The maximum number of characters for alternation cache.
 */
#define EB_MAX_ALTERNATION_CACHE	16

/*
 * Maximum length of a text work buffer.
 */
#define EB_MAX_TEXT_WORK_LENGTH		255

/*
 * The number of text hooks.
 */
#define EB_NUMBER_OF_HOOKS		32

/*
 * The number of search contexts required by a book.
 */
#define EB_NUMBER_OF_SEARCH_CONTEXTS	5

/*
 * Trick for function protypes.
 */
#ifndef EB_P
#if defined(__STDC__) || defined(__cplusplus)
#define EB_P(p) p
#else /* not __STDC__ && not __cplusplus */
#define EB_P(p) ()
#endif /* not __STDC__ && not __cplusplus */
#endif /* EB_P */

/*
 * Types for various codes.
 */
typedef int EB_Error_Code;
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
typedef int EB_Search_Code;
typedef int EB_Text_Code;
typedef int EB_Multi_Search_Code;
typedef int EB_Multi_Entry_Code;
typedef int EB_Zip_Code;
typedef int EB_Hook_Code;

/*
 * Typedef for Structures.
 */
#ifdef ENABLE_PTHREAD
typedef struct EB_Lock_Struct              EB_Lock;
#endif
typedef struct EB_Position_Struct          EB_Position;
typedef struct EB_Huffman_Node_Struct      EB_Huffman_Node;
typedef struct EB_Zip_Struct               EB_Zip;
typedef struct EB_Alternation_Cache_Struct EB_Alternation_Cache;
typedef struct EB_Appendix_Subbook_Struct  EB_Appendix_Subbook;
typedef struct EB_Appendix_Struct          EB_Appendix;
typedef struct EB_Font_Struct              EB_Font;
typedef struct EB_Language_Struct          EB_Language;
typedef struct EB_Search_Struct            EB_Search;
typedef struct EB_Multi_Search_Struct      EB_Multi_Search;
typedef struct EB_Subbook_Struct           EB_Subbook;
typedef struct EB_Text_Context_Struct      EB_Text_Context;
typedef struct EB_Search_Context_Struct    EB_Search_Context;
typedef struct EB_Book_Struct              EB_Book;
typedef struct EB_Hit_Struct               EB_Hit;
typedef struct EB_Hook_Struct              EB_Hook;
typedef struct EB_Hookset_Struct           EB_Hookset;

/*
 * Pthreads lock.
 */
#ifdef ENABLE_PTHREAD
struct EB_Lock_Struct {
    /*
     * Lock count.  (For emulating recursive lock).
     */
    int lock_count;

    /*
     * Mutex for `lock_count'.
     */
    pthread_mutex_t lock_count_mutex;

    /*
     * Mutex for struct entity.
     */
    pthread_mutex_t entity_mutex;
};
#endif /* ENABLE_PTHREAD */

/*
 * A pair of page and offset.
 */
struct EB_Position_Struct {
    /*
     * Page. (1, 2, 3 ...)
     */
    int page;

    /*
     * Offset in `page'. (0 ... 2047)
     */
    int offset;
};

/*
 * A node of static Huffman tree.
 */
struct EB_Huffman_Node_Struct {
    /*
     * node type (ITNERMEDIATE, LEAF8, LEAF16, LEAF32 or EOF).
     */
    int type;

    /*
     * Value of a leaf node.
     */
    unsigned int value;

    /*
     * Frequency of a node.
     */
    int frequency;

    /*
     * Left child.
     */
    EB_Huffman_Node *left;

    /*
     * Right child.
     */
    EB_Huffman_Node *right;
};

/*
 * Compression information of a book.
 */
struct EB_Zip_Struct {
    /*
     * Zip type. (NONE, EPWING or EBZIP)
     */
    EB_Zip_Code code;

    /*
     * Current location.
     */
    off_t location;

    /*
     * Size of an Uncopressed file.
     */
    off_t file_size;

    /*
     * Slice size of an EBZIP compressed file.
     */
    size_t slice_size;

    /*
     * Compression level. (EBZIP compression only)
     */
    int zip_level;

    /*
     * Length of an index. (EBZIP compression only)
     */
    int index_width;

    /*
     * Adler-32 check sum of an uncompressed file. (EBZIP compression only)
     */
    unsigned int crc;

    /*
     * mtime of an uncompressed file. (EBZIP compression only)
     */
    time_t mtime;

    /*
     * Location of an index table. (EPWING and S-EBXA compression only)
     */
    off_t index_location;

    /*
     * Length of an index table. (EPWING and S-EBXA compression only)
     */
    size_t index_length;

    /*
     * Location of a frequency table. (EPWING compression only)
     */
    off_t frequencies_location;

    /*
     * Length of a frequency table. (EPWING compression only)
     */
    size_t frequencies_length;

    /*
     * Huffman tree nodes. (EPWING compression only)
     */
    EB_Huffman_Node *huffman_nodes;

    /*
     * Root node of a Huffman tree. (EPWING compression only)
     */
    EB_Huffman_Node *huffman_root;

    /*
     * Region of compressed pages. (S-EBXA compression only)
     */
    off_t zip_start_location;
    off_t zip_end_location;

    /*
     * Add this value to offset written in index. (S-EBXA compression only)
     */
    off_t index_base;
};

/*
 * Chace of aternation text.
 */
struct EB_Alternation_Cache_Struct {
    /*
     * Character number.
     */
    int character_number;

    /*
     * Alternation string for `char_no'.
     */
    char text[EB_MAX_ALTERNATION_TEXT_LENGTH + 1];
};

/*
 * An appendix for a subbook.
 */
struct EB_Appendix_Subbook_Struct {

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
    char directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * Sub-directory name. (EPWING only)
     */
    char data_directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * File descriptor for the appendix file.
     */
    int appendix_file;

    /*
     * Character code of the book.
     */
    EB_Character_Code character_code;

    /*
     * Start character number of the narrow/wide font.
     */
    int narrow_start;
    int wide_start;

    /*
     * End character number of the narrow/wide font.
     */
    int narrow_end;
    int wide_end;

    /*
     * Start page number of the narrow/wide font.
     */
    int narrow_page;
    int wide_page;

    /*
     * Stop code (first and second characters).
     */
    int stop0;
    int stop1;

    /*
     * Compression Information for appendix file.
     */
    EB_Zip appendix_zip;
};

/*
 * Additional resources for a book.
 */
struct EB_Appendix_Struct {
    /*
     * Path of the book.
     */
    char *path;

    /*
     * The length of the path.
     */
    size_t path_length;

    /*
     * Disc type.  EB (EB/EBG/EBXA/EBXA-C/S-EBXA) or EPWING.
     */
    EB_Disc_Code disc_code;

    /*
     * The number of subbooks the book has.
     */
    int subbook_count;

    /*
     * Subbook list.
     */
    EB_Appendix_Subbook *subbooks;

    /*
     * Current subbook.
     */
    EB_Appendix_Subbook *subbook_current;

    /*
     * Lock.
     */
#ifdef ENABLE_PTHREAD
    EB_Lock lock;
#endif

    /*
     * Cache table for alternation text.
     */
    EB_Alternation_Cache narrow_cache[EB_MAX_ALTERNATION_CACHE];
    EB_Alternation_Cache wide_cache[EB_MAX_ALTERNATION_CACHE];
};

/*
 * A font in a subbook.
 */
struct EB_Font_Struct {
    /*
     * Font Code.
     */
    EB_Font_Code font_code;

    /*
     * Avaiable or not.
     */
    int available;

    /*
     * Character numbers of the start and end of the font.
     */
    int start;
    int end;

    /*
     * Page number of the start page of the font data.
     * Used in EB* only. (In EPWING, it is alyways 1).
     */
    int page;

    /*
     * File descriptor of the font file. (EPWING only)
     */
    int font_file;

    /*
     * File name of the font. (EPWING only)
     */
    char file_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * Compression Information.
     */
    EB_Zip zip;
};

/*
 * A language in a book. (EB* only)
 */
struct EB_Language_Struct {
    /*
     * Language ID.
     */
    EB_Language_Code code;

    /*
     * Location of the messages in the language file.
     */
    off_t location;

    /*
     * The number of messages the language file has.
     */
    int message_count;

    /*
     * Language name.
     */
    char name[EB_MAX_LANGUAGE_NAME_LENGTH + 1];
};

/*
 * Search methods in a subbook.
 */
struct EB_Search_Struct {
    /*
     * Page number of the start page of an index.
     */
    int index_page;

    /*
     * Page number of the start page of candidates.
     * (for an entry in multi search)
     */
    int candidates_page;

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
     * Label. (for an entry in multi search)
     */
    char label[EB_MAX_MULTI_LABEL_LENGTH + 1];
};

/*
 * A multi-search entry in a subbook.
 */
struct EB_Multi_Search_Struct {
    /*
     * Search method information.
     */
    EB_Search search;

    /*
     * The number of entries the multi search has.
     */
    int entry_count;

    /*
     * List of Word entry information.
     */
    EB_Search entries[EB_MAX_MULTI_ENTRIES];
};

/*
 * A subbook in a book.
 */
struct EB_Subbook_Struct {
    /*
     * Whether information about the subbook have been loaded in this
     * structure.
     */
    int initialized;

    /*
     * Index page.
     */
    int index_page;

    /*
     * Subbook ID.
     */
    EB_Subbook_Code code;

    /*
     * File descriptor for the subbook file.
     */
    int text_file;

    /*
     * Title of the subbook.
     */
    char title[EB_MAX_TITLE_LENGTH + 1];

    /*
     * Subbook directory name.
     */
    char directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * Sub-directory names. (EPWING only)
     */
    char data_directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char gaiji_directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char movie_directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char stream_directory_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * File names. (EPWING only)
     */
    char text_file_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char graphic_file_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];
    char sound_file_name[EB_MAX_DIRECTORY_NAME_LENGTH + 1];

    /*
     * The top page of search methods.
     */
    EB_Search word_alphabet;
    EB_Search word_asis;
    EB_Search word_kana;
    EB_Search endword_alphabet;
    EB_Search endword_asis;
    EB_Search endword_kana;
    EB_Search keyword;
    EB_Search menu;
    EB_Search graphic;
    EB_Search copyright;

    /*
     * The number of multi-search methods the subbook has.
     */
    int multi_count;

    /*
     * The top page of multi search methods.
     */
    EB_Multi_Search multis[EB_MAX_MULTI_SEARCHES];

    /*
     * Font list.
     */
    EB_Font narrow_fonts[EB_MAX_FONTS];
    EB_Font wide_fonts[EB_MAX_FONTS];

    /*
     * Current narrow and wide fonts.
     */
    EB_Font *narrow_current;
    EB_Font *wide_current;

    /*
     * Compression Information for text file.
     */
    EB_Zip text_zip;
};

/*
 * Context parameters for text reading.
 */
struct EB_Text_Context_Struct {
    /*
     * Current text content type (text, heading, rawtext or none).
     */
    EB_Text_Code code;

    /*
     * Current offset pointer of the START or HONMON file.
     */
    off_t location;

    /*
     * Work buffer passed to hook functions.
     */
    char work_buffer[EB_MAX_TEXT_WORK_LENGTH + 1];

    /*
     * Length of a string in `work_buffer'.
     */
    size_t work_length;

    /*
     * The number of bytes added to `location', when a string in
     * `work_buffer' is flushed.
     */
    size_t work_step;

    /*
     * Narrow character region flag.
     */
    int narrow_flag;

    /*
     * Whether a printable character has been appeared in the current
     * text content.
     */
    int printable_count;

    /* 
     * EOF flag of the current subbook.
     */
    int file_end_flag;

    /*
     * Whether the current text content ends or not.
     */
    int text_end_flag;

    /*
     * Skip until `skipcoe' appears.
     */
    int skip_code;

    /*
     * Stop-code automatically set by EB Library.
     */
    int auto_stop_code;
};

/*
 * Context parameters for word search.
 */
struct EB_Search_Context_Struct {
    /*
     * Current search method type.
     * (exactword, word, endword, keyword or none).
     */
    EB_Search_Code code;

    /*
     * Function which compares word to search and pattern in an index page.
     */
    int (*compare) EB_P((const char *, const char *, size_t));

    /*
     * Result of comparison by `compare'.
     */
    int comparison_result;

    /*
     * Word to search.
     */
    char word[EB_MAX_WORD_LENGTH + 1];

    /*
     * Canonicalized word to search.
     */
    char canonicalized_word[EB_MAX_WORD_LENGTH + 1];

    /*
     * Page which is searched currently.
     */
    int page;

    /*
     * Offset which is searched currently in the page.
     */
    int offset;

    /*
     * Page ID of the current page.
     */
    int page_id;

    /*
     * How many entries in the current page.
     */
    int entry_count;

    /*
     * Entry index pointer.
     */
    int entry_index;

    /*
     * Length of the current entry.
     */
    int entry_length;

    /*
     * Arrangement style of entries in the current page (fixed or variable).
     */
    int entry_arrangement;

    /*
     * In a group entry or not.
     */
    int in_group_entry;

    /*
     * Current heading position (for keyword search).
     */
    EB_Position keyword_heading;
};

/*
 * A book.
 */
struct EB_Book_Struct {
    /*
     * Book ID.
     */
    EB_Book_Code code;

    /*
     * Disc type.  EB* or EPWING.
     */
    EB_Disc_Code disc_code;

    /*
     * Format version. (EPWING only)
     */
    int version;

    /*
     * Character code of the book.
     */
    EB_Character_Code character_code;

    /*
     * Path of the book.
     */
    char *path;

    /*
     * The length of the path.
     */
    size_t path_length;

    /*
     * The number of subbooks the book has.
     */
    int subbook_count;

    /*
     * Subbook list.
     */
    EB_Subbook *subbooks;

    /*
     * Current subbook.
     */
    EB_Subbook *subbook_current;

    /*
     * The number of languages the book supports.
     */
    int language_count;

    /*
     * Language list.
     */
    EB_Language *languages;

    /*
     * Current language.
     */
    EB_Language *language_current;

    /*
     * Messages in the current language.
     */
    char *messages;

    /*
     * Context parameters for text reading.
     */
    EB_Text_Context text_context;

    /*
     * Context parameters for text reading.
     */
    EB_Search_Context search_contexts[EB_NUMBER_OF_SEARCH_CONTEXTS];

    /*
     * Lock.
     */
#ifdef ENABLE_PTHREAD
    EB_Lock lock;
#endif
};

/*
 * In a word search, heading and text locations of a matched entry
 * are stored.
 */
struct EB_Hit_Struct {
    /*
     * Heading position.
     */
    EB_Position heading;

    /*
     * Text position.
     */
    EB_Position text;
};

/*
 * A text hook.
 */
struct EB_Hook_Struct {
    /*
     * Hook code.
     */
    EB_Hook_Code code;

    /*
     * Hook function for the hook code `code'.
     */
    EB_Error_Code (*function) EB_P((EB_Book *, EB_Appendix *, char *,
	EB_Hook_Code, int, const int *));
};

/*
 * A set of text hooks.
 */
struct EB_Hookset_Struct {
    /*
     * List of hooks.
     */
    EB_Hook hooks[EB_NUMBER_OF_HOOKS];

    /*
     * Lock.
     */
#ifdef ENABLE_PTHREAD
    EB_Lock lock;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* not EB_DEFS_H */
