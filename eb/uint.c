/*
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

/*
 * Get a BCD (binary coded decimal) packed integer with 2 bytes
 * from an octet stream.
 */
unsigned
eb_bcd2(mem)
    const char *mem;
{
    unsigned val;
    const unsigned char *umem = (const unsigned char *)mem;

    val  = ((*(umem    ) >> 4) & 0x0f) * 1000;
    val += ((*(umem    )     ) & 0x0f) * 100;
    val += ((*(umem + 1) >> 4) & 0x0f) * 10;
    val += ((*(umem + 1)     ) & 0x0f);

    return val;
}


/*
 * Get a BCD (binary coded decimal) packed integer with 4 bytes
 * from an octet stream.
 */
unsigned
eb_bcd4(mem)
    const char *mem;
{
    unsigned val;
    const unsigned char *umem = (const unsigned char *)mem;

    val  = ((*(umem    ) >> 4) & 0x0f) * 10000000;
    val += ((*(umem    )     ) & 0x0f) * 1000000;
    val += ((*(umem + 1) >> 4) & 0x0f) * 100000;
    val += ((*(umem + 1)     ) & 0x0f) * 10000;
    val += ((*(umem + 2) >> 4) & 0x0f) * 1000;
    val += ((*(umem + 2)     ) & 0x0f) * 100;
    val += ((*(umem + 3) >> 4) & 0x0f) * 10;
    val += ((*(umem + 3)     ) & 0x0f);

    return val;
}


/*
 * Get a BCD (binary coded decimal) packed integer with 6 bytes
 * from an octet stream.
 */
unsigned
eb_bcd6(mem)
    const char *mem;
{
    unsigned val;
    const unsigned char *umem = (const unsigned char *)mem;

    val  = ((*(umem + 1)     ) & 0x0f);
    val += ((*(umem + 2) >> 4) & 0x0f) * 10;
    val += ((*(umem + 2)     ) & 0x0f) * 100;
    val += ((*(umem + 3) >> 4) & 0x0f) * 1000;
    val += ((*(umem + 3)     ) & 0x0f) * 10000;
    val += ((*(umem + 4) >> 4) & 0x0f) * 100000;
    val += ((*(umem + 4)     ) & 0x0f) * 1000000;
    val += ((*(umem + 5) >> 4) & 0x0f) * 10000000;
    val += ((*(umem + 5)     ) & 0x0f) * 100000000;

    return val;
}


/*
 * Get unsigned integer with 1 byte from an octet stream.
 */
unsigned
eb_uint1(mem)
    const char *mem;
{
    const unsigned char *umem = (const unsigned char *)mem;

    return *umem;
}


/*
 * Get unsigned integer with 2 byte from an octet stream.
 */
unsigned
eb_uint2(mem)
    const char *mem;
{
    unsigned value;
    const unsigned char *umem = (const unsigned char *)mem;

    value = (*umem << 8) + (*(umem + 1));
    return value;
}


/*
 * Get unsigned integer with 4 byte from an octet stream.
 */
unsigned
eb_uint4(mem)
    const char *mem;
{
    unsigned value;
    const unsigned char *umem = (const unsigned char *)mem;

    value = (*umem << 24) + (*(umem + 1) << 16) + (*(umem + 2) << 8)
	+ (*(umem + 3));
    return value;
}

