/*
 * Copyright (c) 1997  Motoyuki Kasahara
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#include "eb.h"
#include "error.h"
#include "font.h"

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef __STDC__
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ */
#endif


/*
 * Return required buffer size for a narrow font character converted
 * to XBM image format.
 */
int
eb_narrow_font_xbm_size(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
        return EB_SIZE_NARROW_FONT_16_XBM;
    case EB_FONT_24:
        return EB_SIZE_NARROW_FONT_24_XBM;
    case EB_FONT_30:
        return EB_SIZE_NARROW_FONT_30_XBM;
    case EB_FONT_48:
        return EB_SIZE_NARROW_FONT_48_XBM;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}

/*
 * Return required buffer size for a narrow font character converted
 * to XPM image format.
 */
int
eb_narrow_font_xpm_size(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
        return EB_SIZE_NARROW_FONT_16_XPM;
    case EB_FONT_24:
        return EB_SIZE_NARROW_FONT_24_XPM;
    case EB_FONT_30:
        return EB_SIZE_NARROW_FONT_30_XPM;
    case EB_FONT_48:
        return EB_SIZE_NARROW_FONT_48_XPM;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}

/*
 * Return required buffer size for a narrow font character converted
 * to GIF image format.
 */
int
eb_narrow_font_gif_size(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
        return EB_SIZE_NARROW_FONT_16_GIF;
    case EB_FONT_24:
        return EB_SIZE_NARROW_FONT_24_GIF;
    case EB_FONT_30:
        return EB_SIZE_NARROW_FONT_30_GIF;
    case EB_FONT_48:
        return EB_SIZE_NARROW_FONT_48_GIF;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}

/*
 * Return required buffer size for a wide font character converted
 * to XBM image format.
 */
int
eb_wide_font_xbm_size(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
        return EB_SIZE_NARROW_FONT_16_XBM;
    case EB_FONT_24:
        return EB_SIZE_NARROW_FONT_24_XBM;
    case EB_FONT_30:
        return EB_SIZE_NARROW_FONT_30_XBM;
    case EB_FONT_48:
        return EB_SIZE_NARROW_FONT_48_XBM;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}

/*
 * Return required buffer size for a wide font character converted
 * to XPM image format.
 */
int
eb_wide_font_xpm_size(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
        return EB_SIZE_NARROW_FONT_16_XPM;
    case EB_FONT_24:
        return EB_SIZE_NARROW_FONT_24_XPM;
    case EB_FONT_30:
        return EB_SIZE_NARROW_FONT_30_XPM;
    case EB_FONT_48:
        return EB_SIZE_NARROW_FONT_48_XPM;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}

/*
 * Return required buffer size for a wide font character converted
 * to GIF image format.
 */
int
eb_wide_font_gif_size(height)
    EB_Font_Code height;
{
    switch (height) {
    case EB_FONT_16:
        return EB_SIZE_NARROW_FONT_16_GIF;
    case EB_FONT_24:
        return EB_SIZE_NARROW_FONT_24_GIF;
    case EB_FONT_30:
        return EB_SIZE_NARROW_FONT_30_GIF;
    case EB_FONT_48:
        return EB_SIZE_NARROW_FONT_48_GIF;
    }

    eb_error = EB_ERR_NO_SUCH_FONT;
    return -1;
}

/*
 * The maximum number of octets in a line in a XBM file.
 */
#define XBM_MAX_OCTETS_A_LINE		12

/*
 * The basename of a XBM file.
 */
#define XBM_BASENAME			"default"

/*
 * Convert a bitmap image to XBM format.
 *
 * It requires four arguements.  `buffer' is buffer to store the
 * XBM image data.  `bitmap', `width', and `height' are bitmap
 * data, width, and height of the bitmap image.
 */
