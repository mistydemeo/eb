/*
 * Copyright (c) 1997  Motoyuki Kasahara
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
#include <ctype.h>
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

/* for Visual C++ by KSK Jan/30/1998 */
#if defined(HAVE_DIRECT_H) && defined(HAVE__GETDCWD)
#include <direct.h>            /* for _getcwd(), _getdcwd() */
#define getcwd _getcwd
#define getdcwd _getdcwd
#endif

#include "eb.h"
#include "error.h"

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
static int eb_catalog_filename_internal EB_P((const char *, size_t,
    EB_Disc_Code *, EB_Case_Code *, EB_Suffix_Code *));
static void eb_fix_filename_internal EB_P((char *, size_t, EB_Case_Code,
    EB_Suffix_Code));


/*
 * Inspect filename mode and disctype of `book'.
 */
int
eb_catalog_filename(book)
    EB_Book *book;
{
    if (eb_catalog_filename_internal(book->path, book->path_length,
	&book->disc_code, &book->case_code, &book->suffix_code) < 0) {
	eb_error = EB_ERR_FAIL_OPEN_CAT;
	return -1;
    }

    return 0;
}


/*
 * Inspect filename mode and disctype of `appendix'.
 */
int
eb_appendix_catalog_filename(appendix)
    EB_Appendix *appendix;
{
    if (eb_catalog_filename_internal(appendix->path, appendix->path_length,
	&appendix->disc_code, &appendix->case_code, &appendix->suffix_code)
	< 0) {
	eb_error = EB_ERR_FAIL_OPEN_CATAPP;
	return -1;
    }

    return 0;
}


/*
 * Subcontractor function for eb_catalog_filename() and
 * eb_appendix_catalog_filename().
 */
static int
eb_catalog_filename_internal(path, pathlen, disccode, casecode, suffixcode)
    const char *path;
    size_t pathlen;
    EB_Disc_Code *disccode;
    EB_Case_Code *casecode;
    EB_Suffix_Code *suffixcode;
{
    struct stat st;
    char catalog[PATH_MAX + 1];
    char *casep;
    char *endp;

    /*
     * Open a catalog file, and check for disc type.
     * At first, it is assumed that this book is EB/EBG/EBXA.
     */
    sprintf(catalog, "%s/%s", path, EB_FILENAME_CATALOG);
    eb_canonicalize_filename(catalog);
    endp = catalog + strlen(catalog);
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_NONE;
	*disccode = EB_DISC_EB;
	return 0;
    }

    strcpy(endp, ".");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_DOT;
	return 0;
    }

    strcpy(endp, ";1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_VERSION;
	return 0;
    }

    strcpy(endp, ".;1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_BOTH;
	return 0;
    }

    sprintf(catalog, "%s/%s", path, EB_FILENAME_CATALOG);
    eb_canonicalize_filename(catalog);
    endp = catalog + strlen(catalog);
    for (casep = catalog + pathlen + 1; *casep != '\0'; casep++) {
	if (isupper(*casep))
	    *casep = tolower(*casep);
    }
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_NONE;
	return 0;
    }

    strcpy(endp, ".");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_DOT;
	return 0;
    }

    strcpy(endp, ".");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_DOT;
	return 0;
    }

    strcpy(endp, ";1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_VERSION;
	return 0;
    }

    strcpy(endp, ".;1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EB;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_BOTH;
	return 0;
    }

    /*
     * Next, it is assumed that this books is EPWING.
     */
    sprintf(catalog, "%s/%s", path, EB_FILENAME_CATALOGS);
    eb_canonicalize_filename(catalog);
    endp = catalog + strlen(catalog);
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_NONE;
	return 0;
    }

    strcpy(endp, ".");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_DOT;
	return 0;
    }

    strcpy(endp, ";1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_VERSION;
	return 0;
    }

    strcpy(endp, ".;1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_UPPER;
	*suffixcode = EB_SUFFIX_BOTH;
	return 0;
    }

    sprintf(catalog, "%s/%s", path, EB_FILENAME_CATALOGS);
    eb_canonicalize_filename(catalog);
    endp = catalog + strlen(catalog);
    for (casep = catalog + pathlen + 1; *casep != '\0'; casep++) {
	if (isupper(*casep))
	    *casep = tolower(*casep);
    }
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_NONE;
	return 0;
    }

    strcpy(endp, ".");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_DOT;
	return 0;
    }

    strcpy(endp, ";1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_VERSION;
	return 0;
    }

    strcpy(endp, ".;1");
    if (stat(catalog, &st) == 0 && S_ISREG(st.st_mode)) {
	*disccode = EB_DISC_EPWING;
	*casecode = EB_CASE_LOWER;
	*suffixcode = EB_SUFFIX_BOTH;
	return 0;
    }

    /*
     * No catalog file is availabe.  Give up.
     * (`eb_error' is set by caller.)
     */
    return -1;
}


