/*                                                            -*- C -*-
 * Copyright (c) 1997, 98, 2000  Motoyuki Kasahara
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

#ifndef EB_EB_H
#define EB_EB_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EB_BUILD_LIBRARY
#include "defs.h"
#else
#include <eb/defs.h>
#endif

/*
 * Function declarations.
 */
/* book.c */
void eb_initialize_book EB_P((EB_Book *));
EB_Error_Code eb_bind EB_P((EB_Book *, const char *));
void eb_suspend EB_P((EB_Book *));
void eb_finalize_book EB_P((EB_Book *));
int eb_is_bound EB_P((EB_Book *));
EB_Error_Code eb_path EB_P((EB_Book *, char *));
EB_Error_Code eb_disc_type EB_P((EB_Book *, EB_Disc_Code *));
EB_Error_Code eb_character_code EB_P((EB_Book *, EB_Character_Code *));

/* copyright.h */
int eb_have_copyright EB_P((EB_Book *));
EB_Error_Code eb_copyright EB_P((EB_Book *, EB_Position *));

/* eb.c */
EB_Error_Code eb_initialize_library EB_P((void));
void eb_finalize_library EB_P((void));

/* endword.c */
int eb_have_endword_search EB_P((EB_Book *));
EB_Error_Code eb_search_endword EB_P((EB_Book *, const char *));

/* exactword.c */
int eb_have_exactword_search EB_P((EB_Book *));
EB_Error_Code eb_search_exactword EB_P((EB_Book *, const char *));

/* graphic.c */
int eb_have_graphic_search EB_P((EB_Book *));

/* keyword.c */
int eb_have_keyword_search EB_P((EB_Book *));
EB_Error_Code eb_search_keyword EB_P((EB_Book *, const char * const []));

/* menu.c */
int eb_have_menu EB_P((EB_Book *));
EB_Error_Code eb_menu EB_P((EB_Book *, EB_Position *));

/* multi.c */
int eb_have_multi_search EB_P((EB_Book *));
EB_Error_Code eb_multi_search_list EB_P((EB_Book *, EB_Multi_Search_Code *,
    int *));
EB_Error_Code eb_multi_entry_list EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code *, int *));
EB_Error_Code eb_multi_entry_label EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code, char *));
int eb_multi_entry_have_candidates EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code));
EB_Error_Code eb_multi_entry_candidates EB_P((EB_Book *, 
    EB_Multi_Search_Code, EB_Multi_Entry_Code, EB_Position *));
EB_Error_Code eb_search_multi EB_P((EB_Book *, EB_Multi_Search_Code,
    const char * const []));

/* search.c */
EB_Error_Code eb_hit_list EB_P((EB_Book *, int, EB_Hit *, int *));

/* subbook.c */
EB_Error_Code eb_initialize_all_subbooks EB_P((EB_Book *));
EB_Error_Code eb_subbook_list EB_P((EB_Book *, EB_Subbook_Code *, int *));
EB_Error_Code eb_subbook EB_P((EB_Book *, EB_Subbook_Code *));
EB_Error_Code eb_subbook_title EB_P((EB_Book *, char *));
EB_Error_Code eb_subbook_title2 EB_P((EB_Book *, EB_Subbook_Code, char *));
EB_Error_Code eb_subbook_directory EB_P((EB_Book *, char *));
EB_Error_Code eb_subbook_directory2 EB_P((EB_Book *, EB_Subbook_Code,
    char *));
EB_Error_Code eb_set_subbook EB_P((EB_Book *, EB_Subbook_Code));
void eb_unset_subbook EB_P((EB_Book *));

/* word.c */
int eb_have_word_search EB_P((EB_Book *));
EB_Error_Code eb_search_word EB_P((EB_Book *, const char *));

#ifdef __cplusplus
}
#endif

#endif /* not EB_EB_H */