size_t
eb_bitmap_to_xbm(buffer, bitmap, width, height)
    char *buffer;
    const char *bitmap;
    int width;
    int height;
{
    char *bufp = buffer;
    const unsigned char *bitp;
    int bitmap_size = (width + 7) / 8 * height;
    int ch;
    int i;

    /*
     * Write a header part.
     */
    sprintf(bufp, "#define %s_width %4d\n", XBM_BASENAME, width);
    bufp = strchr(bufp, '\n') + 1;
    sprintf(bufp, "#define %s_height %4d\n", XBM_BASENAME, height);
    bufp = strchr(bufp, '\n') + 1;
    sprintf(bufp, "static unsigned char %s_bits[] = {\n", XBM_BASENAME);
    bufp = strchr(bufp, '\n') + 1;

    /*
     * Write an image data part.
     */
    for (i = 0, bitp = (const unsigned char *)bitmap;
	 i < bitmap_size; i++, bitp++) {
	ch = 0;
        if (*bitp & 0x80)
            ch |= 0x01;
        if (*bitp & 0x40)
            ch |= 0x02;
        if (*bitp & 0x20)
            ch |= 0x04;
        if (*bitp & 0x10)
            ch |= 0x08;
        if (*bitp & 0x08)
            ch |= 0x10;
        if (*bitp & 0x04)
            ch |= 0x20;
        if (*bitp & 0x02)
            ch |= 0x40;
        if (*bitp & 0x01)
            ch |= 0x80;

	if (i % XBM_MAX_OCTETS_A_LINE != 0) {
	    sprintf(bufp, ", 0x%02x", ch);
	    bufp += 6;
	} else if (i == 0) {
	    sprintf(bufp, "   0x%02x", ch);
	    bufp += 7;
	} else {
	    sprintf(bufp, ",\n   0x%02x", ch);
	    bufp += 9;
	}
    }
    
    memcpy(bufp, "};\n", 3);
    bufp += 3;

    return (bufp - buffer);
}

/********************************************************************/

/*
 * The basename of a XPM file.
 */
#define XPM_BASENAME		"default"

/*
 * The foreground and background colors of XPM image.
 */
#define XPM_FOREGROUND_COLOR		"Black"
#define XPM_BACKGROUND_COLOR		"None"

/*
 * Convert a bitmap image to XPM format.
 *
 * It requires four arguements.  `buffer' is buffer to store the
 * XPM image data.  `bitmap', `width', and `height' are bitmap
 * data, width, and height of the bitmap image.
 */
size_t
eb_bitmap_to_xpm(buffer, bitmap, width, height)
    char *buffer;
    const char *bitmap;
    int width;
    int height;
{
    char *bufp = buffer;
    const unsigned char *bitp;
    int i, j;

    /*
     * Write a header part.
     */
    sprintf(bufp, "/* XPM */\n");
    bufp = strchr(bufp, '\n') + 1;

    sprintf(bufp, "static char * %s[] = {\n", XPM_BASENAME);
    bufp = strchr(bufp, '\n') + 1;
    
    sprintf(bufp, "\"%d %d 2 1\",\n", width, height);
    bufp = strchr(bufp, '\n') + 1;

    sprintf(bufp, "\" 	c %s\",\n", XPM_BACKGROUND_COLOR);
    bufp = strchr(bufp, '\n') + 1;

    sprintf(bufp, "\". 	c %s\",\n", XPM_FOREGROUND_COLOR);
    bufp = strchr(bufp, '\n') + 1;

    /*
     * Write an image data part.
     */
    for (i = 0, bitp = (const unsigned char *)bitmap; i < height; i++) {
	if (0 < i) {
	    strcpy(bufp, "\",\n\"");
	    bufp += 4;
	} else {
	    *bufp++ = '\"';
	}

	for (j = 0; j + 7 < width; j += 8, bitp++) {
	    *bufp++ = (*bitp & 0x80) ? '.' : ' ';
	    *bufp++ = (*bitp & 0x40) ? '.' : ' ';
	    *bufp++ = (*bitp & 0x20) ? '.' : ' ';
	    *bufp++ = (*bitp & 0x10) ? '.' : ' ';
	    *bufp++ = (*bitp & 0x08) ? '.' : ' ';
	    *bufp++ = (*bitp & 0x04) ? '.' : ' ';
	    *bufp++ = (*bitp & 0x02) ? '.' : ' ';
	    *bufp++ = (*bitp & 0x01) ? '.' : ' ';
	}

	if (j < width) {
	    if (j++ < width)
		*bufp++ = (*bitp & 0x80) ? '.' : ' ';
	    if (j++ < width)
		*bufp++ = (*bitp & 0x40) ? '.' : ' ';
	    if (j++ < width)
		*bufp++ = (*bitp & 0x20) ? '.' : ' ';
	    if (j++ < width)
		*bufp++ = (*bitp & 0x10) ? '.' : ' ';
	    if (j++ < width)
		*bufp++ = (*bitp & 0x08) ? '.' : ' ';
	    if (j++ < width)
		*bufp++ = (*bitp & 0x04) ? '.' : ' ';
	    if (j++ < width)
		*bufp++ = (*bitp & 0x02) ? '.' : ' ';
	    if (j++ < width)
		*bufp++ = (*bitp & 0x01) ? '.' : ' ';
	    bitp++;
	}
    }

    memcpy(bufp, "\"};\n", 4);
    bufp += 4;

    return (bufp - buffer);
}

