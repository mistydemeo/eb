/*
 * Copyright (c) 1997, 2000  Motoyuki Kasahara
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
#include <sys/stat.h>

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

/* for Visual C++ by KSK Jan/30/1998 */
#if defined(HAVE_DIRECT_H) && defined(HAVE__GETDCWD)
#include <direct.h>            /* for _getcwd(), _getdcwd() */
#define getcwd _getcwd
#define getdcwd _getdcwd
#endif

#include "eb.h"
#include "error.h"
#include "internal.h"

#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif /* HAVE_STRCHR */

#ifndef HAVE_MEMCPY
#define memcpy(d, s, n) bcopy((s), (d), (n))
#ifdef __STDC__
void *memchr(const void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
#else /* not __STDC__ */
char *memchr();
int memcmp();
char *memmove();
char *memset();
#endif /* not __STDC__ */
#endif

#ifndef HAVE_GETCWD
#define getcwd(d,n) getwd(d)
#endif

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

/*
 * Unexported functions.
 */
static EB_Error_Code eb_catalog_file_name_internal EB_P((const char *, size_t,
    EB_Disc_Code *, EB_Case_Code *, EB_Suffix_Code *));
static EB_Error_Code eb_canonicalize_file_name_internal EB_P((char *));
static void eb_fix_file_name_internal EB_P((char *, size_t, EB_Case_Code,
    EB_Suffix_Code));

/*
 * Inspect file name mode and disctype of `book'.
 */
EB_Error_Code
eb_catalog_file_name(book)
    EB_Book *book;
{
    EB_Error_Code error_code;

    error_code = eb_catalog_file_name_internal(book->path, book->path_length,
	&book->disc_code, &book->case_code, &book->suffix_code);
    if (error_code != EB_SUCCESS)
	goto failed;

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return EB_ERR_FAIL_OPEN_CAT;
}


/*
 * Inspect file name mode and disctype of `appendix'.
 */
EB_Error_Code
eb_appendix_catalog_file_name(appendix)
    EB_Appendix *appendix;
{
    EB_Error_Code error_code;

    error_code = eb_catalog_file_name_internal(appendix->path,
	appendix->path_length, &appendix->disc_code, &appendix->case_code,
	&appendix->suffix_code);
    if (error_code != EB_SUCCESS)
	goto failed;

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    return EB_ERR_FAIL_OPEN_CATAPP;
}


/*
 * Subcontractor function for eb_catalog_file_name() and
 * eb_appendix_catalog_file_name().
 */
static EB_Error_Code
eb_catalog_file_name_internal(path, path_length, disc_code, case_code,
    suffix_code)
    const char *path;
    size_t path_length;
    EB_Disc_Code *disc_code;
    EB_Case_Code *case_code;
    EB_Suffix_Code *suffix_code;
{
    struct stat status;
    char catalog_file_name[PATH_MAX + 1];
    char *case_p;
    char *end_p;

    /*
     * Open a catalog file, and check for disc type.
     * At first, it is assumed that this book is EB*.
     */
    sprintf(catalog_file_name, "%s/%s", path, EB_FILE_NAME_CATALOG);
    eb_canonicalize_file_name_internal(catalog_file_name);
    end_p = catalog_file_name + strlen(catalog_file_name);
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_NONE;
	*disc_code = EB_DISC_EB;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EB;
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_DOT;
	return EB_SUCCESS;
    }

    strcpy(end_p, ";1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EB;
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_VERSION;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".;1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EB;
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_BOTH;
	return EB_SUCCESS;
    }

    sprintf(catalog_file_name, "%s/%s", path, EB_FILE_NAME_CATALOG);
    eb_canonicalize_file_name_internal(catalog_file_name);
    end_p = catalog_file_name + strlen(catalog_file_name);
    for (case_p = catalog_file_name + path_length + 1; *case_p != '\0';
	 case_p++) {
	if ('A' <= *case_p && *case_p <= 'Z')
	    *case_p += ('a' - 'A');
    }
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EB;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_NONE;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EB;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_DOT;
	return EB_SUCCESS;
    }

    strcpy(end_p, ";1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EB;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_VERSION;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".;1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EB;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_BOTH;
	return EB_SUCCESS;
    }

    /*
     * Next, it is assumed that this books is EPWING.
     */
    sprintf(catalog_file_name, "%s/%s", path, EB_FILE_NAME_CATALOGS);
    eb_canonicalize_file_name_internal(catalog_file_name);
    end_p = catalog_file_name + strlen(catalog_file_name);
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_NONE;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_DOT;
	return EB_SUCCESS;
    }

    strcpy(end_p, ";1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_VERSION;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".;1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_UPPER;
	*suffix_code = EB_SUFFIX_BOTH;
	return EB_SUCCESS;
    }

    sprintf(catalog_file_name, "%s/%s", path, EB_FILE_NAME_CATALOGS);
    eb_canonicalize_file_name_internal(catalog_file_name);
    end_p = catalog_file_name + strlen(catalog_file_name);
    for (case_p = catalog_file_name + path_length + 1; *case_p != '\0';
	 case_p++) {
	if ('A' <= *case_p && *case_p <= 'Z')
	    *case_p += ('a' - 'A');
    }
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_NONE;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_DOT;
	return EB_SUCCESS;
    }

    strcpy(end_p, ";1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_VERSION;
	return EB_SUCCESS;
    }

    strcpy(end_p, ".;1");
    if (stat(catalog_file_name, &status) == 0 && S_ISREG(status.st_mode)) {
	*disc_code = EB_DISC_EPWING;
	*case_code = EB_CASE_LOWER;
	*suffix_code = EB_SUFFIX_BOTH;
	return EB_SUCCESS;
    }

    /*
     * No catalog file is availabe.  Give up.
     */
    return EB_ERR_FAIL_OPEN_CAT;
}


