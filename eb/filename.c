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


#ifndef DOS_FILE_PATH

/*
 * Canonicalize `path_name' (UNIX version).
 * Replace `/./' and `/../' in `path_name' to equivalent straight
 * form.  If an error occurs, -1 is returned and the error code
 * is set to `error'.  Otherwise 0 is returned.
 */
EB_Error_Code
eb_canonicalize_path_name(path_name)
    char *path_name;
{
    char cwd[PATH_MAX + 1];
    char *source;
    char *destination;
    char *slash;
    size_t path_name_length;
    size_t cwd_length;
    int i;

    if (*path_name != '/') {
	/*
	 * `path_name' is an relative path.  Convert to an absolute
	 * path.
	 */
	if (getcwd(cwd, PATH_MAX + 1) == NULL)
	    return EB_ERR_FAIL_GETCWD;
	cwd_length = strlen(cwd);
	path_name_length = strlen(path_name);
	if (PATH_MAX < cwd_length + 1 + path_name_length)
	    return EB_ERR_TOO_LONG_FILE_NAME;

	source = path_name + path_name_length;
	destination = path_name + cwd_length + 1 + path_name_length;
	for (i = 0; i <= path_name_length; i++)
	    *destination-- = *source--;
	*destination = '/';

	memcpy(path_name, cwd, cwd_length);
    }

    /*
     * Canonicalize book->path.
     * Replace `.' and `..' segments in the path.
     */
    source = path_name;
    destination = path_name;
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
	    slash = strrchr(path_name, '/');
	    if (slash == NULL)
		destination = path_name;
	    else
		destination = slash;
	} else
	    *destination++ = *source++;
    }
    *destination = '\0';

    /*
     * When the path comes to be empty, set the path to `/'.
     */
    if (*(path_name) == '\0') {
	*(path_name) = '/';
	*(path_name + 1) = '\0';
    }

    return EB_SUCCESS;
}

#else /* DOS_FILE_PATH */

/*
 * Canonicalize `path_name' (DOS version).
 * Replace `\.\' and `\..\' in `path_name' to equivalent straight
 * form.  If an error occurs, -1 is returned and the error code
 * is set to `error'.  Otherwise 0 is returned.
 *
 * eb_canonicalize_path_name_internal() for MSDOS by KSK Jan/30/1998
 */
EB_Error_Code
eb_canonicalize_path_name(path_name)
    char *path_name;
{
    char cwd[PATH_MAX + 1];
    char *source;
    char *destination;
    char *slash;
    size_t path_name_length;
    size_t cwd_length;
    char *ppath_name;		/* path_name without drive letter */
    int current_drive;
    int is_unc;			/* is `path_name' UNC path? */
    int i;

    /* canonicalize path name separator into '\' */
    for (destination = path_name; *destination != '\0'; destination++) {
	/* forget about SJIS path_name :-p */
	if (*destination == '/') 
	    *destination = '\\';
    }
    /* check a drive letter and UNC path */
    path_name_length = strlen(path_name);
    current_drive = 0;
    is_unc = 0;
    ppath_name = path_name;
    if (path_name_length >= 2) {
	if ('a' <= *path_name && *path_name <= 'z'
	    && *(path_name + 1) == ':') {
	    current_drive = *path_name - 'a' + 1;
	    ppath_name = path_name + 2;
	} else if ('A' <= *path_name && *path_name <= 'Z'
	    && *(path_name + 1) == ':') {
	    current_drive = *path_name - 'A' + 1;
	    ppath_name = path_name + 2;
	} else if (*path_name == '\\' && *(path_name + 1) == '\\') {
	    is_unc = 1;
	    ppath_name = path_name + 1;
	}
    }

    if (!is_unc) {
	if (*ppath_name != '\\' || current_drive == 0) {
	    /* `path_name' is a relative path or has no drive letter */

	    if (current_drive == 0) {
		/* `path_name' has no drive letter */
		if (getcwd(cwd, PATH_MAX + 1) == NULL)
		    return EB_ERR_FAIL_GETCWD;
		if (*ppath_name == '\\') {
		    /*
		     * `path_name' is a absolute path and has no
		     * drive letter use only a drive letter
		     */
		    cwd[2] = '\0';
		}
	    } else {
		/* `path_name' is a relative path with a drive letter  */
		if (getdcwd(current_drive, cwd, PATH_MAX + 1) == NULL) {
		    return EB_ERR_FAIL_GETCWD;
		}
	    }

	    cwd_length = strlen(cwd);
	    path_name_length = strlen(ppath_name);
	    if (PATH_MAX < cwd_length + 1 + path_name_length)
		return EB_ERR_TOO_LONG_FILE_NAME;

	    source = ppath_name + path_name_length;
	    destination = path_name + cwd_length + 1 + path_name_length;
	    for (i = 0; i <= path_name_length; i++) {
		*destination-- = *source--;
	    }
	    *destination = '\\';
	    memcpy(path_name, cwd, cwd_length);
	    ppath_name = path_name + 2;
	}
    }
    /*
     * Canonicalize book->path.
     * Replace "." and ".." segments in the path.
     */
    source = ppath_name;
    destination = ppath_name;
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
	    slash = strrchr(ppath_name, '\\');
	    if (slash == NULL)
		destination = ppath_name;
	    else
		destination = slash;
	} else
	    *destination++ = *source++;
    }
    *destination = '\0';

    /*
     * When the path comes to be empty, set the path to `\\'.
     */
    if (*(ppath_name) == '\0') {
	*(ppath_name) = '\\';
	*(ppath_name + 1) = '\0';
    }

    return EB_SUCCESS;
}

