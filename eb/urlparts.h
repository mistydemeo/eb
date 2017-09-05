/*
 * Copyright (c) 2000, 01
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

#ifndef URLPARTS_H
#define URLPARTS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * URL Parts.
 */
typedef struct {
    char *url;
    char *scheme;
    char *user;
    char *password;
    char *host;
    char *port;
    char *path;
    char *params;
    char *query;
    char *fragment;
    char *buffer;
} URL_Parts;

/*
 * Function Declarations.
 */
#ifdef __STDC__
void url_parts_initialize(URL_Parts *);
void url_parts_finalize(URL_Parts *);
const char *url_parts_url(URL_Parts *);
const char *url_parts_scheme(URL_Parts *);
const char *url_parts_user(URL_Parts *);
const char *url_parts_password(URL_Parts *);
const char *url_parts_host(URL_Parts *);
const char *url_parts_port(URL_Parts *);
const char *url_parts_path(URL_Parts *);
const char *url_parts_params(URL_Parts *);
const char *url_parts_query(URL_Parts *);
const char *url_parts_fragment(URL_Parts *);
int url_parts_parse(URL_Parts *, const char *);
void url_parts_print(URL_Parts *);
#else /* not __STDC__ */
void url_parts_initialize();
void url_parts_finalize();
const char *url_parts_url();
const char *url_parts_scheme();
const char *url_parts_user();
const char *url_parts_password();
const char *url_parts_host();
const char *url_parts_port();
const char *url_parts_path();
const char *url_parts_params();
const char *url_parts_query();
const char *url_parts_fragment();
int url_parts_parse();
void url_parts_print();
#endif /* not __STDC__ */

#endif /* not URLPARTS_H */
