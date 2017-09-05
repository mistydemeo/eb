/*                                                            -*- C -*-
 * Copyright (c) 1998-2006  Motoyuki Kasahara
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "ebzip.h"

#include "getumask.h"
#include "makedir.h"
#include "samefile.h"
#include "yesno.h"

/*
 * File name to be deleted and file to be closed when signal is received.
 */
static const char *trap_file_name = NULL;
static int trap_file = -1;

/*
 * Unexported function.
 */
static int ebzip_zip_file_internal(const char *out_file_name,
    const char *in_file_name, Zio_Code in_zio_code, int index_page,
    Zip_Speedup *speedup);
static void trap(int signal_number);


/*
 * Ccompress a file `in_file_name'.
 * For START file, use ebzip_zip_start_file() instead.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_zip_file(const char *out_file_name, const char *in_file_name,
    Zio_Code in_zio_code, Zip_Speedup *speedup)
{
    return ebzip_zip_file_internal(out_file_name, in_file_name,
	in_zio_code, 0, speedup);
}

/*
 * Compress TART file `in_file_name'.
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
int
ebzip_zip_start_file(const char *out_file_name, const char *in_file_name,
    Zio_Code in_zio_code, int index_page, Zip_Speedup *speedup)
{
    return ebzip_zip_file_internal(out_file_name, in_file_name,
	in_zio_code, index_page, speedup);
}

/*
 * Internal function for zip_unzip_file() and ebzip_zip_sebxa_start().
 * If it succeeds, 0 is returned.  Otherwise -1 is returned.
 */
