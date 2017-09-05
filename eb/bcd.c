/*
 * Copyright (c) 1997, 98, 2000, 01
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

#include "build-pre.h"
#include "eb.h"
#include "error.h"
#include "build-post.h"

/*
 * Get a BCD (binary coded decimal) packed integer with 2 bytes
 * from an octet stream.
 */
unsigned
eb_bcd2(stream)
    const char *stream;
{
    unsigned value;
    const unsigned char *s = (const unsigned char *)stream;

    value  = ((*(s    ) >> 4) & 0x0f) * 1000;
    value += ((*(s    )     ) & 0x0f) * 100;
    value += ((*(s + 1) >> 4) & 0x0f) * 10;
    value += ((*(s + 1)     ) & 0x0f);

    return value;
}


/*
 * Get a BCD (binary coded decimal) packed integer with 4 bytes
 * from an octet stream.
 */
unsigned
eb_bcd4(stream)
    const char *stream;
{
    unsigned value;
    const unsigned char *s = (const unsigned char *)stream;

    value  = ((*(s    ) >> 4) & 0x0f) * 10000000;
    value += ((*(s    )     ) & 0x0f) * 1000000;
    value += ((*(s + 1) >> 4) & 0x0f) * 100000;
    value += ((*(s + 1)     ) & 0x0f) * 10000;
    value += ((*(s + 2) >> 4) & 0x0f) * 1000;
    value += ((*(s + 2)     ) & 0x0f) * 100;
    value += ((*(s + 3) >> 4) & 0x0f) * 10;
    value += ((*(s + 3)     ) & 0x0f);

    return value;
}


/*
 * Get a BCD (binary coded decimal) packed integer with 6 bytes
 * from an octet stream.
 */
unsigned
eb_bcd6(stream)
    const char *stream;
{
    unsigned value;
    const unsigned char *s = (const unsigned char *)stream;

    value  = ((*(s + 1)     ) & 0x0f);
    value += ((*(s + 2) >> 4) & 0x0f) * 10;
    value += ((*(s + 2)     ) & 0x0f) * 100;
    value += ((*(s + 3) >> 4) & 0x0f) * 1000;
    value += ((*(s + 3)     ) & 0x0f) * 10000;
    value += ((*(s + 4) >> 4) & 0x0f) * 100000;
    value += ((*(s + 4)     ) & 0x0f) * 1000000;
    value += ((*(s + 5) >> 4) & 0x0f) * 10000000;
    value += ((*(s + 5)     ) & 0x0f) * 100000000;

    return value;
}


