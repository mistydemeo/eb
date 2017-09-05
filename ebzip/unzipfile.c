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

#include <zlib.h>

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

#include "getumask.h"
#include "makedir.h"
#include "samefile.h"
#include "yesno.h"

#include "ebzip.h"
#include "ebutils.h"

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
 * File name to be deleted and file to be closed when signal is received.
 */
static const char *trap_file_name = NULL;
static int trap_file = -1;

/*
 * Unexported function.
 */
static RETSIGTYPE trap EB_P((int));


/*
 * Uncompress a file `in_file_name'.
 * It uncompresses the existed file nearest to the beginning of the
 * list.  If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_unzip_file(out_file_name, in_file_name, in_zio_code)
    const char *out_file_name;
    const char *in_file_name;
    Zio_Code in_zio_code;
{
    Zio in_zio;
    unsigned char *buffer = NULL;
    size_t total_length;
    int out_file = -1;
    size_t length;
    struct stat in_status, out_status;
    unsigned int crc = 1;
    int information_interval;
    int total_slices;
    int i;

    zio_initialize(&in_zio);

    /*
     * Simply copy a file, when an input file is not compressed.
     */
    if (in_zio_code == ZIO_NONE)
	return ebzip_copy_file(out_file_name, in_file_name);

    /*
     * Output file name information.
     */
    if (!ebzip_quiet_flag) {
	printf(_("==> uncompress %s <==\n"), in_file_name);
	printf(_("output to %s\n"), out_file_name);
	fflush(stdout);
    }

    /*
     * Get status of the input file.
     */
    if (stat(in_file_name, &in_status) < 0 || !S_ISREG(in_status.st_mode)) {
        fprintf(stderr, _("%s: no such file: %s\n"), invoked_name,
            in_file_name);
        goto failed;
    }

    /*
     * Do nothing if the `in_file_name' and `out_file_name' are the same.
     */
    if (is_same_file(out_file_name, in_file_name)) {
	if (!ebzip_quiet_flag) {
	    printf(_("the input and output files are the same, skipped.\n\n"));
	    fflush(stdout);
	}
	return 0;
    }

    /*
     * Allocate memories for in/out buffers.
     */
    buffer = (unsigned char *)malloc(EB_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL);
    if (buffer == NULL) {
	fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	goto failed;
    }

    /*
     * If the file `out_file_name' already exists, confirm and unlink it.
     */
    if (!ebzip_test_flag
	&& stat(out_file_name, &out_status) == 0
	&& S_ISREG(out_status.st_mode)) {
	if (ebzip_overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!ebzip_quiet_flag) {
		fputs(_("already exists, skip the file\n\n"), stderr);
		fflush(stderr);
	    }
	    return 0;
	} else if (ebzip_overwrite_mode == EBZIP_OVERWRITE_QUERY) {
	    int y_or_n;

	    fprintf(stderr, _("\nthe file already exists: %s\n"),
		out_file_name);
	    y_or_n = query_y_or_n(_("do you wish to overwrite (y or n)? "));
	    fputc('\n', stderr);
	    fflush(stderr);
	    if (!y_or_n)
		return 0;
        }
	if (unlink(out_file_name) < 0) {
	    fprintf(stderr, _("%s: failed to unlink the file: %s\n"),
		invoked_name, out_file_name);
	    goto failed;
	}
    }

    /*
     * Open files.
     */
    if (zio_open(&in_zio, in_file_name, in_zio_code) < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }
    if (!ebzip_test_flag) {
	trap_file_name = out_file_name;
#ifdef SIGHUP
	signal(SIGHUP, trap);
#endif
	signal(SIGINT, trap);
#ifdef SIGQUIT
	signal(SIGQUIT, trap);
#endif
#ifdef SIGTERM
	signal(SIGTERM, trap);
#endif

#ifdef O_CREAT
	out_file = open(out_file_name, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	    0666 ^ get_umask());
#else
	out_file = creat(out_file_name, 0666 ^ get_umask());
#endif
	if (out_file < 0) {
	    fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	trap_file = out_file;
    }

    /*
     * Read a slice from the input file, uncompress it if required,
     * and then write it to the output file.
     */
    total_length = 0;
    total_slices = (in_zio.file_size + in_zio.slice_size - 1)
	/ in_zio.slice_size;
    information_interval = EBZIP_PROGRESS_INTERVAL_FACTOR;
    for (i = 0; i < total_slices; i++) {
	/*
	 * Read the slice from `file' and unzip it, if it is zipped.
	 * We assumes the slice is not compressed if its length is
	 * equal to `slice_size'.
	 */
	if (zio_lseek(&in_zio, total_length, SEEK_SET) < 0) {
	    fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	}
	length = zio_read(&in_zio, (char *)buffer, in_zio.slice_size);
	if (length < 0) {
	    fprintf(stderr, _("%s: failed to read from the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	} else if (length == 0) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	} else if (length != in_zio.slice_size
	    && total_length + length != in_zio.file_size) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	}

	/*
	 * Update CRC.  (Calculate adler32 again.)
	 */
	if (in_zio.code == ZIO_EBZIP1)
	    crc = adler32((uLong)crc, (Bytef *)buffer, (uInt)length);

	/*
	 * Write the slice to `out_file'.
	 */
	if (!ebzip_test_flag) {
	    if (write(out_file, buffer, length) != length) {
		fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	}
	total_length += length;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!ebzip_quiet_flag
	    && i % information_interval + 1 == information_interval) {
	    printf(_("%4.1f%% done (%lu / %lu bytes)\n"),
		(double)(i + 1) * 100.0 / (double)total_slices,
		(unsigned long)total_length, (unsigned long)in_zio.file_size);
	    fflush(stdout);
	}
    }

    /*
     * Output the result unless quiet mode.
     */
    if (!ebzip_quiet_flag) {
	printf(_("completed (%lu / %lu bytes)\n"),
	    (unsigned long)in_zio.file_size, (unsigned long)in_zio.file_size);
	printf(_("%lu -> %lu bytes\n\n"),
	    (unsigned long)in_status.st_size, (unsigned long)total_length);
	fflush(stdout);
    }

    /*
     * Close files.
     */
    zio_close(&in_zio);
    zio_finalize(&in_zio);

    if (!ebzip_test_flag) {
	close(out_file);
	out_file = -1;
	trap_file = -1;
	trap_file_name = NULL;
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
    }

    /*
     * Check for CRC.
     */
    if (in_zio.code == ZIO_EBZIP1 && in_zio.crc != crc) {
	fprintf(stderr, _("%s: CRC error: %s\n"), invoked_name, out_file_name);
	goto failed;
    }

    /*
     * Delete an original file unless the keep flag is set.
     */
    if (!ebzip_test_flag && !ebzip_keep_flag && unlink(in_file_name) < 0) {
	fprintf(stderr, _("%s: failed to unlink the file: %s\n"), invoked_name,
	    in_file_name);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and mtime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
#if defined(HAVE_UTIME)
    if (!ebzip_test_flag) {
	struct utimbuf utim;

	utim.actime = in_status.st_atime;
	utim.modtime = in_status.st_mtime;
	utime(out_file_name, &utim);
    }
#endif

    /*
     * Dispose memories.
     */
    free(buffer);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (buffer != NULL)
	free(buffer);

    zio_close(&in_zio);
    zio_finalize(&in_zio);

    if (0 <= out_file) {
	close(out_file);
	trap_file = -1;
	trap_file_name = NULL;
#ifdef SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
	signal(SIGINT, SIG_DFL);
#ifdef SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
    }

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}


/*
 * Signal handler.
 */
static RETSIGTYPE
trap(signal_number)
    int signal_number;
{
    if (0 <= trap_file)
	close(trap_file);
    if (trap_file_name != NULL)
	unlink(trap_file_name);
    
    exit(1);

    /* not reached */
#ifndef RETSIGTYPE_VOID
    return 0;
#endif
}
