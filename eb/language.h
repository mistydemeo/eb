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

#ifndef EB_LANGUAGE_H
#define EB_LANGUAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifdef EB_BUILD_LIBRARY
#include "defs.h"
#else
#include <eb/defs.h>
#endif

/*
 * Language codes.
 */
#define EB_LANG_ENGLISH		0
#define EB_LANG_FRENCH		1
#define EB_LANG_GERMAN		2
#define EB_LANG_ITALIAN		3
#define EB_LANG_SPANISH		4
#define EB_LANG_JAPANESE	5
#define EB_LANG_DANISH		6
#define EB_LANG_FINNISH		7
#define EB_LANG_SWEDISH		8
#define EB_LANG_NORWEGIAN	9
#define EB_LANG_DUTCH		10
#define EB_LANG_POLISH		11

/*
 * Message codes.
 */
#define EB_MSG_WORD_SEARCH	0x00
#define EB_MSG_END_SEARCH	0x01
#define EB_MSG_KEY_SEARCH	0x02
#define EB_MSG_MENU_SEARCH	0x03
#define EB_MSG_MULTI_SEARCH	0x04
#define EB_MSG_GRAPHIC_SEARCH	0x05
#define EB_MSG_BOOK_CONTAIN	0x20
#define EB_MSG_ENTER_WORD	0x21
#define EB_MSG_ENTER_WORDS	0x22
#define EB_MSG_SUCCESSFUL	0x23
#define EB_MSG_ENTRY		0x24
#define EB_MSG_ENTRIES		0x25
#define EB_MSG_SEARCH_FAIL	0x26
#define EB_MSG_PUSH_NO		0x27
#define EB_MSG_INSERT_DISC	0x28
#define EB_MSG_CANT_READ	0x29
#define EB_MSG_REINSERT_DISC	0x2a
#define EB_MSG_CLEAN_DISC	0x2b
#define EB_MSG_NOT_EB_DISC	0x2c
#define EB_MSG_SEARCHING	0x2d
#define EB_MSG_CHANGING_BATT	0x2e
#define EB_MSG_DRY_BATT		0x2f
#define EB_MSG_CHARGE_END	0x30
#define EB_MSG_BATT_EMPTY	0x31
#define EB_MSG_BATT_CHANGE	0x32
#define EB_MSG_CANT_CHANGE_BATT	0x33
#define EB_MSG_SELECT_LANGUAGE	0x34
#define EB_MSG_MENU		0x35

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
/* language.c */
int eb_language_count EB_P((EB_Book *));
int eb_language_list EB_P((EB_Book *, EB_Language_Code *));
int eb_have_language EB_P((EB_Book *, EB_Language_Code));
EB_Language_Code eb_language EB_P((EB_Book *));
const char *eb_language_name EB_P((EB_Book *));
const char *eb_language_name2 EB_P((EB_Book *, EB_Language_Code));
int eb_set_language EB_P((EB_Book *, EB_Language_Code));
void eb_unset_language EB_P((EB_Book *));
/* message.c */
int eb_message_count EB_P((EB_Book *));
int eb_message_list EB_P((EB_Book *, EB_Message_Code *));
int eb_have_message EB_P((EB_Book *, EB_Message_Code));
const char *eb_message EB_P((EB_Book *, EB_Message_Code));

#ifdef __cplusplus
}
#endif

#endif /* not EB_LANGUAGE_H */