#endif /* DOS_FILE_PATH */


/*
 * Rewrite `directory_name' in the `path' directory to a real directory name.
 * If a directory matched to `directory_name' exists, then 0 is returned,
 * and `directory_name' is rewritten to that name.
 * Otherwise -1 is returned.
 */
int
eb_fix_directory_name(path, directory_name)
    const char *path;
    char *directory_name;
{
    struct dirent *entry;
    DIR *dir;
    size_t directory_name_length;
    int have_dot;

    directory_name_length = strlen(directory_name);

    /*
     * Check whether `directory_name' contains a dot.
     */
    have_dot = (strchr(directory_name, '.') != NULL);

    /*
     * Open the directory `path'.
     */
    dir = opendir(path);
    if (dir == NULL)
        goto failed;

    for (;;) {
        /*
         * Read the directory entry.
         */
        entry = readdir(dir);
        if (entry == NULL)
            break;

	if (EB_MAX_FILE_NAME_LENGTH < NAMLEN(entry))
	    continue;

	if (strncasecmp(entry->d_name, directory_name, directory_name_length)
	    == 0)
	    break;
    }

    if (entry == NULL)
	goto failed;

    strcpy(directory_name, entry->d_name);
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
 * Rewrite `sub_directory_name' in the `path/directory_name' directory
 * to a real sub directory name.
 * If a directory matched to `sub_directory_name' exists, then 0 is
 * returned, and `directory_name' is rewritten to that name.
 * Otherwise -1 is returned.
 */
int
eb_fix_directory_name2(path, directory_name, sub_directory_name)
    const char *path;
    const char *directory_name;
    char *sub_directory_name;
{
    char sub_path[PATH_MAX + 1];

    sprintf(sub_path, "%s/%s", path, directory_name);
    return eb_fix_directory_name(sub_path, sub_directory_name);
}


/*
 * Rewrite `file_name' in the `path_name' directory to a real file name.
 * If a file matched to `file_name' exists, then 0 is returned,
 * and `file_name' is rewritten to that name.
 * Otherwise -1 is returned.
 */
int
eb_fix_file_name(path_name, file_name)
    const char *path_name;
    char *file_name;
{
    struct dirent *entry;
    DIR *dir;
    size_t file_name_length;
    const char *suffix;
    int have_dot;

    file_name_length = strlen(file_name);

    /*
     * Check whether `file_name' contains a dot.
     */
    have_dot = (strchr(file_name, '.') != NULL);

    /*
     * Open the directory `path_name'.
     */
    dir = opendir(path_name);
    if (dir == NULL)
        goto failed;

    for (;;) {
        /*
         * Read the directory entry.
         */
        entry = readdir(dir);
        if (entry == NULL)
            break;

	if (EB_MAX_FILE_NAME_LENGTH < NAMLEN(entry))
	    continue;

	/*
	 * Compare the given file name and the entry file name.
	 * We consider they are matched when:
	 *      <given name>       == <entry name>,
	 *   or <given name>+";1'  == <entry name>,
	 *   or <given name>+"."   == <entry name> if no "." in <given name>,
	 *   or <given name>+".;1" == <entry name> if no "." in <given name>.
	 *
	 * All the comparisons are done without case sensitivity.
	 * We support version number ";1" only.
	 */
	if (strncasecmp(entry->d_name, file_name, file_name_length) == 0) {
	    suffix = entry->d_name + file_name_length;
	    if (*suffix == '\0')
		break;
	    else if (strcmp(suffix, ";1") == 0)
		break;
	    else if (!have_dot && strcmp(suffix, ".") == 0)
		break;
	    else if (!have_dot && strcmp(suffix, ".;1") == 0)
		break;
	}
    }

    if (entry == NULL)
	goto failed;

    strcpy(file_name, entry->d_name);
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
 * Rewrite `file_name' in the directory
 *    `path_name/sub_directory_name' 
 * to a real file name.
 *
 * If a file matched to `file_name' exists, then 0 is returned,
 * and `file_name' is rewritten to that name.
 * Otherwise -1 is returned.
 */
int
eb_fix_file_name2(path_name, sub_directory_name, file_name)
    const char *path_name;
    const char *sub_directory_name;
    char *file_name;
{
    char sub_path_name[PATH_MAX + 1];

    sprintf(sub_path_name, "%s/%s", path_name, sub_directory_name);
    return eb_fix_file_name(sub_path_name, file_name);
}


/*
 * Rewrite `file_name' in the directory
 *    `path_name/sub_directory_name/sub2_directory_name' 
 * to a real file name.
 *
 * If a file matched to `file_name' exists, then 0 is returned,
 * and `file_name' is rewritten to that name.
 * Otherwise -1 is returned.
 */
int
eb_fix_file_name3(path_name, sub_directory_name, sub2_directory_name,
    file_name)
    const char *path_name;
    const char *sub_directory_name;
    const char *sub2_directory_name;
    char *file_name;
{
    char sub2_path_name[PATH_MAX + 1];

    sprintf(sub2_path_name, "%s/%s/%s", path_name, sub_directory_name,
	sub2_directory_name);
    return eb_fix_file_name(sub2_path_name, file_name);
}


/*
 * Compose a file name
 *     `path_name/file_name.suffix'
 * and copy it into `composed_path_name'.
 *
 * If a file `composed_path_name' exists, then 0 is returned.
 * Otherwise -1 is returned.
 */
int
eb_compose_path_name(path_name, file_name, suffix, composed_path_name)
    const char *path_name;
    const char *file_name;
    const char *suffix;
    char *composed_path_name;
{
    char fixed_file_name[EB_MAX_FILE_NAME_LENGTH + 1];

    sprintf(fixed_file_name, "%s%s", file_name, suffix);
    if (eb_fix_file_name(path_name, fixed_file_name) == 0) {
	sprintf(composed_path_name, "%s/%s", path_name, fixed_file_name);
	return 0;
    }

    return -1;
}


/*
 * Compose a file name
 *     `path_name/sub_directory/file_name.suffix'
 * and copy it into `composed_path_name'.
 *
 * If a file `composed_path_name' exists, then 0 is returned.
 * Otherwise -1 is returned.
 */
int
eb_compose_path_name2(path_name, sub_directory_name, file_name, suffix,
    composed_path_name)
    const char *path_name;
    const char *sub_directory_name;
    const char *file_name;
    const char *suffix;
    char *composed_path_name;
{
    char fixed_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char sub_path_name[PATH_MAX + 1];

    sprintf(fixed_file_name, "%s%s", file_name, suffix);
    sprintf(sub_path_name, "%s/%s", path_name, sub_directory_name);

    if (eb_fix_file_name(sub_path_name, fixed_file_name) == 0) {
	sprintf(composed_path_name, "%s/%s", sub_path_name, fixed_file_name);
	return 0;
    }

    return -1;
}


/*
 * Compose a file name
 *     `path_name/sub_directory/sub2_directory/file_name.suffix'
 * and copy it into `composed_path_name'.
 *
 * If a file `composed_path_name' exists, then 0 is returned.
 * Otherwise -1 is returned.
 */
int
eb_compose_path_name3(path_name, sub_directory_name, sub2_directory_name,
    file_name, suffix, composed_path_name)
    const char *path_name;
    const char *sub_directory_name;
    const char *sub2_directory_name;
    const char *file_name;
    const char *suffix;
    char *composed_path_name;
{
    char fixed_file_name[EB_MAX_FILE_NAME_LENGTH + 1];
    char sub2_path_name[PATH_MAX + 1];

    sprintf(fixed_file_name, "%s%s", file_name, suffix);
    sprintf(sub2_path_name, "%s/%s/%s", path_name, sub_directory_name,
	sub2_directory_name);

    if (eb_fix_file_name(sub2_path_name, fixed_file_name) == 0) {
	sprintf(composed_path_name, "%s/%s", sub2_path_name, fixed_file_name);
	return 0;
    }

    return -1;
}


