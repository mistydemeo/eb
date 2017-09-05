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
#define EB_LANGUAGE_ENGLISH		0
#define EB_LANGUAGE_FRENCH		1
#define EB_LANGUAGE_GERMAN		2
#define EB_LANGUAGE_ITALIAN		3
#define EB_LANGUAGE_SPANISH		4
#define EB_LANGUAGE_JAPANESE		5
#define EB_LANGUAGE_DANISH		6
#define EB_LANGUAGE_FINNISH		7
#define EB_LANGUAGE_SWEDISH		8
#define EB_LANGUAGE_NORWEGIAN		9
#define EB_LANGUAGE_DUTCH		10
#define EB_LANGUAGE_POLISH		11
#define EB_LANGUAGE_INVALID		-1

/*
 * Message codes.
 */
#define EB_MESSAGE_WORD_SEARCH		0x00
#define EB_MESSAGE_END_SEARCH		0x01
#define EB_MESSAGE_KEY_SEARCH		0x02
#define EB_MESSAGE_MENU_SEARCH		0x03
#define EB_MESSAGE_MULTI_SEARCH		0x04
#define EB_MESSAGE_GRAPHIC_SEARCH	0x05
#define EB_MESSAGE_BOOK_CONTAIN		0x20
#define EB_MESSAGE_ENTER_WORD		0x21
#define EB_MESSAGE_ENTER_WORDS		0x22
#define EB_MESSAGE_SUCCESSFUL		0x23
#define EB_MESSAGE_ENTRY		0x24
#define EB_MESSAGE_ENTRIES		0x25
#define EB_MESSAGE_SEARCH_FAIL		0x26
#define EB_MESSAGE_PUSH_NO		0x27
#define EB_MESSAGE_INSERT_DISC		0x28
#define EB_MESSAGE_CANT_READ		0x29
#define EB_MESSAGE_REINSERT_DISC	0x2a
#define EB_MESSAGE_CLEAN_DISC		0x2b
#define EB_MESSAGE_NOT_EB_DISC		0x2c
#define EB_MESSAGE_SEARCHING		0x2d
#define EB_MESSAGE_CHANGING_BATT	0x2e
#define EB_MESSAGE_DRY_BATT		0x2f
#define EB_MESSAGE_CHARGE_END		0x30
#define EB_MESSAGE_BATT_EMPTY		0x31
#define EB_MESSAGE_BATT_CHANGE		0x32
#define EB_MESSAGE_CANT_CHANGE_BATT	0x33
#define EB_MESSAGE_SELECT_LANGUAGE	0x34
#define EB_MESSAGE_MENU			0x35
#define EB_MESSAGE_INVALID		-1

/*
 * Function declarations.
 */
/* language.c */
EB_Error_Code eb_language_list EB_P((EB_Book *, EB_Language_Code *, int *));
int eb_have_language EB_P((EB_Book *, EB_Language_Code));
EB_Error_Code eb_language EB_P((EB_Book *, EB_Language_Code *));
EB_Error_Code eb_language_name EB_P((EB_Book *, char *));
EB_Error_Code eb_language_name2 EB_P((EB_Book *, EB_Language_Code, char *));
EB_Error_Code eb_set_language EB_P((EB_Book *, EB_Language_Code));
void eb_unset_language EB_P((EB_Book *));

/* message.c */
EB_Error_Code eb_message_list EB_P((EB_Book *, EB_Message_Code *, int *));
int eb_have_message EB_P((EB_Book *, EB_Message_Code));
EB_Error_Code eb_message EB_P((EB_Book *, EB_Message_Code, char *));

#ifdef __cplusplus
}
#endif

#endif /* not EB_LANGUAGE_H */