/********************************************************************/

/*
 * The Foreground and background colors of GIF image.
 */
#define GIF_FOREGROUND_COLOR		0x000000
#define GIF_BACKGROUND_COLOR		0xffffff

/*
 * The preamble of a GIF image.
 */
#define GIF_PREAMBLE_LENGTH	38

static unsigned char gif_preamble[GIF_PREAMBLE_LENGTH] = {
    /*
     * Header. (6 bytes)
     */
    'G', 'I', 'F', '8', '9', 'a',

    /*
     * Logical Screen Descriptor. (7 bytes)
     *   global color table flag = 1.
     *   color resolution = 1 - 1 = 0.
     *   sort flag = 0.
     *   size of global color table = 1 - 1 = 0.
     *   background color index = 0.
     *   the pixel aspect ratio = 0 (unused)
     * Logical screen width and height are set at run time.
     */
    0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,

    /*
     * Global Color Table. (6 bytes)
     * These are set at run time.
     */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /*
     * Graphic Control Extension. (8 bytes)
     *   disposal method = 0.
     *   user input flag = 0.
     *   transparency flag = 1.
     *   delay time = 0.
     *   transparent color index = 0.
     */
    0x21, 0xf9, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00,

    /*
     * Image Descriptor. (10 bytes)
     *   image left position = 0. 
     *   image top position = 0. 
     *   local color table flag = 0.
     *   interlace flag = 0.
     *   sort flag = 0.
     *   size of local color table = 0.
     * Image width and height are set at run time.
     */
    0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /*
     * Code size. (1byte)
     */
    0x03
};

/*
 * Convert a bitmap image to GIF format.
 *
 * It requires four arguements.  `buffer' is buffer to store the
 * GIF image data.  `bitmap', `width', and `height' are bitmap
 * data, width, and height of the bitmap image.
 */