#ifndef DOS_FILE_PATH

/*
 * Canonicalize `filename' (UNIX version).
 * Replace `/./' and `/../' in `filename' to equivalent straight
 * forms.
 */
int
eb_canonicalize_filename(filename)
    char *filename;
{
    char curdir[PATH_MAX + 1];
    char *src, *dst;
    char *rslash;
    size_t namelen, curlen;
    int i;

    if (*filename != '/') {
	/*
	 * `filename' is an relative path.  Convert to an absolute
	 * path.
	 */
	if (getcwd(curdir, PATH_MAX + 1) == NULL) {
	    eb_error = EB_ERR_FAIL_GETCWD;
	    return -1;
	}
	curlen = strlen(curdir);
	namelen = strlen(filename);
	if (PATH_MAX < curlen + 1 + namelen) {
	    eb_error = EB_ERR_TOO_LONG_FILENAME;
	    return -1;
	}

	src = filename + namelen;
	dst = filename + curlen + 1 + namelen;
	for (i = 0; i <= namelen; i++)
	    *dst-- = *src--;
	*dst = '/';

	memcpy(filename, curdir, curlen);
    }

    /*
     * Canonicalize book->path.
     * Replace `.' and `..' segments in the path.
     */
    src = filename;
    dst = filename;
    while (*src != '\0') {
	if (*src != '/') {
	    *dst++ = *src++;
	    continue;
	}

	if (*(src + 1) == '/' || *(src + 1) == '\0') {
	    /*
	     * `//' -- Ignore 2nd slash (`/').
	     */
	    src++;
	    continue;
	} else if (*(src + 1) == '.'
	    && (*(src + 2) == '/' || *(src + 2) == '\0')) {
	    /*
	     * `/.' -- The current segment itself.  Removed.
	     */
	    src += 2;
	} else if (*(src + 1) == '.' && *(src + 2) == '.'
	    && (*(src + 3) == '/' || *(src + 3) == '\0')) {
	    /*
	     * `/..' -- Back to a parent segment.
	     */
	    src += 3;
	    *dst = '\0';
	    rslash = strrchr(filename, '/');
	    if (rslash == NULL)
		dst = filename;
	    else
		dst = rslash;
	} else
	    *dst++ = *src++;
    }
    *dst = '\0';

    /*
     * When the path comes to be empty, set the path to `/'.
     */
    if (*(filename) == '\0') {
	*(filename) = '/';
	*(filename + 1) = '\0';
    }

    return 0;
}

#else /* DOS_FILE_PATH */

/*
 * Canonicalize `filename' (DOS version).
 * Replace `\.\' and `\..\' in `filename' to equivalent straight
 * forms.
 *
 * eb_canonicalize_filename(filename) for MSDOS by KSK Jan/30/1998
 */
