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


/*
 * Convert a string from JIS X 0208 to EUC JP.
 */
void
eb_jisx0208_to_euc(outstr, instr)
    char *outstr;
    const char *instr;
{
    unsigned char *op = (unsigned char *)outstr;
    const unsigned char *ip = (unsigned char *)instr;

    while (*ip != '\0')
	*op++ = ((*ip++) | 0x80);

    *op = '\0';
}


/*
 * Convert a string from shift-JIS to EUC JP.
 * (Shift-JIS is used only in the `LANGUAGE' file.)
 */
void
eb_sjis_to_euc(outstr, instr)
    char *outstr;
    const char *instr;
{
    unsigned char *op = (unsigned char *)outstr;
    const unsigned char *ip = (unsigned char *)instr;
    unsigned char c1, c2;

    for (;;) {
	/*
	 * Break at '\0'.
	 */
	c1 = *ip++;
	if (c1 == '\0')
	    break;
	if (c1 <= 0x7f) {
	    *op++ = c1;
	    continue;
	}

	c2 = *ip++;
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

	*op++ = c1;
	*op++ = c2;
    }

    *op = '\0';
}


