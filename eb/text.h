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

#ifndef EB_TEXT_H
#define EB_TEXT_H

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
 * Hook codes.
 * (When you add or remove a hook, update EB_NUMER_OF_HOOKS in defs.h.)
 */
#define EB_HOOK_NULL			-1
#define EB_HOOK_INITIALIZE		0
#define EB_HOOK_BEGIN_NARROW		1
#define EB_HOOK_END_NARROW		2
#define EB_HOOK_BEGIN_SUBSCRIPT		3
#define EB_HOOK_END_SUBSCRIPT		4

#define EB_HOOK_SET_INDENT		5
#define EB_HOOK_NEWLINE			6
#define EB_HOOK_BEGIN_SUPERSCRIPT	7
#define EB_HOOK_END_SUPERSCRIPT		8
#define EB_HOOK_BEGIN_TABLE		9

#define EB_HOOK_END_TABLE		10
#define EB_HOOK_BEGIN_NO_NEWLINE	11
#define EB_HOOK_END_NO_NEWLINE		12
#define EB_HOOK_BEGIN_EMPHASIS		13
#define EB_HOOK_END_EMPHASIS		14

#define EB_HOOK_BEGIN_PICTURE		15
#define EB_HOOK_END_PICTURE		16
#define EB_HOOK_BEGIN_MENU		17
#define EB_HOOK_END_MENU		18
#define EB_HOOK_BEGIN_SOUND		19

#define EB_HOOK_END_SOUND		20
#define EB_HOOK_BEGIN_REFERENCE		21
#define EB_HOOK_END_REFERENCE		22
#define EB_HOOK_BEGIN_KEYWORD		23
#define EB_HOOK_END_KEYWORD		24

#define EB_HOOK_NARROW_FONT		25
#define EB_HOOK_WIDE_FONT		26
#define EB_HOOK_ISO8859_1		27
#define EB_HOOK_NARROW_JISX0208		28
#define EB_HOOK_WIDE_JISX0208		29

#define EB_HOOK_GB2312			30
#define EB_HOOK_STOP_CODE		31

/*
 * CPP macro version of get_uint1(), get_uint2(), get_uint4().
 * If EB_UINT_FUNCTION is defined, function version is used, instead.
 */
#if !defined(EB_UINT_FUNCTION) && !defined(EB_BUILD_LIBRARY)
#define eb_uint1(p) (*(const unsigned char *)(p))

#define eb_uint2(p) ((*(const unsigned char *)(p) << 8) \
	+ (*(const unsigned char *)((p) + 1)))

#define eb_uint4(p) ((*(const unsigned char *)(p) << 24) \
	+ (*(const unsigned char *)((p) + 1) << 16) \
	+ (*(const unsigned char *)((p) + 2) << 8) \
	+ (*(const unsigned char *)((p) + 3)))
#endif /* !EB_UINT_FUNCTION && !EB_BUILD_LIBRARY */

/*
 * Function declarations.
 */
/* hook.c */
void eb_initialize_hookset EB_P((EB_Hookset *));
void eb_finalize_hookset EB_P((EB_Hookset *));
EB_Error_Code eb_set_hook EB_P((EB_Hookset *, const EB_Hook *));
EB_Error_Code eb_set_hooks EB_P((EB_Hookset *, const EB_Hook *));
EB_Error_Code eb_hook_euc_to_ascii EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));
EB_Error_Code eb_hook_stop_code EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));
EB_Error_Code eb_hook_narrow_character_text EB_P((EB_Book *, EB_Appendix *,
    char *, EB_Hook_Code, int, const int *));
EB_Error_Code eb_hook_wide_character_text EB_P((EB_Book *, EB_Appendix *,
    char *, EB_Hook_Code, int, const int *));
EB_Error_Code eb_hook_empty EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));

/* text.c */
void eb_initialize_text EB_P((EB_Book *));
EB_Error_Code eb_seek_text EB_P((EB_Book *, const EB_Position *));
EB_Error_Code eb_tell_text EB_P((EB_Book *, EB_Position *));
EB_Error_Code eb_read_text EB_P((EB_Book *, EB_Appendix *, const EB_Hookset *,
    size_t, char *, ssize_t *));
EB_Error_Code eb_read_heading EB_P((EB_Book *, EB_Appendix *,
    const EB_Hookset *, size_t, char *, ssize_t *));
EB_Error_Code eb_read_rawtext EB_P((EB_Book *, size_t, char *, ssize_t *));
EB_Error_Code eb_forward_text EB_P((EB_Book *, const EB_Hookset *));

/* uint.c */
#ifdef EB_UINT_FUNCTION
unsigned eb_uint1 EB_P((const char *));
unsigned eb_uint2 EB_P((const char *));
unsigned eb_uint4 EB_P((const char *));
#endif /* EB_UINT_FUNCTION */
unsigned eb_bcd2 EB_P((const char *));
unsigned eb_bcd4 EB_P((const char *));
unsigned eb_bcd6 EB_P((const char *));

#ifdef __cplusplus
}
#endif

#endif /* not EB_TEXT_H */