/*
 * Canonicalize `file_name' of a file in a book.
 */
EB_Error_Code
eb_canonicalize_file_name(book, file_name)
    EB_Book *book;
    char *file_name;
{
    EB_Error_Code error_code;

    error_code = eb_canonicalize_file_name_internal(file_name);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
}


/*
 * Canonicalize `file_name' of a file in an appendix.
 */
EB_Error_Code
eb_canonicalize_appendix_file_name(appendix, file_name)
    EB_Appendix *appendix;
    char *file_name;
{
    EB_Error_Code error_code;

    error_code = eb_canonicalize_file_name_internal(file_name);
    if (error_code != EB_SUCCESS)
	goto failed;

    /*
     * An error occurs...
     */
  failed:
    return error_code;
}

#ifndef DOS_FILE_PATH

/*
 * Canonicalize `file_name' (UNIX version).
 * Replace `/./' and `/../' in `file_name' to equivalent straight
 * form.  If an error occurs, -1 is returned and the error code
 * is set to `error'.  Otherwise 0 is returned.
 */
static EB_Error_Code
eb_canonicalize_file_name_internal(file_name)
    char *file_name;
{
    char cwd[PATH_MAX + 1];
    char *source;
    char *destination;
    char *slash;
    size_t file_name_length;
    size_t cwd_length;
    int i;

    if (*file_name != '/') {
	/*
	 * `file_name' is an relative path.  Convert to an absolute
	 * path.
	 */
	if (getcwd(cwd, PATH_MAX + 1) == NULL)
	    return EB_ERR_FAIL_GETCWD;
	cwd_length = strlen(cwd);
	file_name_length = strlen(file_name);
	if (PATH_MAX < cwd_length + 1 + file_name_length)
	    return EB_ERR_TOO_LONG_FILE_NAME;

	source = file_name + file_name_length;
	destination = file_name + cwd_length + 1 + file_name_length;
	for (i = 0; i <= file_name_length; i++)
	    *destination-- = *source--;
	*destination = '/';

	memcpy(file_name, cwd, cwd_length);
    }

    /*
     * Canonicalize book->path.
     * Replace `.' and `..' segments in the path.
     */
    source = file_name;
    destination = file_name;
    while (*source != '\0') {
	if (*source != '/') {
	    *destination++ = *source++;
	    continue;
	}

	/*
	 * `*source' is slash (`/')
	 */
	if (*(source + 1) == '/' || *(source + 1) == '\0') {
	    /*
	     * `//' -- Ignore 2nd slash (`/').
	     */
	    source++;
	    continue;
	} else if (*(source + 1) == '.'
	    && (*(source + 2) == '/' || *(source + 2) == '\0')) {
	    /*
	     * `/.' -- The current segment itself.  Removed.
	     */
	    source += 2;
	} else if (*(source + 1) == '.' && *(source + 2) == '.'
	    && (*(source + 3) == '/' || *(source + 3) == '\0')) {
	    /*
	     * `/..' -- Back to a parent segment.
	     */
	    source += 3;
	    *destination = '\0';
	    slash = strrchr(file_name, '/');
	    if (slash == NULL)
		destination = file_name;
	    else
		destination = slash;
	} else
	    *destination++ = *source++;
    }
    *destination = '\0';

    /*
     * When the path comes to be empty, set the path to `/'.
     */
    if (*(file_name) == '\0') {
	*(file_name) = '/';
	*(file_name + 1) = '\0';
    }

    return EB_SUCCESS;
}