int
eb_canonicalize_filename(filename)
    char *filename;
{
    char curdir[PATH_MAX + 1];
    char *src, *dst;
    char *rslash;
    size_t namelen, curlen;
    char *pfilename;		/* filename without drive letter */
    int curdrive;
    int isunc;			/* is `filename' UNC path? */
    int i;

    /* canonicalize path name separator into '\' */
    for (dst = filename; *dst; dst++) {
	/* forget about SJIS pathname :-p */
	if (*dst == '/') 
	    *dst = '\\';
    }
    /* check a drive letter and UNC path */
    namelen = strlen(filename);
    curdrive = 0;
    isunc = 0;
    pfilename = filename;
    if (namelen >= 2) {
	if (isalpha(*filename) && (*(filename+1) == ':')) {
	    curdrive = toupper(*filename) - 'A' + 1;
	    pfilename = filename + 2;
	} else if ((*filename == '\\') && (*(filename+1) == '\\')) {
	    isunc = 1;
	    pfilename = filename + 1;
	}
    }

    if (!isunc) {
	if ((*pfilename != '\\') || (curdrive == 0)) {
	    /* `filename' is a relative path or has no drive letter */

	    if (curdrive == 0) {
		/* `filename' has no drive letter */
		if (getcwd(curdir, PATH_MAX + 1) == NULL) {
		    eb_error = EB_ERR_FAIL_GETCWD;
		    return -1;
		}
		if (*pfilename == '\\') {
		    /* `filename' is a absolute path and has no drive letter */
		    /* use only a drive letter */
		    curdir[2] = '\0';
		}
	    } else {
		/* `filename' is a relative path with a drive letter  */
		if (getdcwd(curdrive, curdir, PATH_MAX + 1) == NULL) {
		    eb_error = EB_ERR_FAIL_GETCWD;
		    return -1;
		}
	    }

	    curlen = strlen(curdir);
	    namelen = strlen(pfilename);
	    if (PATH_MAX < curlen + 1 + namelen) {
		eb_error = EB_ERR_TOO_LONG_FILENAME;
		return -1;
	    }
	    src = pfilename + namelen;
	    dst = filename + curlen + 1 + namelen;
	    for (i = 0; i <= namelen; i++) {
		*dst-- = *src--;
	    }
	    *dst = '\\';
	    memcpy(filename, curdir, curlen);
	    pfilename = filename + 2;
	}
    }
    /*
     * Canonicalize book->path.
     * Replace "." and ".." segments in the path.
     */
    src = pfilename;
    dst = pfilename;
    while (*src != '\0') {
	if (*src != '\\') {
	    *dst++ = *src++;
	    continue;
	}

	if (*(src + 1) == '\\' || *(src + 1) == '\0') {
	    /*
	     * `\\' -- Ignore 2nd backslash (`\').
	     */
	    src++;
	    continue;
	} else if (*(src + 1) == '.'
	    && (*(src + 2) == '\\' || *(src + 2) == '\0')) {
	    /*
	     * `\.' -- The current segment itself.  Removed.
	     */
	    src += 2;
	} else if (*(src + 1) == '.' && *(src + 2) == '.'
	    && (*(src + 3) == '\\' || *(src + 3) == '\0')) {
	    /*
	     * `\..' -- Back to a parent segment.
	     */
	    src += 3;
	    *dst = '\0';
	    rslash = strrchr(pfilename, '\\');
	    if (rslash == NULL)
		dst = pfilename;
	    else
		dst = rslash;
	} else
	    *dst++ = *src++;
    }
    *dst = '\0';

    /*
     * When the path comes to be empty, set the path to `\\'.
     */
    if (*(pfilename) == '\0') {
	*(pfilename) = '\\';
	*(pfilename + 1) = '\0';
    }

    return 0;
}

#endif /* DOS_FILE_PATH */


/*
 * Fix cases and suffix of `filename' to adapt to `book'.
 */
void
eb_fix_filename(book, filename)
    EB_Book *book;
    char *filename;
{
    eb_fix_filename_internal(filename, book->path_length, book->case_code,
	book->suffix_code);
}


/*
 * Fix cases and suffix of `filename' to adapt to `appendix'.
 */
void
eb_fix_appendix_filename(appendix, filename)
    EB_Appendix *appendix;
    char *filename;
{
    eb_fix_filename_internal(filename, appendix->path_length,
	appendix->case_code, appendix->suffix_code);
}


/*
 * Subcontractor function for eb_fix_filename() and
 * eb_fix_appendix_filename().
 */
static void
eb_fix_filename_internal(filename, pathlen, casecode, suffixcode)
    char *filename;
    size_t pathlen;
    EB_Case_Code casecode;
    EB_Suffix_Code suffixcode;
{
    char *p;

    /*
     * Change cases of the filename under `path' to lower cases, if needed.
     */
    if (casecode == EB_CASE_LOWER) {
	for (p = filename + pathlen + 1; *p != '\0'; p++) {
	    if (isupper(*p))
		*p = tolower(*p);
	}
    }

    /*
     * Append `.', `;1', or `.;1' to the filename if needed.
     */
    if (suffixcode == EB_SUFFIX_DOT)
	strcat(filename, ".");
    else if (suffixcode == EB_SUFFIX_VERSION)
	strcat(filename, ";1");
    else if (suffixcode == EB_SUFFIX_BOTH)
	strcat(filename, ".;1");

    /*
     * Trim successive slashes (`/').
     */
    eb_canonicalize_filename(filename);
}
