/*
 * Copyright (c) 1997, 98, 2000  Motoyuki Kasahara
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"

/*
 * Undefine CPP macro version.
 */
#undef eb_uint1
#undef eb_uint2
#undef eb_uint3
#undef eb_uint4

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


/*
 * Get unsigned integer with 1 byte from an octet stream.
 */
unsigned
eb_uint1(stream)
    const char *stream;
{
    const unsigned char *s = (const unsigned char *)stream;

    return *s;
}


/*
 * Get unsigned integer with 2 byte from an octet stream.
 */
unsigned
eb_uint2(stream)
    const char *stream;
{
    unsigned value;
    const unsigned char *s = (const unsigned char *)stream;

    value = (*s << 8) + (*(s + 1));
    return value;
}


/*
 * Get unsigned integer with 4 byte from an octet stream.
 */
unsigned
eb_uint4(stream)
    const char *stream;
{
    unsigned value;
    const unsigned char *s = (const unsigned char *)stream;

    value = (*s << 24) + (*(s + 1) << 16) + (*(s + 2) << 8) + (*(s + 3));
    return value;
}

