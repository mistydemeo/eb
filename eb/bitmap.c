/*
 * Copyright (c) 1997, 2000, 01  
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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "internal.h"
#include "font.h"

/*
 * Return required buffer size for a narrow font character converted
 * to XBM image format.
 */
EB_Error_Code
eb_narrow_font_xbm_size(height, size)
    EB_Font_Code height;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
        *size = EB_SIZE_NARROW_FONT_16_XBM;
    case EB_FONT_24:
        *size = EB_SIZE_NARROW_FONT_24_XBM;
    case EB_FONT_30:
        *size = EB_SIZE_NARROW_FONT_30_XBM;
    case EB_FONT_48:
        *size = EB_SIZE_NARROW_FONT_48_XBM;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    return error_code;
}

/*
 * Return required buffer size for a narrow font character converted
 * to XPM image format.
 */
EB_Error_Code
eb_narrow_font_xpm_size(height, size)
    EB_Font_Code height;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
        *size = EB_SIZE_NARROW_FONT_16_XPM;
    case EB_FONT_24:
        *size = EB_SIZE_NARROW_FONT_24_XPM;
    case EB_FONT_30:
        *size = EB_SIZE_NARROW_FONT_30_XPM;
    case EB_FONT_48:
        *size = EB_SIZE_NARROW_FONT_48_XPM;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    return error_code;
}

/*
 * Return required buffer size for a narrow font character converted
 * to GIF image format.
 */
EB_Error_Code
eb_narrow_font_gif_size(height, size)
    EB_Font_Code height;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
        *size = EB_SIZE_NARROW_FONT_16_GIF;
    case EB_FONT_24:
        *size = EB_SIZE_NARROW_FONT_24_GIF;
    case EB_FONT_30:
        *size = EB_SIZE_NARROW_FONT_30_GIF;
    case EB_FONT_48:
        *size = EB_SIZE_NARROW_FONT_48_GIF;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    return error_code;
}

/*
 * Return required buffer size for a wide font character converted
 * to XBM image format.
 */
EB_Error_Code
eb_wide_font_xbm_size(height, size)
    EB_Font_Code height;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
        *size = EB_SIZE_NARROW_FONT_16_XBM;
    case EB_FONT_24:
        *size = EB_SIZE_NARROW_FONT_24_XBM;
    case EB_FONT_30:
        *size = EB_SIZE_NARROW_FONT_30_XBM;
    case EB_FONT_48:
        *size = EB_SIZE_NARROW_FONT_48_XBM;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    return error_code;
}

/*
 * Return required buffer size for a wide font character converted
 * to XPM image format.
 */
EB_Error_Code
eb_wide_font_xpm_size(height, size)
    EB_Font_Code height;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
        *size = EB_SIZE_NARROW_FONT_16_XPM;
    case EB_FONT_24:
        *size = EB_SIZE_NARROW_FONT_24_XPM;
    case EB_FONT_30:
        *size = EB_SIZE_NARROW_FONT_30_XPM;
    case EB_FONT_48:
        *size = EB_SIZE_NARROW_FONT_48_XPM;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    return error_code;
}

/*
 * Return required buffer size for a wide font character converted
 * to GIF image format.
 */
EB_Error_Code
eb_wide_font_gif_size(height, size)
    EB_Font_Code height;
    size_t *size;
{
    EB_Error_Code error_code;

    switch (height) {
    case EB_FONT_16:
        *size = EB_SIZE_NARROW_FONT_16_GIF;
    case EB_FONT_24:
        *size = EB_SIZE_NARROW_FONT_24_GIF;
    case EB_FONT_30:
        *size = EB_SIZE_NARROW_FONT_30_GIF;
    case EB_FONT_48:
        *size = EB_SIZE_NARROW_FONT_48_GIF;
    default:
	error_code = EB_ERR_NO_SUCH_FONT;
	goto failed;
    }

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    *size = 0;
    return error_code;
}

/*
 * The maximum number of octets in a line in a XBM file.
 */
#define XBM_MAX_OCTETS_A_LINE		12

/*
 * The base name of a XBM file.
 */
#define XBM_BASE_NAME			"default"

/*
 * Convert a bitmap image to XBM format.
 *
 * It requires four arguements.  `xbm' is buffer to store the XBM
 * image data.  `bitmap', `width', and `height' are bitmap data,
 * width, and height of the bitmap image.
 */