#else /* DOS_FILE_PATH */

/*
 * Canonicalize `file_name' (DOS version).
 * Replace `\.\' and `\..\' in `file_name' to equivalent straight
 * form.  If an error occurs, -1 is returned and the error code
 * is set to `error'.  Otherwise 0 is returned.
 *
 * eb_canonicalize_file_name_internal() for MSDOS by KSK Jan/30/1998
 */
static EB_Error_Code
eb_canonicalize_file_name_internal(file_name)
    char *file_name;
{
    char cwd[PATH_MAX + 1];
    char *source;
    char *destination;
    char *slash;
    size_t file_name_length;
    size_t cwd_length;
    char *pfile_name;		/* file_name without drive letter */
    int current_drive;
    int is_unc;			/* is `file_name' UNC path? */
    int i;

    /* canonicalize path name separator into '\' */
    for (destination = file_name; *destination != '\0'; destination++) {
	/* forget about SJIS file_name :-p */
	if (*destination == '/') 
	    *destination = '\\';
    }
    /* check a drive letter and UNC path */
    file_name_length = strlen(file_name);
    current_drive = 0;
    is_unc = 0;
    pfile_name = file_name;
    if (file_name_length >= 2) {
	if ('a' <= *file_name && *file_name <= 'z'
	    && *(file_name + 1) == ':') {
	    current_drive = *file_name - 'a' + 1;
	    pfile_name = file_name + 2;
	} else if ('A' <= *file_name && *file_name <= 'Z'
	    && *(file_name + 1) == ':') {
	    current_drive = *file_name - 'A' + 1;
	    pfile_name = file_name + 2;
	} else if (*file_name == '\\' && *(file_name + 1) == '\\') {
	    is_unc = 1;
	    pfile_name = file_name + 1;
	}
    }

    if (!is_unc) {
	if (*pfile_name != '\\' || current_drive == 0) {
	    /* `file_name' is a relative path or has no drive letter */

	    if (current_drive == 0) {
		/* `file_name' has no drive letter */
		if (getcwd(cwd, PATH_MAX + 1) == NULL)
		    return EB_ERR_FAIL_GETCWD;
		if (*pfile_name == '\\') {
		    /*
		     * `file_name' is a absolute path and has no
		     * drive letter use only a drive letter
		     */
		    cwd[2] = '\0';
		}
	    } else {
		/* `file_name' is a relative path with a drive letter  */
		if (getdcwd(current_drive, cwd, PATH_MAX + 1) == NULL) {
		    return EB_ERR_FAIL_GETCWD;
		}
	    }

	    cwd_length = strlen(cwd);
	    file_name_length = strlen(pfile_name);
	    if (PATH_MAX < cwd_length + 1 + file_name_length) {
		if (error != NULL)
		    *error = EB_ERR_TOO_LONG_FILE_NAME;
		return -1;
	    }
	    source = pfile_name + file_name_length;
	    destination = file_name + cwd_length + 1 + file_name_length;
	    for (i = 0; i <= file_name_length; i++) {
		*destination-- = *source--;
	    }
	    *destination = '\\';
	    memcpy(file_name, cwd, cwd_length);
	    pfile_name = file_name + 2;
	}
    }
    /*
     * Canonicalize book->path.
     * Replace "." and ".." segments in the path.
     */
    source = pfile_name;
    destination = pfile_name;
    while (*source != '\0') {
	if (*source != '\\') {
	    *destination++ = *source++;
	    continue;
	}

	/*
	 * `*source' is slash (`/')
	 */
	if (*(source + 1) == '\\' || *(source + 1) == '\0') {
	    /*
	     * `\\' -- Ignore 2nd backslash (`\').
	     */
	    source++;
	    continue;
	} else if (*(source + 1) == '.'
	    && (*(source + 2) == '\\' || *(source + 2) == '\0')) {
	    /*
	     * `\.' -- The current segment itself.  Removed.
	     */
	    source += 2;
	} else if (*(source + 1) == '.' && *(source + 2) == '.'
	    && (*(source + 3) == '\\' || *(source + 3) == '\0')) {
	    /*
	     * `\..' -- Back to a parent segment.
	     */
	    source += 3;
	    *destination = '\0';
	    slash = strrchr(pfile_name, '\\');
	    if (slash == NULL)
		destination = pfile_name;
	    else
		destination = slash;
	} else
	    *destination++ = *source++;
    }
    *destination = '\0';

    /*
     * When the path comes to be empty, set the path to `\\'.
     */
    if (*(pfile_name) == '\0') {
	*(pfile_name) = '\\';
	*(pfile_name + 1) = '\0';
    }

    return EB_SUCCESS;
}