size_t
eb_bitmap_to_gif(buffer, bitmap, width, height)
    char *buffer;
    const char *bitmap;
    int width;
    int height;
{
    unsigned char *bufp = (unsigned char *)buffer;
    const unsigned char *bitp;
    int i, j;

    /*
     * Set logical screen width and height.
     */
    gif_preamble[6] = width & 0xff;
    gif_preamble[7] = (width >> 8) & 0xff;
    gif_preamble[8] = height & 0xff;
    gif_preamble[9] = (height >> 8) & 0xff;

    /*
     * Set global colors.
     */
    gif_preamble[13] = (GIF_BACKGROUND_COLOR >> 16) & 0xff;
    gif_preamble[14] = (GIF_BACKGROUND_COLOR >> 8) & 0xff;
    gif_preamble[15] = GIF_BACKGROUND_COLOR & 0xff;
    gif_preamble[16] = (GIF_FOREGROUND_COLOR >> 16) & 0xff;
    gif_preamble[17] = (GIF_FOREGROUND_COLOR >> 8) & 0xff;
    gif_preamble[18] = GIF_FOREGROUND_COLOR & 0xff;
    
    /*
     * Set image width and height.
     */
    gif_preamble[32] = width & 0xff;
    gif_preamble[33] = (width >> 8) & 0xff;
    gif_preamble[34] = height & 0xff;
    gif_preamble[35] = (height >> 8) & 0xff;

    memcpy(bufp, gif_preamble, GIF_PREAMBLE_LENGTH);
    bufp += GIF_PREAMBLE_LENGTH;

    /*
     * Output image data.
     */
    bitp = (const unsigned char *)bitmap;
    for (i = 0;  i < height; i++) {
	*bufp++ = (unsigned char)width;
	for (j = 0; j + 7 < width; j += 8, bitp++) {
	    *bufp++ = (*bitp & 0x80) ? 0x81 : 0x80;
	    *bufp++ = (*bitp & 0x40) ? 0x81 : 0x80;
	    *bufp++ = (*bitp & 0x20) ? 0x81 : 0x80;
	    *bufp++ = (*bitp & 0x10) ? 0x81 : 0x80;
	    *bufp++ = (*bitp & 0x08) ? 0x81 : 0x80;
	    *bufp++ = (*bitp & 0x04) ? 0x81 : 0x80;
	    *bufp++ = (*bitp & 0x02) ? 0x81 : 0x80;
	    *bufp++ = (*bitp & 0x01) ? 0x81 : 0x80;
	}

	if (j < width) {
	    if (j++ < width)
		*bufp++ = (*bitp & 0x80) ? 0x81 : 0x80;
	    if (j++ < width)
		*bufp++ = (*bitp & 0x40) ? 0x81 : 0x80;
	    if (j++ < width)
		*bufp++ = (*bitp & 0x20) ? 0x81 : 0x80;
	    if (j++ < width)
		*bufp++ = (*bitp & 0x10) ? 0x81 : 0x80;
	    if (j++ < width)
		*bufp++ = (*bitp & 0x08) ? 0x81 : 0x80;
	    if (j++ < width)
		*bufp++ = (*bitp & 0x04) ? 0x81 : 0x80;
	    if (j++ < width)
		*bufp++ = (*bitp & 0x02) ? 0x81 : 0x80;
	    if (j++ < width)
		*bufp++ = (*bitp & 0x01) ? 0x81 : 0x80;
	    bitp++;
	}
    }

    memcpy(bufp, "\001\011\000\073", 4);
    bufp += 4;

    return ((char *)bufp - buffer);
}


#ifdef TEST

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#define test_width 32
#define test_height 16
static unsigned char test_bits[] = {
   0xff, 0xff, 0xff, 0xff, 0x80, 0x81, 0x83, 0x01, 0x80, 0x81, 0x01, 0x01,
   0x80, 0x81, 0x01, 0x01, 0xe3, 0x8f, 0x11, 0xc7, 0xe3, 0x8f, 0x0f, 0xc7,
   0xe3, 0x81, 0x87, 0xc7, 0xe3, 0x81, 0xc3, 0xc7, 0xe3, 0x81, 0xe1, 0xc7,
   0xe3, 0x8f, 0x11, 0xc7, 0xe3, 0x8f, 0x11, 0xc7, 0xe3, 0x81, 0x01, 0xc7,
   0xe3, 0x81, 0x01, 0xc7, 0xe3, 0x81, 0x83, 0xc7, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff
};

int
main(argc, argv)
    int argc;
    char *argv[];
{
    char image[EB_SIZE_FONT_IMAGE];
    size_t size;
    int file;

    size = eb_convert_bitmap_xbm(image, test_bits, test_width, test_height);
    file = creat("test.xbm", 0644);
    if (file < 0)
	exit(1);
    if (write(file, image, size) != size) {
	close(file);
	exit(1);
    }

    size = eb_convert_bitmap_xpm(image, test_bits, test_width, test_height);
    file = creat("test.xpm", 0644);
    if (file < 0)
	exit(1);
    if (write(file, image, size) != size) {
	close(file);
	exit(1);
    }

    size = eb_convert_bitmap_gif(image, test_bits, test_width, test_height);
    file = creat("test.gif", 0644);
    if (file < 0)
	exit(1);
    if (write(file, image, size) != size) {
	close(file);
	exit(1);
    }

    return 0;
}

#endif /* TEST */