static int
ebzip_zip_file_internal(const char *out_file_name, const char *in_file_name,
    Zio_Code in_zio_code, int index_page, Zip_Speedup *speedup)
{
    Zio in_zio, out_zio;
    unsigned char *in_buffer = NULL, *out_buffer = NULL;
    off_t in_total_length, out_total_length;
    ssize_t in_length;
    size_t out_length;
    struct stat in_status, out_status;
    off_t slice_location = 0;
    off_t next_location;
    size_t index_length;
    int progress_interval;
    int total_slices;
    int i;

    zio_initialize(&in_zio);
    zio_initialize(&out_zio);

    /*
     * Output information.
     */
    if (!ebzip_quiet_flag) {
	fprintf(stderr, _("==> compress %s <==\n"), in_file_name);
	fprintf(stderr, _("output to %s\n"), out_file_name);
	fflush(stderr);
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
	    fprintf(stderr,
		_("the input and output files are the same, skipped.\n\n"));
	    fflush(stderr);
	}
	return 0;
    }

    /*
     * Allocate memories for in/out buffers.
     */
    in_buffer = (unsigned char *)malloc(EB_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL);
    if (in_buffer == NULL) {
	fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	goto failed;
    }

    out_buffer = (unsigned char *) malloc((EB_SIZE_PAGE << ZIO_MAX_EBZIP_LEVEL)
	+ ZIO_SIZE_EBZIP_MARGIN);
    if (out_buffer == NULL) {
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
	} else if (ebzip_overwrite_mode == EBZIP_OVERWRITE_CONFIRM) {
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
	fprintf(stderr, _("%s: failed to open the file: %s\n"),
	    invoked_name, in_file_name);
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
	out_zio.file = open(out_file_name,
	    O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0666 ^ get_umask());
#else
	out_zio.file = creat(out_file_name, 0666 ^ get_umask());
#endif
	if (out_zio.file < 0) {
	    fprintf(stderr, _("%s: failed to open the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	trap_file = out_zio.file;
    }

    /*
     * Initialize `zip'.
     */
    out_zio.code = ZIO_EBZIP1;
    out_zio.slice_size = EB_SIZE_PAGE << ebzip_level;
    out_zio.file_size = in_zio.file_size;
    out_zio.crc = 1;
    out_zio.mtime = in_status.st_mtime;

    if (out_zio.file_size      < (off_t) 1 << 16)
	out_zio.index_width = 2;
    else if (out_zio.file_size < (off_t) 1 << 24)
	out_zio.index_width = 3;
    else if (out_zio.file_size < (off_t) 1 << 32 || !off_t_is_large)
	out_zio.index_width = 4;
    else
	out_zio.index_width = 5;

    /*
     * Fill header and index part with `\0'.
     *
     * Original File:
     *   +-----------------+-----------------+-....-+-------+
     *   |     slice 1     |     slice 2     |      |slice N| [EOF]
     *   |                 |                 |      |       |
     *   +-----------------+-----------------+-....-+-------+
     *        slice_size        slice_size            odds
     *   <-------------------- file size ------------------->
     *
     * Compressed file:
     *   +------+---------+...+---------+---------+----------+...+-
     *   |Header|index for|   |index for|index for|compressed|   |
     *   |      | slice 1 |   | slice N |   EOF   |  slice 1 |   |
     *   +------+---------+...+---------+---------+----------+...+-
     *             index         index     index
     *             width         width     width
     *          <---------  index_length --------->
     *
     *     total_slices = N = (file_size + slice_size - 1) / slice_size
     *     index_length = (N + 1) * index_width
     */
    total_slices = (out_zio.file_size + out_zio.slice_size - 1)
	/ out_zio.slice_size;
    index_length = (total_slices + 1) * out_zio.index_width;
    memset(out_buffer, '\0', out_zio.slice_size);

    if (!ebzip_test_flag) {
	for (i = index_length + ZIO_SIZE_EBZIP_HEADER;
	     out_zio.slice_size <= i; i -= out_zio.slice_size) {
	    if (write(out_zio.file, out_buffer, out_zio.slice_size)
		!= out_zio.slice_size) {
		fprintf(stderr, _("%s: failed to write to the file: %s\n"),
		    invoked_name, out_file_name);
		goto failed;
	    }
	}
	if (0 < i) {
	    if (write(out_zio.file, out_buffer, i) != i) {
		fprintf(stderr, _("%s: failed to write to the file: %s\n"),
		    invoked_name, out_file_name);
		goto failed;
	    }
	}
    }

    /*
     * Read a slice from the input file, compress it, and then
     * write it to the output file.
     */
    in_total_length = 0;
    out_total_length = 0;
    progress_interval = EBZIP_PROGRESS_INTERVAL_FACTOR >> ebzip_level;
    if (((total_slices + 999) / 1000) > progress_interval)
	progress_interval = ((total_slices + 999) / 1000);

    for (i = 0; i < total_slices; i++) {
	/*
	 * Read a slice from the original file.
	 */
	if (zio_lseek(&in_zio, in_total_length, SEEK_SET) < 0) {
	    fprintf(stderr, _("%s: failed to seek the file: %s\n"),
		invoked_name, in_file_name);
	    goto failed;
	}
	in_length = zio_read(&in_zio, (char *)in_buffer, out_zio.slice_size);
	if (in_length < 0) {
	    fprintf(stderr, _("%s: failed to read from the file: %s\n"),
		invoked_name, in_file_name);
	    goto failed;
	} else if (in_length == 0) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"),
		invoked_name, in_file_name);
	    goto failed;
	} else if (in_length != out_zio.slice_size
	    && in_total_length + in_length != out_zio.file_size) {
	    fprintf(stderr, _("%s: unexpected EOF: %s\n"),
		invoked_name, in_file_name);
	    goto failed;
	}

	/*
	 * Update CRC.  (Calculate adler32 again.)
	 */
	out_zio.crc = adler32((uLong)out_zio.crc, (Bytef *)in_buffer,
	    (uInt)in_length);

	/*
	 * If this is last slice and its length is shorter than
	 * `slice_size', fill `\0'.
	 */
	if (in_length < out_zio.slice_size) {
	    memset(in_buffer + in_length, '\0',
		out_zio.slice_size - in_length);
	    in_length = out_zio.slice_size;
	}

	/*
	 * Compress the slice.
	 */
	if (speedup != NULL
	    && ebzip_is_speedup_slice(speedup, i, ebzip_level)) {
	    out_length = out_zio.slice_size;
	} else if (ebzip1_slice((char *)out_buffer, &out_length,
	    (char *)in_buffer, out_zio.slice_size) < 0) {
	    fprintf(stderr, _("%s: memory exhausted\n"), invoked_name);
	    goto failed;
	}
	if (out_zio.slice_size <= out_length) {
	    memcpy(out_buffer, in_buffer, out_zio.slice_size);
	    out_length = out_zio.slice_size;
	}

	/*
	 * Write the slice to the zip file.
	 * If the length of the zipped slice is not shorter than
	 * original, write orignal slice.
	 */
	if (!ebzip_test_flag) {
	    slice_location = lseek(out_zio.file, 0, SEEK_END);
	    if (slice_location < 0) {
		fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	    if (write(out_zio.file, out_buffer, out_length) != out_length) {
		fprintf(stderr, _("%s: failed to write to the file: %s\n"),
		    invoked_name, out_file_name);
		goto failed;
	    }
	}

	/*
	 * Write an index for the slice.
	 */
	next_location = slice_location + out_length;
	switch (out_zio.index_width) {
	case 2:
	    out_buffer[0] = (slice_location >>  8) & 0xff;
	    out_buffer[1] =  slice_location        & 0xff;
	    out_buffer[2] = (next_location  >>  8) & 0xff;
	    out_buffer[3] =  next_location         & 0xff;
	    break;
	case 3:
	    out_buffer[0] = (slice_location >> 16) & 0xff;
	    out_buffer[1] = (slice_location >>  8) & 0xff;
	    out_buffer[2] =  slice_location        & 0xff;
	    out_buffer[3] = (next_location  >> 16) & 0xff;
	    out_buffer[4] = (next_location  >>  8) & 0xff;
	    out_buffer[5] =  next_location         & 0xff;
	    break;
	case 4:
	    out_buffer[0] = (slice_location >> 24) & 0xff;
	    out_buffer[1] = (slice_location >> 16) & 0xff;
	    out_buffer[2] = (slice_location >>  8) & 0xff;
	    out_buffer[3] =  slice_location        & 0xff;
	    out_buffer[4] = (next_location  >> 24) & 0xff;
	    out_buffer[5] = (next_location  >> 16) & 0xff;
	    out_buffer[6] = (next_location  >>  8) & 0xff;
	    out_buffer[7] =  next_location         & 0xff;
	    break;
	case 5:
	    out_buffer[0] = (slice_location >> 32) & 0xff;
	    out_buffer[1] = (slice_location >> 24) & 0xff;
	    out_buffer[2] = (slice_location >> 16) & 0xff;
	    out_buffer[3] = (slice_location >>  8) & 0xff;
	    out_buffer[4] =  slice_location        & 0xff;
	    out_buffer[5] = (next_location >>  32) & 0xff;
	    out_buffer[6] = (next_location >>  24) & 0xff;
	    out_buffer[7] = (next_location >>  16) & 0xff;
	    out_buffer[8] = (next_location >>   8) & 0xff;
	    out_buffer[9] =  next_location         & 0xff;
	    break;
	}

	if (!ebzip_test_flag) {
	    if (lseek(out_zio.file,
		ZIO_SIZE_EBZIP_HEADER + (off_t) i * out_zio.index_width,
		SEEK_SET) < 0) {
		fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	    if (write(out_zio.file, out_buffer, out_zio.index_width * 2)
		!= out_zio.index_width * 2) {
		fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		    invoked_name, strerror(errno), out_file_name);
		goto failed;
	    }
	}

	in_total_length += in_length;
	out_total_length += out_length + out_zio.index_width;

	/*
	 * Output status information unless `quiet' mode.
	 */
	if (!ebzip_quiet_flag && (i + 1) % progress_interval == 0) {
#if defined(PRINTF_LL_MODIFIER)
	    fprintf(stderr, _("%4.1f%% done (%llu / %llu bytes)\n"),
		(double) (i + 1) * 100.0 / (double) total_slices,
		(unsigned long long) in_total_length,
		(unsigned long long) in_zio.file_size);
#elif defined(PRINTF_I64_MODIFIER)
	    fprintf(stderr, _("%4.1f%% done (%I64u / %I64u bytes)\n"),
		(double) (i + 1) * 100.0 / (double) total_slices,
		(unsigned __int64) in_total_length,
		(unsigned __int64) in_zio.file_size);
#else
	    fprintf(stderr, _("%4.1f%% done (%lu / %lu bytes)\n"),
		(double) (i + 1) * 100.0 / (double) total_slices,
		(unsigned long) in_total_length,
		(unsigned long) in_zio.file_size);
#endif
	    fflush(stderr);
	}
    }

    /*
     * Write a header part (22 bytes):
     *     magic-id		5   bytes  ( 0 ...  4)
     *     zip-mode		4/8 bytes  ( 5)
     *     slice_size		4/8 bytes  ( 5)
     *     (reserved)		4   bytes  ( 6 ...  9)
     *     file_size		4   bytes  (10 ... 13)
     *     crc			4   bytes  (14 ... 17)
     *     mtime		4   bytes  (18 ... 21)
     */
    memcpy(out_buffer, "EBZip", 5);

    if (out_zio.file_size < (off_t) 1 << 32 || !off_t_is_large)
	out_buffer[5] = (1 << 4) + (ebzip_level & 0x0f);
    else
	out_buffer[5] = (2 << 4) + (ebzip_level & 0x0f);
    out_buffer[ 6] = 0;
    out_buffer[ 7] = 0;
    out_buffer[ 8] = 0;
    out_buffer[ 9] = (out_zio.file_size >> 32) & 0xff;
    out_buffer[10] = (out_zio.file_size >> 24) & 0xff;
    out_buffer[11] = (out_zio.file_size >> 16) & 0xff;
    out_buffer[12] = (out_zio.file_size >> 8) & 0xff;
    out_buffer[13] = out_zio.file_size & 0xff;
    out_buffer[14] = (out_zio.crc >> 24) & 0xff;
    out_buffer[15] = (out_zio.crc >> 16) & 0xff;
    out_buffer[16] = (out_zio.crc >> 8) & 0xff;
    out_buffer[17] = out_zio.crc & 0xff;
    out_buffer[18] = (out_zio.mtime >> 24) & 0xff;
    out_buffer[19] = (out_zio.mtime >> 16) & 0xff;
    out_buffer[20] = (out_zio.mtime >> 8) & 0xff;
    out_buffer[21] = out_zio.mtime & 0xff;

    if (!ebzip_test_flag) {
	if (lseek(out_zio.file, 0, SEEK_SET) < 0) {
	    fprintf(stderr, _("%s: failed to seek the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
	if (write(out_zio.file, out_buffer, ZIO_SIZE_EBZIP_HEADER)
	    != ZIO_SIZE_EBZIP_HEADER) {
	    fprintf(stderr, _("%s: failed to write to the file, %s: %s\n"),
		invoked_name, strerror(errno), out_file_name);
	    goto failed;
	}
    }

    /*
     * Output the result information unless quiet mode.
     */
    out_total_length += ZIO_SIZE_EBZIP_HEADER + out_zio.index_width;

    if (!ebzip_quiet_flag) {
#if defined(PRINTF_LL_MODIFIER)
	fprintf(stderr, _("completed (%llu / %llu bytes)\n"),
	    (unsigned long long) in_zio.file_size,
	    (unsigned long long) in_zio.file_size);
	if (in_total_length != 0) {
	    fprintf(stderr, _("%llu -> %llu bytes (%4.1f%%)\n\n"),
		(unsigned long long) in_zio.file_size,
		(unsigned long long) out_total_length,
		(double) out_total_length * 100.0 / (double) in_zio.file_size);
	} else {
	    fprintf(stderr, _("%llu -> %llu bytes\n\n"),
		(unsigned long long) in_zio.file_size,
		(unsigned long long) out_total_length);
	}
#elif defined(PRINTF_I64_MODIFIER)
	fprintf(stderr, _("completed (%I64u / %I64u bytes)\n"),
	    (unsigned __int64) in_zio.file_size,
	    (unsigned __int64) in_zio.file_size);
	if (in_total_length != 0) {
	    fprintf(stderr, _("%I64u -> %I64u bytes (%4.1f%%)\n\n"),
		(unsigned __int64) in_zio.file_size,
		(unsigned __int64) out_total_length,
		(double) out_total_length * 100.0 / (double) in_zio.file_size);
	} else {
	    fprintf(stderr, _("%I64u -> %I64u bytes\n\n"),
		(unsigned __int64) in_zio.file_size,
		(unsigned __int64) out_total_length);
	}
#else
	fprintf(stderr, _("completed (%lu / %lu bytes)\n"),
	    (unsigned long) in_zio.file_size,
	    (unsigned long) in_zio.file_size);
	if (in_total_length != 0) {
	    fprintf(stderr, _("%lu -> %lu bytes (%4.1f%%)\n\n"),
		(unsigned long) in_zio.file_size,
		(unsigned long) out_total_length,
		(double) out_total_length * 100.0 / (double) in_zio.file_size);
	} else {
	    fprintf(stderr, _("%lu -> %lu bytes\n\n"),
		(unsigned long) in_zio.file_size,
		(unsigned long) out_total_length);
	}
#endif
	fflush(stderr);
    }

    /*
     * Close files.
     */
    zio_close(&in_zio);
    zio_finalize(&in_zio);

    if (!ebzip_test_flag) {
	close(out_zio.file);
	out_zio.file = -1;
	zio_finalize(&out_zio);
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
     * Delete an original file unless the keep flag is set.
     */
    if (!ebzip_test_flag && !ebzip_keep_flag)
	unlink_files_add(in_file_name);

    /*
     * Set owner, group, permission, atime and utime of `out_zio.file'.
     * We ignore return values of `chown', `chmod' and `utime'.
     */
    if (!ebzip_test_flag) {
	struct utimbuf utim;

	utim.actime = in_status.st_atime;
	utim.modtime = in_status.st_mtime;
	utime(out_file_name, &utim);
    }

    /*
     * Dispose memories.
     */
    free(in_buffer);
    free(out_buffer);

    return 0;

    /*
     * An error occurs...
     */
  failed:
    if (in_buffer != NULL)
	free(in_buffer);
    if (out_buffer != NULL)
	free(out_buffer);

    zio_close(&in_zio);
    zio_finalize(&in_zio);

    if (0 <= out_zio.file) {
	close(out_zio.file);
	out_zio.file = -1;
	zio_finalize(&out_zio);
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
static void
trap(int signal_number)
{
    if (0 <= trap_file)
	close(trap_file);
    if (trap_file_name != NULL)
	unlink(trap_file_name);

    exit(1);
}
