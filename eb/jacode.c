/*
 * Copyright (c) 1997, 99, 2000  Motoyuki Kasahara
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

/*
 * Convert a string from JIS X 0208 to EUC JP.
 */
void
eb_jisx0208_to_euc(out_string, in_string)
    char *out_string;
    const char *in_string;
{
    unsigned char *out_p = (unsigned char *)out_string;
    const unsigned char *in_p = (unsigned char *)in_string;

    while (*in_p != '\0')
	*out_p++ = ((*in_p++) | 0x80);

    *out_p = '\0';
}


/*
 * Convert a string from shift-JIS to EUC JP.
 * (Shift-JIS is used only in the `LANGUAGE' file.)
 */
void
eb_sjis_to_euc(out_string, in_string)
    char *out_string;
    const char *in_string;
{
    unsigned char *out_p = (unsigned char *)out_string;
    const unsigned char *in_p = (unsigned char *)in_string;
    unsigned char c1, c2;

    for (;;) {
	/*
	 * Break at '\0'.
	 */
	c1 = *in_p++;
	if (c1 == '\0')
	    break;

	if (c1 <= 0x7f) {
	    /*
	     * JIS X 0201 Roman character.
	     */
	    *out_p++ = c1;
	} else if (0xa1 <= c1 && c1 <= 0xdf) {
	    /*
	     * JIS X 0201 Kana.
	     */
	    *out_p++ = ' ';
	} else {
	    /*
	     * JIS X 0208 character.
	     */
	    c2 = *in_p++;
	    if (c2 == 0x00)
		break;

	    if (c2 < 0x9f) {
		if (c1 < 0xdf)
		    c1 = ((c1 - 0x30) << 1) - 1;
		else
		    c1 = ((c1 - 0x70) << 1) - 1;

		if (c2 < 0x7f)
		    c2 += 0x61;
		else
		    c2 += 0x60;
	    } else {
		if (c1 < 0xdf)
		    c1 = (c1 - 0x30) << 1;
		else
		    c1 = (c1 - 0x70) << 1;
		c2 += 0x02;
	    }

	    *out_p++ = c1;
	    *out_p++ = c2;
	}
    }

    *out_p = '\0';
}


