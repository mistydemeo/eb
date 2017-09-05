/*
 * Copyright (c) 1998, 2000, 01  
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

#ifndef YESNO_H
#define YESNO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * Function declarations.
 */
#ifdef __STDC__
int query_y_or_n(const char *);
#else /* not __STDC__ */
int query_y_or_n();
#endif  /* not __STDC__ */

#endif /* not YESNO_H */
