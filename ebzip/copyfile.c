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
#include "error.h"
#include "build-post.h"

#include "ebzip.h"
#include "ebutils.h"

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
 * Copy a file.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_copy_file(out_file_name, in_file_name)
    const char *out_file_name;
    const char *in_file_name;
{
    unsigned char buffer[EB_SIZE_PAGE];
    size_t total_length;
    struct stat in_status, out_status;
    int in_file = -1, out_file = -1;
    size_t in_length;
    int information_interval;
    int total_slices;
    int i;

    /*
     * Output file name information.
     */
    if (!ebzip_quiet_flag) {
	printf(_("==> copy %s <==\n"), in_file_name);
	printf(_("output to %s\n"), out_file_name);
	fflush(stdout);
    }

    /*
     * Check for the input file.
     */
    if (stat(in_file_name, &in_status) != 0 || !S_ISREG(in_status.st_mode)) {
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
     * When test mode, return immediately.
     */
    if (ebzip_test_flag) {
	fputc('\n', stderr);
	fflush(stderr);
	return 0;
    }

    /*
     * If the file to be output already exists, confirm and unlink it.
     */
    if (stat(out_file_name, &out_status) == 0
	&& S_ISREG(out_status.st_mode)) {
	if (ebzip_overwrite_mode == EBZIP_OVERWRITE_NO) {
	    if (!ebzip_quiet_flag) {
		fputs(_("already exists, skip the file\n\n"), stderr);
		fflush(stderr);
	    }
	    return 0;
	}
	if (ebzip_overwrite_mode == EBZIP_OVERWRITE_QUERY) {
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
    in_file = open(in_file_name, O_RDONLY | O_BINARY);
    if (in_file < 0) {
	fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
	    invoked_name, strerror(errno), in_file_name);
	goto failed;
    }

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

    /*
     * Read data from the input file, compress the data, and then
     * write them to the output file.
     */
    total_length = 0;
    total_slices = (in_status.st_size + EB_SIZE_PAGE - 1) / EB_SIZE_PAGE;
    information_interval = EBZIP_PROGRESS_INTERVAL_FACTOR;
    for (i = 0; i < total_slices; i++) {
	/*
	 * Read data from `in_file', and write them to `out_file'.
	 */
	in_length = read(in_file, buffer, EB_SIZE_PAGE);
	if (in_length < 0) {
	    fprintf(stderr, _("%s: failed to read from the file, %s: %s\n"),
		invoked_name, strerror(errno), in_file_name);
	    goto failed;
	} else if (in_length == 0) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	} else if (in_length != EB_SIZE_PAGE
	    && total_length + in_length != in_status.st_size) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"), 
		invoked_name, in_file_name);
	    goto failed;
	}

	/*
	 * Write decoded data to `out_file'.
	 */
	if (write(out_file, buffer, in_length) != in_length) {
	    fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	total_length += in_length;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!ebzip_quiet_flag
	    && i % information_interval + 1 == information_interval) {
	    printf(_("%4.1f%% done (%lu / %lu bytes)\n"),
		(double)(i + 1) * 100.0 / (double)total_slices,
		(unsigned long)total_length, (unsigned long)in_status.st_size);
	    fflush(stdout);
	}
    }

    /*
     * Output the result unless quiet mode.
     */
    if (!ebzip_quiet_flag) {
	printf(_("completed (%lu / %lu bytes)\n"),
	    (unsigned long)total_length, (unsigned long)total_length);
	fputc('\n', stdout);
	fflush(stdout);
    }

    /*
     * Close files.
     */
    close(in_file);
    in_file = -1;

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

    /*
     * Delete an original file unless the keep flag is set.
     */
    if (!ebzip_keep_flag && unlink(in_file_name) < 0) {
	fprintf(stderr, _("%s: failed to unlink the file: %s\n"), invoked_name,
	    in_file_name);
	goto failed;
    }

    /*
     * Set owner, group, permission, atime and mtime of `out_file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
#if defined(HAVE_UTIME) && defined(HAVE_STRUCT_UTIMBUF)
    {
	struct utimbuf utim;

	utim.actime = in_status.st_atime;
	utim.modtime = in_status.st_mtime;
	utime(out_file_name, &utim);
    }
#endif

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (0 <= in_file)
	close(in_file);
    if (0 <= out_file)
	close(out_file);

    fputc('\n', stderr);
    fflush(stderr);

    return -1;
}


/*
 * Copy all files in a directory.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_copy_files_in_directory(out_directory_name, in_directory_name)
    const char *out_directory_name;
    const char *in_directory_name;
{
    struct dirent *entry;
    DIR *dir = NULL;
    struct stat in_status;
    char in_path_name[PATH_MAX + 1];
    char out_path_name[PATH_MAX + 1];

    /*
     * Check for the input directory.
     */
    if (stat(in_directory_name, &in_status) != 0
	|| !S_ISDIR(in_status.st_mode))
	return 0;


    /*
     * Make the output directory if missing.
     */
    if (!ebzip_test_flag
	&& make_missing_directory(out_directory_name, 0777 ^ get_umask()) < 0)
	goto failed;

    /*
     * Open the directory `path'.
     */
    dir = opendir(in_directory_name);
    if (dir == NULL) {
	fprintf(stderr, _("%s: failed to open the directory, %s: %s\n"),
	    invoked_name, strerror(errno), in_directory_name);
        goto failed;
    }

    for (;;) {
        /*
         * Read the directory entry.
         */
        entry = readdir(dir);
        if (entry == NULL)
            break;

	eb_compose_path_name(in_directory_name, entry->d_name, in_path_name);
	eb_compose_path_name(out_directory_name, entry->d_name, out_path_name);

	if (stat(in_path_name, &in_status) != 0
	    || !S_ISREG(in_status.st_mode))
	    continue;

	ebzip_copy_file(out_path_name, in_path_name);
    }

    closedir(dir);
    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (dir != NULL)
        closedir(dir);
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
