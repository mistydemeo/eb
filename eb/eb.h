/*                                                            -*- C -*-
 * Copyright (c) 1997, 1998  Motoyuki Kasahara
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
 * Function declarations.
 */
/* copyright.h */
int eb_have_copyright EB_P((EB_Book *));
int eb_copyright EB_P((EB_Book *, EB_Position *));
/* eb.c */
void eb_initialize EB_P((EB_Book *));
int eb_bind EB_P((EB_Book *, const char *));
void eb_suspend EB_P((EB_Book *));
void eb_clear EB_P((EB_Book *));
int eb_is_bound EB_P((EB_Book *));
const char *eb_path EB_P((EB_Book *));
EB_Disc_Code eb_disc_type EB_P((EB_Book *));
EB_Character_Code eb_character_code EB_P((EB_Book *));
/* filename.c */
int eb_catalog_filename EB_P((EB_Book *));
int eb_appendix_catalog_filename EB_P((EB_Appendix *));
int eb_canonicalize_filename EB_P((char *));
void eb_fix_filename EB_P((EB_Book *, char *));
void eb_fix_appendix_filename EB_P((EB_Appendix *, char *));
/* graphic.c */
int eb_have_graphic_search EB_P((EB_Book *));
/* keyword.c */
int eb_have_keyword_search EB_P((EB_Book *));
/* menu.c */
int eb_have_menu EB_P((EB_Book *));
int eb_menu EB_P((EB_Book *, EB_Position *));
/* multi.c */
int eb_initialize_multi_search EB_P((EB_Book *));
int eb_have_multi_search EB_P((EB_Book *));
int eb_multi_search_count EB_P((EB_Book *));
int eb_multi_search_list EB_P((EB_Book *, EB_Multi_Search_Code *));
int eb_multi_entry_count EB_P((EB_Book *, EB_Multi_Search_Code));
int eb_multi_entry_list EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code *));
const char *eb_multi_entry_label EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code));
int eb_multi_entry_have_exactword_search EB_P((EB_Book *,
    EB_Multi_Search_Code, EB_Multi_Entry_Code));
int eb_multi_entry_have_word_search EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code));
int eb_multi_entry_have_endword_search EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code));
int eb_multi_entry_have_keyword_search EB_P((EB_Book *, EB_Multi_Search_Code,
    EB_Multi_Entry_Code));
/* search.c */
int eb_have_exactword_search EB_P((EB_Book *));
int eb_search_exactword EB_P((EB_Book *, const char *));
int eb_have_word_search EB_P((EB_Book *));
int eb_search_word EB_P((EB_Book *, const char *));
int eb_have_endword_search EB_P((EB_Book *));
int eb_search_endword EB_P((EB_Book *, const char *));
int eb_hit_list EB_P((EB_Book *, EB_Hit *, int));
/* subbook.c */
int eb_initialize_all_subbooks EB_P((EB_Book *));
int eb_subbook_count EB_P((EB_Book *));
int eb_subbook_list EB_P((EB_Book *, EB_Subbook_Code *));
EB_Subbook_Code eb_subbook EB_P((EB_Book *));
const char *eb_subbook_title EB_P((EB_Book *));
const char *eb_subbook_title2 EB_P((EB_Book *, EB_Subbook_Code));
const char *eb_subbook_directory EB_P((EB_Book *));
const char *eb_subbook_directory2 EB_P((EB_Book *, EB_Subbook_Code));
int eb_set_subbook EB_P((EB_Book *, EB_Subbook_Code));
void eb_unset_subbook EB_P((EB_Book *));

#ifdef __cplusplus
}
#endif

#endif /* not EB_EB_H */
