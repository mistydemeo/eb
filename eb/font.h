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

#ifndef EB_FONT_H
#define EB_FONT_H

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
 * Font types.
 */
#define EB_FONT_16		16
#define EB_FONT_24		24
#define EB_FONT_30		30
#define EB_FONT_48		48

/*
 * Font sizes.
 */
#define EB_SIZE_NARROW_FONT_16	16
#define EB_SIZE_WIDE_FONT_16	32
#define EB_SIZE_NARROW_FONT_24	48
#define EB_SIZE_WIDE_FONT_24	72
#define EB_SIZE_NARROW_FONT_30	60
#define EB_SIZE_WIDE_FONT_30	120
#define EB_SIZE_NARROW_FONT_48	144
#define EB_SIZE_WIDE_FONT_48	288

/*
 * Font width.
 */
#define EB_WIDTH_NARROW_FONT_16	8
#define EB_WIDTH_WIDE_FONT_16	16
#define EB_WIDTH_NARROW_FONT_24	16
#define EB_WIDTH_WIDE_FONT_24	24
#define EB_WIDTH_NARROW_FONT_30	16
#define EB_WIDTH_WIDE_FONT_30	32
#define EB_WIDTH_NARROW_FONT_48	24
#define EB_WIDTH_WIDE_FONT_48	48

/*
 * Bitmap image sizes.
 */
#define EB_SIZE_NARROW_FONT_16_XBM		184
#define EB_SIZE_WIDE_FONT_16_XBM		284
#define EB_SIZE_NARROW_FONT_16_XPM		266
#define EB_SIZE_WIDE_FONT_16_XPM		395
#define EB_SIZE_NARROW_FONT_16_GIF		186
#define EB_SIZE_WIDE_FONT_16_GIF		314

#define EB_SIZE_NARROW_FONT_24_XBM		383
#define EB_SIZE_WIDE_FONT_24_XBM		533
#define EB_SIZE_NARROW_FONT_24_XPM		555
#define EB_SIZE_WIDE_FONT_24_XPM		747
#define EB_SIZE_NARROW_FONT_24_GIF		450
#define EB_SIZE_WIDE_FONT_24_GIF		642

#define EB_SIZE_NARROW_FONT_30_XBM		458
#define EB_SIZE_WIDE_FONT_30_XBM		833
#define EB_SIZE_NARROW_FONT_30_XPM		675
#define EB_SIZE_WIDE_FONT_30_XPM		1155
#define EB_SIZE_NARROW_FONT_30_GIF		552
#define EB_SIZE_WIDE_FONT_30_GIF		1032

#define EB_SIZE_NARROW_FONT_48_XBM		983
#define EB_SIZE_WIDE_FONT_48_XBM		1883
#define EB_SIZE_NARROW_FONT_48_XPM		1419
#define EB_SIZE_WIDE_FONT_48_XPM		2571
#define EB_SIZE_NARROW_FONT_48_GIF		1242
#define EB_SIZE_WIDE_FONT_48_GIF		2394

#define EB_SIZE_FONT_IMAGE	EB_SIZE_WIDE_FONT_48_XPM	    

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
/* bitmap.c */
int eb_narrow_font_xbm_size EB_P((EB_Font_Code));
int eb_narrow_font_xpm_size EB_P((EB_Font_Code));
int eb_narrow_font_gif_size EB_P((EB_Font_Code));
int eb_wide_font_xbm_size EB_P((EB_Font_Code));
int eb_wide_font_xpm_size EB_P((EB_Font_Code));
int eb_wide_font_gif_size EB_P((EB_Font_Code));
size_t eb_bitmap_to_xbm EB_P((char *, const char *, int, int));
size_t eb_bitmap_to_xpm EB_P((char *, const char *, int, int));
size_t eb_bitmap_to_gif EB_P((char *, const char *, int, int));
/* font.c */
EB_Font_Code eb_font EB_P((EB_Book *));
int eb_set_font EB_P((EB_Book *, EB_Font_Code));
void eb_unset_font EB_P((EB_Book *));
int eb_font_count EB_P((EB_Book *));
int eb_font_list EB_P((EB_Book *, EB_Font_Code *));
int eb_have_font EB_P((EB_Book *, EB_Font_Code));
/* narwfont.c */
int eb_have_narrow_font EB_P((EB_Book *));
int eb_narrow_font_width EB_P((EB_Book *));
int eb_narrow_font_width2 EB_P((EB_Font_Code));
int eb_narrow_font_size EB_P((EB_Book *));
int eb_narrow_font_size2 EB_P((EB_Font_Code));
const char *eb_narrow_font_filename EB_P((EB_Book *));
const char *eb_narrow_font_filename2 EB_P((EB_Book *, EB_Font_Code));
int eb_narrow_font_start EB_P((EB_Book *));
int eb_narrow_font_end EB_P((EB_Book *));
int eb_narrow_font_character_bitmap EB_P((EB_Book *, int, char *));
int eb_forward_narrow_font_character EB_P((EB_Book *, int, int));
int eb_backward_narrow_font_character EB_P((EB_Book *, int, int));
/* widefont.c */
int eb_have_wide_font EB_P((EB_Book *));
int eb_wide_font_width EB_P((EB_Book *));
int eb_wide_font_width2 EB_P((EB_Font_Code));
int eb_wide_font_size EB_P((EB_Book *));
int eb_wide_font_size2 EB_P((EB_Font_Code));
const char *eb_wide_font_filename EB_P((EB_Book *));
const char *eb_wide_font_filename2 EB_P((EB_Book *, EB_Font_Code));
int eb_wide_font_start EB_P((EB_Book *));
int eb_wide_font_end EB_P((EB_Book *));
int eb_wide_font_character_bitmap EB_P((EB_Book *, int, char *));
int eb_forward_wide_font_character EB_P((EB_Book *, int, int));
int eb_backward_wide_font_character EB_P((EB_Book *, int, int));

#ifdef __cplusplus
}
#endif

#endif /* not EB_FONT_H */
