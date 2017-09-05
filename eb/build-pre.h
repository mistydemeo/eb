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

#ifndef EB_BUILD_PRE_H
#define EB_BUILD_PRE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__STDC__) || defined(MSVC)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

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

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else /* not HAVE_DIRENT_H */
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif /* HAVE_SYS_NDIR_H */
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif /* HAVE_SYS_DIR_H */
#if HAVE_NDIR_H
#include <ndir.h>
#endif /* HAVE_NDIR_H */
#endif /* not HAVE_DIRENT_H */

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#if defined(HAVE_DIRECT_H) && defined(HAVE__GETDCWD)
#include <direct.h>            /* for _getcwd(), _getdcwd() */
#define getcwd _getcwd
#define getdcwd _getdcwd
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

/*
 * strchr() and strrchr().
 */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

/*
 * memcpy(), memchr(), memcmp(), memmove() and memset().
 */
#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef PROTOTYPES
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif
#endif

/*
 * Mutual exclusion lock of Pthreads.
 */
#ifndef ENABLE_PTHREAD
#define pthread_mutex_lock(m)
#define pthread_mutex_unlock(m)
#endif

/*
 * stat() macros.
 */
#ifdef  STAT_MACROS_BROKEN
#ifdef  S_ISREG
#undef  S_ISREG
#endif
#ifdef  S_ISDIR
#undef  S_ISDIR
#endif
#endif  /* STAT_MACROS_BROKEN */

#ifndef S_ISREG
#define S_ISREG(m)   (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m)   (((m) & S_IFMT) == S_IFDIR)
#endif

/*
 * Whence parameter for lseek().
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/*
 * Flags for open().
 */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * Character type tests and conversions.
 */
#define ASCII_ISDIGIT(c) ('0' <= (c) && (c) <= '9')
#define ASCII_ISUPPER(c) ('A' <= (c) && (c) <= 'Z')
#define ASCII_ISLOWER(c) ('a' <= (c) && (c) <= 'z')
#define ASCII_ISALPHA(c) \
 (ASCII_ISUPPER(c) || ASCII_ISLOWER(c))
#define ASCII_ISALNUM(c) \
 (ASCII_ISUPPER(c) || ASCII_ISLOWER(c) || ASCII_ISDIGIT(c))
#define ASCII_ISXDIGIT(c) \
 (ASCII_ISDIGIT(c) || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define ASCII_TOUPPER(c) (('a' <= (c) && (c) <= 'z') ? (c) - 0x20 : (c))
#define ASCII_TOLOWER(c) (('A' <= (c) && (c) <= 'Z') ? (c) + 0x20 : (c))

/*
 * For (void *).
 */
#ifndef VOID
#if defined(__STDC__) || defined(__cplusplus)
#define VOID void
#else
#define VOID char
#endif
#endif

/*
 * Tricks for gettext.
 */
#ifdef ENABLE_NLS
#define _(string) gettext(string)
#ifdef gettext_noop
#define N_(string) gettext_noop(string)
#else
#define N_(string) (string)
#endif
#else
#define _(string) (string)
#define N_(string) (string)
#endif

#ifndef HAVE_GETCWD
#define getcwd(d,n) getwd(d)
#endif

/*
 * Trick for function protypes.
 */
#ifndef EB_P
#ifdef PROTOTYPES
#define EB_P(p) p
#else
#define EB_P(p)
#endif
#endif

/*
 * Fake missing function names.
 */
#ifndef HAVE_MEMMOVE
#define memmove eb_memmove
#endif

#ifndef HAVE_STRCASECMP
#define strcasecmp eb_strcasecmp
#define strncasecmp eb_strncasecmp
#endif

#ifndef HAVE_GETADDRINFO
#define addrinfo ebnet_addrinfo
#define getaddrinfo ebnet_getaddrinfo
#define freeaddrinfo ebnet_freeaddrinfo
#endif

#ifndef HAVE_GETNAMEINFO
#define getnameinfo ebnet_getnameinfo
#endif

#ifndef HAVE_GAI_STRERROR
#define gai_strerror ebnet_gai_strerror
#endif

#ifndef IN6ADDR_ANY_DECLARED
#define in6addr_any ebnet_in6addr_any
#endif

#ifndef IN6ADDR_LOOPBACK_DECLARED
#define in6addr_loopback ebnet_in6addr_loopback
#endif

#endif /* EB_BUILD_PRE_H */
