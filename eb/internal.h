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

#ifndef EB_INTERNAL_H
#define EB_INTERNAL_H

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
 * CPP macro version of get_uint1(), get_uint2(), get_uint4().
 */
#define eb_uint1(p) (*(const unsigned char *)(p))

#define eb_uint2(p) ((*(const unsigned char *)(p) << 8) \
	+ (*(const unsigned char *)((p) + 1)))

#define eb_uint3(p) ((*(const unsigned char *)(p) << 16) \
	+ (*(const unsigned char *)((p) + 1) << 8) \
	+ (*(const unsigned char *)((p) + 2)))

#define eb_uint4(p) ((*(const unsigned char *)(p) << 24) \
	+ (*(const unsigned char *)((p) + 1) << 16) \
	+ (*(const unsigned char *)((p) + 2) << 8) \
	+ (*(const unsigned char *)((p) + 3)))

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
/* appendix.c */
void eb_initialize_alt_cache EB_P((EB_Appendix *));
/* font.c */
int eb_initialize_fonts EB_P((EB_Book *));
/* jpcode.c */
void eb_jisx0208_to_euc EB_P((char *, const char *));
void eb_sjis_to_euc EB_P((char *, const char *));
/* language.c */
int eb_initialize_languages EB_P((EB_Book *));
/* match.c */
int eb_match_word EB_P((const char *, const char *, size_t));
int eb_match_exactword EB_P((const char *, const char *, size_t));
/* message.c */
int eb_initialize_messages EB_P((EB_Book *));
/* search.c */
void eb_initialize_search EB_P((void));
/* setword.c */
EB_Word_Code eb_set_word EB_P((EB_Book *, char *, char *, const char *));
EB_Word_Code eb_set_endword EB_P((EB_Book *, char *, char *, const char *));
/* subbook.c */
int eb_initialize_subbook EB_P((EB_Book *));
/* text.c */
void eb_initialize_text EB_P((void));
/* unzip.c */
int eb_unzip_mode1 EB_P((char *, size_t, char *, size_t));
/* zio.c */
void eb_zclear EB_P((void));
int eb_zopen EB_P((EB_Zip *, char *));
int eb_zclose EB_P((EB_Zip *, int));
off_t eb_zlseek EB_P((EB_Zip *, int, off_t, int));
ssize_t eb_zread EB_P((EB_Zip *, int, char *, size_t));
ssize_t eb_read_all EB_P((int, void *, size_t nbyte));
#ifdef __cplusplus
}
#endif

#endif /* not EB_INTERNAL_H */