void
eb_bitmap_to_xbm(bitmap, width, height, xbm, xbm_length)
    const char *bitmap;
    int width;
    int height;
    char *xbm;
    size_t *xbm_length;
{
    char *xbm_p = xbm;
    const unsigned char *bitmap_p = (const unsigned char *)bitmap;
    int bitmap_size = (width + 7) / 8 * height;
    int hex;
    int i;

    /*
     * Output a header.
     */
    sprintf(xbm_p, "#define %s_width %4d\n", XBM_BASE_NAME, width);
    xbm_p = strchr(xbm_p, '\n') + 1;
    sprintf(xbm_p, "#define %s_height %4d\n", XBM_BASE_NAME, height);
    xbm_p = strchr(xbm_p, '\n') + 1;
    sprintf(xbm_p, "static unsigned char %s_bits[] = {\n", XBM_BASE_NAME);
    xbm_p = strchr(xbm_p, '\n') + 1;

    /*
     * Output image data.
     */
    for (i = 0; i < bitmap_size; i++) {
	hex = 0;
        if (*bitmap_p & 0x80)
            hex |= 0x01;
        if (*bitmap_p & 0x40)
            hex |= 0x02;
        if (*bitmap_p & 0x20)
            hex |= 0x04;
        if (*bitmap_p & 0x10)
            hex |= 0x08;
        if (*bitmap_p & 0x08)
            hex |= 0x10;
        if (*bitmap_p & 0x04)
            hex |= 0x20;
        if (*bitmap_p & 0x02)
            hex |= 0x40;
        if (*bitmap_p & 0x01)
            hex |= 0x80;
	bitmap_p++;

	if (i % XBM_MAX_OCTETS_A_LINE != 0) {
	    sprintf(xbm_p, ", 0x%02x", hex);
	    xbm_p += 6;
	} else if (i == 0) {
	    sprintf(xbm_p, "   0x%02x", hex);
	    xbm_p += 7;
	} else {
	    sprintf(xbm_p, ",\n   0x%02x", hex);
	    xbm_p += 9;
	}
    }
    
    /*
     * Output a footer.
     */
    memcpy(xbm_p, "};\n", 3);
    xbm_p += 3;

    *xbm_length = (xbm_p - xbm);
}


/*
 * The base name of a XPM file.
 */
#define XPM_BASE_NAME		"default"

/*
 * The foreground and background colors of XPM image.
 */
#define XPM_FOREGROUND_COLOR		"Black"
#define XPM_BACKGROUND_COLOR		"None"

/*
 * Convert a bitmap image to XPM format.
 *
 * It requires four arguements.  `xpm' is buffer to store the XPM
 * image data.  `bitmap', `width', and `height' are bitmap data,
 * width, and height of the bitmap image.
 */
void
eb_bitmap_to_xpm(bitmap, width, height, xpm, xpm_length)
    const char *bitmap;
    int width;
    int height;
    char *xpm;
    size_t *xpm_length;
{
    char *xpm_p = xpm;
    const unsigned char *bitmap_p = (const unsigned char *)bitmap;
    int i, j;

    /*
     * Output a header.
     */
    sprintf(xpm_p, "/* XPM */\n");
    xpm_p = strchr(xpm_p, '\n') + 1;

    sprintf(xpm_p, "static char * %s[] = {\n", XPM_BASE_NAME);
    xpm_p = strchr(xpm_p, '\n') + 1;
    
    sprintf(xpm_p, "\"%d %d 2 1\",\n", width, height);
    xpm_p = strchr(xpm_p, '\n') + 1;

    sprintf(xpm_p, "\" 	c %s\",\n", XPM_BACKGROUND_COLOR);
    xpm_p = strchr(xpm_p, '\n') + 1;

    sprintf(xpm_p, "\". 	c %s\",\n", XPM_FOREGROUND_COLOR);
    xpm_p = strchr(xpm_p, '\n') + 1;

    /*
     * Output image data.
     */
    for (i = 0; i < height; i++) {
	if (0 < i) {
	    strcpy(xpm_p, "\",\n\"");
	    xpm_p += 4;
	} else {
	    *xpm_p++ = '\"';
	}

	for (j = 0; j + 7 < width; j += 8, bitmap_p++) {
	    *xpm_p++ = (*bitmap_p & 0x80) ? '.' : ' ';
	    *xpm_p++ = (*bitmap_p & 0x40) ? '.' : ' ';
	    *xpm_p++ = (*bitmap_p & 0x20) ? '.' : ' ';
	    *xpm_p++ = (*bitmap_p & 0x10) ? '.' : ' ';
	    *xpm_p++ = (*bitmap_p & 0x08) ? '.' : ' ';
	    *xpm_p++ = (*bitmap_p & 0x04) ? '.' : ' ';
	    *xpm_p++ = (*bitmap_p & 0x02) ? '.' : ' ';
	    *xpm_p++ = (*bitmap_p & 0x01) ? '.' : ' ';
	}

	if (j < width) {
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x80) ? '.' : ' ';
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x40) ? '.' : ' ';
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x20) ? '.' : ' ';
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x10) ? '.' : ' ';
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x08) ? '.' : ' ';
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x04) ? '.' : ' ';
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x02) ? '.' : ' ';
	    if (j++ < width)
		*xpm_p++ = (*bitmap_p & 0x01) ? '.' : ' ';
	    bitmap_p++;
	}
    }

    /*
     * Output a footer.
     */
    memcpy(xpm_p, "\"};\n", 4);
    xpm_p += 4;

    *xpm_length = (xpm_p - xpm);
}


