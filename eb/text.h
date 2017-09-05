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
 */
#define EB_HOOK_NULL			-1
#define EB_HOOK_BEGIN_NARROW		0
#define EB_HOOK_NARROW			1
#define EB_HOOK_END_NARROW		2
#define EB_HOOK_BEGIN_SUBSCRIPT		3
#define EB_HOOK_SUBSCRIPT		4
#define EB_HOOK_END_SUBSCRIPT		5
#define EB_HOOK_SET_INDENT		6
#define EB_HOOK_NEWLINE			7
#define EB_HOOK_BEGIN_SUPERSCRIPT	8
#define EB_HOOK_SUPERSCRIPT		9
#define EB_HOOK_END_SUPERSCRIPT		10
#define EB_HOOK_BEGIN_TABLE		11
#define EB_HOOK_END_TABLE		12
#define EB_HOOK_BEGIN_NO_NEWLINE	13
#define EB_HOOK_NO_NEWLINE		14
#define EB_HOOK_END_NO_NEWLINE		15
#define EB_HOOK_BEGIN_EMPHASIS		16
#define EB_HOOK_EMPHASIS		17
#define EB_HOOK_END_EMPHASIS		18
#define EB_HOOK_BEGIN_PICTURE		19
#define EB_HOOK_PICTURE			20
#define EB_HOOK_END_PICTURE		21
#define EB_HOOK_BEGIN_MENU		22
#define EB_HOOK_MENU			23
#define EB_HOOK_END_MENU		24
#define EB_HOOK_BEGIN_SOUND		25
#define EB_HOOK_SOUND			26
#define EB_HOOK_END_SOUND		27
#define EB_HOOK_BEGIN_REFERENCE		28
#define EB_HOOK_REFERENCE		29
#define EB_HOOK_END_REFERENCE		30
#define EB_HOOK_BEGIN_KEYWORD		31
#define EB_HOOK_KEYWORD			32
#define EB_HOOK_END_KEYWORD		33
#define EB_HOOK_ISO8859_1		34
#define EB_HOOK_NARROW_JISX0208		35
#define EB_HOOK_WIDE_JISX0208		36
#define EB_HOOK_NARROW_FONT		37
#define EB_HOOK_WIDE_FONT		38
#define EB_HOOK_INITIALIZE		39
#define EB_HOOK_STOPCODE		40
#define EB_HOOK_GB2312			41

/*
 * The number of hooks.
 */
#define EB_NUM_HOOKS		42

/*
 * The maximum length of a string in the text work buffer.
 */
#define EB_MAXLEN_TEXT_WORK	255

/*
 * CPP macro version of get_uint1(), get_uint2(), get_uint4().
 * If EB_UINT_FUNC is defined, use function version, instead.
 */
#if !defined(EB_UINT_FUNC) && !defined(EB_BUILD_LIBRARY)
#define eb_uint1(p) (*(const unsigned char *)(p))

#define eb_uint2(p) ((*(const unsigned char *)(p) << 8) \
	+ (*(const unsigned char *)((p) + 1)))

#define eb_uint4(p) ((*(const unsigned char *)(p) << 24) \
	+ (*(const unsigned char *)((p) + 1) << 16) \
	+ (*(const unsigned char *)((p) + 2) << 8) \
	+ (*(const unsigned char *)((p) + 3)))
#endif /* !EB_UINT_FUNC  && !EB_BUILD_LIBRARY */

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
 * Type for hook code.
 */
typedef int EB_Hook_Code;

/*
 * EB_Hook -- A text hook.
 */
typedef struct {
    EB_Hook_Code code;
    int (*function) EB_P((EB_Book *, EB_Appendix *, char *, EB_Hook_Code, int,
	const int *));
} EB_Hook;

/*
 * EB_Hookset -- A set of text hooks.
 */
typedef struct {
    EB_Hook hooks[EB_NUM_HOOKS];
} EB_Hookset;

/*
 * Function declarations.
 */
/* hook.c */
int eb_hook_euc_to_ascii EB_P((EB_Book *, EB_Appendix *, char *, EB_Hook_Code,
    int, const int *));
int eb_hook_stopcode EB_P((EB_Book *, EB_Appendix *, char *, EB_Hook_Code,
    int, const int *));
int eb_hook_stopcode_dummy EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));
int eb_hook_stopcode_mixed EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));
int eb_hook_narrow_character_text EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));
int eb_hook_wide_character_text EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));
int eb_hook_empty EB_P((EB_Book *, EB_Appendix *, char *,
    EB_Hook_Code, int, const int *));
/* text.c */
void eb_initialize_hookset EB_P((EB_Hookset *));
int eb_set_hook EB_P((EB_Hookset *, const EB_Hook *));
int eb_set_hooks EB_P((EB_Hookset *, const EB_Hook *));
int eb_seek EB_P((EB_Book *, const EB_Position *));
int eb_text EB_P((EB_Book *, EB_Appendix *, const EB_Hookset *, char *,
    size_t));
int eb_heading EB_P((EB_Book *, EB_Appendix *, const EB_Hookset *, char *,
    size_t));
ssize_t eb_rawtext EB_P((EB_Book *, char *, size_t));
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