#endif /* DOS_FILE_PATH */


/*
 * Fix cases and suffix of `file_name' to adapt to `book'.
 */
void
eb_fix_file_name(book, file_name)
    EB_Book *book;
    char *file_name;
{
    eb_fix_file_name_internal(file_name, book->path_length, book->case_code,
	book->suffix_code);
}


/*
 * Fix cases and suffix of `file_name' to adapt to `appendix'.
 */
void
eb_fix_appendix_file_name(appendix, file_name)
    EB_Appendix *appendix;
    char *file_name;
{
    eb_fix_file_name_internal(file_name, appendix->path_length,
	appendix->case_code, appendix->suffix_code);
}


/*
 * Subcontractor function for eb_fix_file_name() and
 * eb_fix_appendix_file_name().
 */
static void
eb_fix_file_name_internal(file_name, path_length, case_code, suffix_code)
    char *file_name;
    size_t path_length;
    EB_Case_Code case_code;
    EB_Suffix_Code suffix_code;
{
    char *p;

    /*
     * Change cases of the file_name under `path' to lower cases, if needed.
     */
    if (case_code == EB_CASE_LOWER) {
	for (p = file_name + path_length + 1; *p != '\0'; p++) {
	    if ('A' <= *p && *p <= 'Z')
		*p += ('a' - 'A');
	}
    }

    /*
     * Append `.', `;1', or `.;1' to the file_name if needed.
     */
    if (suffix_code == EB_SUFFIX_DOT)
	strcat(file_name, ".");
    else if (suffix_code == EB_SUFFIX_VERSION)
	strcat(file_name, ";1");
    else if (suffix_code == EB_SUFFIX_BOTH)
	strcat(file_name, ".;1");

    /*
     * Trim successive slashes (`/').
     */
    eb_canonicalize_file_name_internal(file_name);
}
