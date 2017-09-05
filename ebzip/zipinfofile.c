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

#include "eb.h"
#include "build-post.h"

#include "ebzip.h"

#include "samefile.h"
#include "yesno.h"

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
 * Unexported functions.
 */
static int ebzip_zipinfo_file_internal EB_P((const char *, Zio_Code, int));


/*
 * Output status of a file `in_file_name'.
 * For START file, use ebzip_zipinfo_start_file() instead.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_zipinfo_file(in_file_name, in_zio_code)
    const char *in_file_name;
    Zio_Code in_zio_code;
{
    return ebzip_zipinfo_file_internal(in_file_name, in_zio_code, 0);
}

/*
 * Output status of START file `in_file_name'.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_zipinfo_start_file(in_file_name, in_zio_code, index_page)
    const char *in_file_name;
    Zio_Code in_zio_code;
    int index_page;
{
    return ebzip_zipinfo_file_internal(in_file_name, in_zio_code, index_page);
}

/*
 * Output status of a file `file_name'.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
ebzip_zipinfo_file_internal(in_file_name, in_zio_code, index_page)
    const char *in_file_name;
    Zio_Code in_zio_code;
    int index_page;
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
    zio_initialize(&in_zio);
    if (stat(in_file_name, &in_status) == 0 && S_ISREG(in_status.st_mode))
	in_file = zio_open(&in_zio, in_file_name, in_zio_code);

    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }
    if (in_zio_code == ZIO_SEBXA) {
	off_t index_location;
	off_t index_base;
	off_t zio_start_location;
	off_t zio_end_location;

	if (get_sebxa_indexes(in_file_name, index_page, &index_location,
	    &index_base, &zio_start_location, &zio_end_location) < 0) {
	    goto failed;
	}
	zio_set_sebxa_mode(&in_zio, index_location, index_base,
	    zio_start_location, zio_end_location);
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
	else if (in_zio.code == ZIO_SEBXA)
	    printf(_("S-EBXA compression)\n"));
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
