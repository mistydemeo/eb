/*                                                            -*- C -*-
 * Copyright (c) 1997, 98, 99, 2000, 01  
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
#define EB_HOOK_BEGIN_NO_NEWLINE	9

#define EB_HOOK_END_NO_NEWLINE		10
#define EB_HOOK_BEGIN_EMPHASIS		11
#define EB_HOOK_END_EMPHASIS		12
#define EB_HOOK_BEGIN_CANDIDATE		13
#define EB_HOOK_END_CANDIDATE_GROUP	14

#define EB_HOOK_END_CANDIDATE_LEAF	15
#define EB_HOOK_BEGIN_REFERENCE		16
#define EB_HOOK_END_REFERENCE		17
#define EB_HOOK_BEGIN_KEYWORD		18
#define EB_HOOK_END_KEYWORD		19

#define EB_HOOK_NARROW_FONT		20
#define EB_HOOK_WIDE_FONT		21
#define EB_HOOK_ISO8859_1		22
#define EB_HOOK_NARROW_JISX0208		23
#define EB_HOOK_WIDE_JISX0208		24

#define EB_HOOK_GB2312			25
#define EB_HOOK_BEGIN_MONO_GRAPHIC	26
#define EB_HOOK_END_MONO_GRAPHIC	27
#define EB_HOOK_BEGIN_GRAY_GRAPHIC	28
#define EB_HOOK_END_GRAY_GRAPHIC	29

#define EB_HOOK_BEGIN_COLOR_BMP		30
#define EB_HOOK_BEGIN_COLOR_JPEG	31
#define EB_HOOK_END_COLOR_GRAPHIC	32
#define EB_HOOK_END_IN_COLOR_GRAPHIC	33
#define EB_HOOK_BEGIN_WAVE		34

#define EB_HOOK_END_WAVE		35
#define EB_HOOK_BEGIN_MPEG		36
#define EB_HOOK_END_MPEG		37
#define EB_HOOK_BEGIN_GRAPHIC_REFERENCE	38
#define EB_HOOK_END_GRAPHIC_REFERENCE	39

#define EB_HOOK_GRAPHIC_REFERENCE	40
#define EB_HOOK_BEGIN_DECORATION	41
#define EB_HOOK_END_DECORATION		42

/*
 * Function declarations.
 */
/* hook.c */
void eb_initialize_hookset EB_P((EB_Hookset *));
void eb_finalize_hookset EB_P((EB_Hookset *));
EB_Error_Code eb_set_hook EB_P((EB_Hookset *, const EB_Hook *));
EB_Error_Code eb_set_hooks EB_P((EB_Hookset *, const EB_Hook *));
EB_Error_Code eb_hook_euc_to_ascii EB_P((EB_Book *, EB_Appendix *, void *,
    EB_Hook_Code, int, const unsigned int *));
EB_Error_Code eb_hook_stop_code EB_P((EB_Book *, EB_Appendix *, void *,
    EB_Hook_Code, int, const unsigned int *));
EB_Error_Code eb_hook_narrow_character_text EB_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
EB_Error_Code eb_hook_wide_character_text EB_P((EB_Book *, EB_Appendix *,
    void *, EB_Hook_Code, int, const unsigned int *));
EB_Error_Code eb_hook_newline EB_P((EB_Book *, EB_Appendix *, void *,
    EB_Hook_Code, int, const unsigned int *));
EB_Error_Code eb_hook_empty EB_P((EB_Book *, EB_Appendix *, void *,
    EB_Hook_Code, int, const unsigned int *));

/* readtext.c */
EB_Error_Code eb_seek_text EB_P((EB_Book *, const EB_Position *));
EB_Error_Code eb_tell_text EB_P((EB_Book *, EB_Position *));
EB_Error_Code eb_read_text EB_P((EB_Book *, EB_Appendix *, EB_Hookset *,
    void *, size_t, char *, ssize_t *));
EB_Error_Code eb_read_heading EB_P((EB_Book *, EB_Appendix *, EB_Hookset *,
    void *, size_t, char *, ssize_t *));
EB_Error_Code eb_read_rawtext EB_P((EB_Book *, size_t, char *, ssize_t *));
int eb_is_text_stopped EB_P((EB_Book *));
EB_Error_Code eb_write_text_byte1 EB_P((EB_Book *, int));
EB_Error_Code eb_write_text_byte2 EB_P((EB_Book *, int, int));
EB_Error_Code eb_write_text_string EB_P((EB_Book *, const char *));
EB_Error_Code eb_write_text EB_P((EB_Book *, const char *, size_t));
const char *eb_current_candidate EB_P((EB_Book *));
EB_Error_Code eb_forward_text EB_P((EB_Book *, EB_Appendix *));
EB_Error_Code eb_backward_text EB_P((EB_Book *, EB_Appendix *));

#ifdef __cplusplus
}
#endif

#endif /* not EB_TEXT_H */
