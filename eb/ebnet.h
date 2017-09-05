/*
 * Copyright (c) 2003
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

#ifndef EBNET_H
#define EBNET_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include "eb.h"

/*
 * Trick for function protypes.
 */
#ifndef EB_P
#if defined(__STDC__) || defined(__cplusplus) || defined(WIN32)
#define EB_P(p) p
#else /* not (__STDC__ && __cplusplus && WIN32) */
#define EB_P(p) ()
#endif /* not (__STDC__ && __cplusplus && WIN32) */
#endif /* EB_P */

/*
 * Service name.
 */
#define EBNET_SERVICE_NAME		"ebnet"

/*
 * Default port number.
 */
#define EBNET_DEFAULT_PORT		"22010"

/*
 * Maximum length of book name.
 * EBNETD defins 14, but we add +4 for the ".app" suffix here.
 */
#define EBNET_MAX_BOOK_NAME_LENGTH	18

/*
 * Maximum length of book title.
 * EBNETD defins 80, and EB Library uses the same value.
 */
#define EBNET_MAX_BOOK_TITLE_LENGTH	80

/*
 * Maximum length of an EBNET request or response line.
 */
#define EBNET_MAX_LINE_LENGTH		511

/*
 * Timeout period in seconds.
 */
#define EBNET_TIMEOUT_SECONDS		30

/*
 * Function declarations.
 */
/* multiplex.c */
void ebnet_initialize_multiplex EB_P((void));
void ebnet_finalize EB_P((void));
void ebnet_set_hello_hook EB_P((int (*) EB_P((int))));
void ebnet_set_bye_hook EB_P((int (*) EB_P((int))));
int ebnet_connect_socket EB_P((const char *, int, int));
void ebnet_disconnect_socket EB_P((int));
int ebnet_reconnect_socket EB_P((int));
int ebnet_set_lost_sync EB_P((int));
int ebnet_set_book_name EB_P((int, const char *));
const char *ebnet_get_book_name EB_P((int));
int ebnet_set_file_path EB_P((int, const char *));
const char *ebnet_get_file_path EB_P((int));
int ebnet_set_offset EB_P((int, off_t));
off_t ebnet_get_offset EB_P((int));
int ebnet_set_file_size EB_P((int, size_t));
ssize_t ebnet_get_file_size EB_P((int));

/* ebnet.c */
void ebnet_initialize EB_P((void));
EB_Error_Code ebnet_get_booklist EB_P((EB_BookList *, const char *));
EB_Error_Code ebnet_bind EB_P((EB_Book *, const char *));
EB_Error_Code ebnet_bind_appendix EB_P((EB_Appendix *, const char *));
void ebnet_finalize_book EB_P((EB_Book *));
void ebnet_finalize_appendix EB_P((EB_Appendix *));
int ebnet_open EB_P((const char *));
int ebnet_close EB_P((int));
off_t ebnet_lseek EB_P((int, off_t, int));
ssize_t ebnet_read EB_P((int, char *, size_t));
EB_Error_Code ebnet_fix_directory_name EB_P((const char *, char *));
EB_Error_Code ebnet_find_file_name EB_P((const char *, const char *, char *));
EB_Error_Code ebnet_canonicalize_url EB_P((char *));

#endif /* EBNET_H */
