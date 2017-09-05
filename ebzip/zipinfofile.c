/*                                                            -*- C -*-
 * Copyright (c) 1998, 99, 2000, 01  
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

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

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
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
 * stat macros.
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
 * The maximum length of path name.
 */
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX        MAXPATHLEN
#else /* not MAXPATHLEN */
#define PATH_MAX        1024
#endif /* not MAXPATHLEN */
#endif /* not PATH_MAX */

#include "eb.h"
#include "internal.h"

#include "ebutils.h"
#include "samefile.h"
#include "yesno.h"

#include "ebzip.h"

/*
 * Trick for function protypes.
 */
#ifndef EB_P
#ifdef __STDC__
#define EB_P(p) p
#else /* not __STDC__ */
#define EB_P(p) ()
#endif /* not __STDC__ */
#endif /* EB_P */

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

/*
 * Output status of a file `file_name'.
 * It outputs status of the existed file nearest to the beginning of
 * the list.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_zipinfo_file(in_file_name, in_zio_code)
    const char *in_file_name;
    Zio_Code in_zio_code;
{
    Zio in_zio;
    int in_file = -1;
    struct stat in_status;

    /*
     * Output file name information.
     */
    printf("==> %s <==\n", in_file_name);
    fflush(stdout);

    /*
     * Open the file.
     */
    if (stat(in_file_name, &in_status) == 0 && S_ISREG(in_status.st_mode))
	in_file = zio_open(&in_zio, in_file_name, in_zio_code);

    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }

    /*
     * Close the file.
     */
    close(in_file);

    /*
     * Output information.
     */
    if (in_zio.code == ZIO_PLAIN) {
	printf(_("%lu bytes (not compressed)\n"),
	    (unsigned long)in_status.st_size);
    } else {
	printf(_("%lu -> %lu bytes "),
	    (unsigned long)in_zio.file_size, (unsigned long)in_status.st_size);
	if (in_zio.file_size == 0)
	    fputs(_("(empty original file, "), stdout);
	else {
	    printf("(%4.1f%%, ", (double)in_status.st_size * 100.0
		/ (double)in_zio.file_size);
	}
	if (in_zio.code == ZIO_EBZIP1)
	    printf(_("ebzip level %d compression)\n"), in_zio.zip_level);
	else
	    printf(_("EPWING compression)\n"));
    }

    fputc('\n', stdout);
    fflush(stdout);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= in_file)
	close(in_file);

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}