/*
 * The Foreground and background colors of GIF image.
 */
#define GIF_FOREGROUND_COLOR		0x000000
#define GIF_BACKGROUND_COLOR		0xffffff

/*
 * The preamble of a GIF image.
 */
#define GIF_PREAMBLE_LENGTH	38

static const unsigned char gif_default_preamble[GIF_PREAMBLE_LENGTH] = {
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
 * It requires four arguements.  `gif' is buffer to store the GIF
 * image data.  `bitmap', `width', and `height' are bitmap data,
 * width, and height of the bitmap image.
 *
 * Note: This GIF image doesn't use LZW because of patent.
 */
void
eb_bitmap_to_gif(bitmap, width, height, gif, gif_length)
    const char *bitmap;
    int width;
    int height;
    char *gif;
    size_t *gif_length;
{
    unsigned char *gif_p = (unsigned char *)gif;
    const unsigned char *bitmap_p = (const unsigned char *)bitmap;
    unsigned char gif_preamble[GIF_PREAMBLE_LENGTH];
    int i, j;

    /*
     * Copy the default preamble.
     */
    memcpy(gif_preamble, gif_default_preamble, GIF_PREAMBLE_LENGTH);

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

    /*
     * Output a preamble.
     */
    memcpy(gif_p, gif_preamble, GIF_PREAMBLE_LENGTH);
    gif_p += GIF_PREAMBLE_LENGTH;

    /*
     * Output image data.
     */
    for (i = 0;  i < height; i++) {
	*gif_p++ = (unsigned char)width;
	for (j = 0; j + 7 < width; j += 8, bitmap_p++) {
	    *gif_p++ = (*bitmap_p & 0x80) ? 0x81 : 0x80;
	    *gif_p++ = (*bitmap_p & 0x40) ? 0x81 : 0x80;
	    *gif_p++ = (*bitmap_p & 0x20) ? 0x81 : 0x80;
	    *gif_p++ = (*bitmap_p & 0x10) ? 0x81 : 0x80;
	    *gif_p++ = (*bitmap_p & 0x08) ? 0x81 : 0x80;
	    *gif_p++ = (*bitmap_p & 0x04) ? 0x81 : 0x80;
	    *gif_p++ = (*bitmap_p & 0x02) ? 0x81 : 0x80;
	    *gif_p++ = (*bitmap_p & 0x01) ? 0x81 : 0x80;
	}

	if (j < width) {
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x80) ? 0x81 : 0x80;
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x40) ? 0x81 : 0x80;
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x20) ? 0x81 : 0x80;
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x10) ? 0x81 : 0x80;
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x08) ? 0x81 : 0x80;
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x04) ? 0x81 : 0x80;
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x02) ? 0x81 : 0x80;
	    if (j++ < width)
		*gif_p++ = (*bitmap_p & 0x01) ? 0x81 : 0x80;
	    bitmap_p++;
	}
    }

    /*
     * Output a trailer.
     */
    memcpy(gif_p, "\001\011\000\073", 4);
    gif_p += 4;

    *gif_length = ((char *)gif_p - gif);
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
