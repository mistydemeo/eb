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

#include "ebconfig.h"

#include "eb.h"
#include "error.h"
#include "internal.h"


#ifndef DOS_FILE_PATH

/*
 * Canonicalize `path_name' (UNIX version).
 * Convert a path name to an absolute path.
 */
EB_Error_Code
eb_canonicalize_path_name(path_name)
    char *path_name;
{
    char cwd[PATH_MAX + 1];
    char temporary_path_name[PATH_MAX + 1];
    size_t path_name_length;

    if (*path_name != '/') {
	/*
	 * `path_name' is an relative path.  Convert to an absolute
	 * path.
	 */
	if (getcwd(cwd, PATH_MAX + 1) == NULL)
	    return EB_ERR_FAIL_GETCWD;
	if (PATH_MAX < strlen(cwd) + 1 + strlen(path_name))
	    return EB_ERR_TOO_LONG_FILE_NAME;
	sprintf(temporary_path_name, "%s/%s", cwd, path_name);
	strcpy(path_name, temporary_path_name);
    }

    /*
     * Unless `path_name' is "/", eliminate `/' in the tail of the
     * path name.
     */
    path_name_length = strlen(path_name);
    if (1 < path_name_length && *(path_name + path_name_length - 1) == '/')
	*(path_name + path_name_length - 1) = '\0';

    return EB_SUCCESS;
}

#else /* DOS_FILE_PATH */

/*
 * Canonicalize `path_name' (DOS version).
 * Convert a path name to an absolute path with drive letter unless
 * that is an UNC path.
 *
 * Original version by KSK Jan/30/1998.
 * Current version by Motoyuki Kasahara.
 */
EB_Error_Code
eb_canonicalize_path_name(path_name)
    char *path_name;
{
    char cwd[PATH_MAX + 1];
    char temporary_path_name[PATH_MAX + 1];
    size_t path_name_length;

    if (*path_name == '\\' && *(path_name + 1) == '\\') {
	/*
	 * `path_name' is UNC path.  Nothing to be done.
	 */
    } else if (isalpha(*path_name) && *(path_name + 1) == ':') {
	/*
	 * `path_name' is has a drive letter.
	 * Nothing to be done if it is an absolute path.
	 */
	if (*(path_name + 2) != '\\') {
	    /*
	     * `path_name' is a relative path.
	     * Covert the path name to an absolute path.
	     */
	    if (getdcwd(toupper(*path_name) - 'A' + 1, cwd, PATH_MAX + 1)
		== NULL) {
		return EB_ERR_FAIL_GETCWD;
	    }
	    if (PATH_MAX < strlen(cwd) + 1 + strlen(path_name + 2))
		return EB_ERR_TOO_LONG_FILE_NAME;
	    sprintf(temporary_path_name, "%s\\%s", cwd, path_name + 2);
	    strcpy(path_name, temporary_path_name);
	}
    } else if (*path_name == '\\') {
	/*
	 * `path_name' is has no drive letter and is an absolute path.
	 * Add a drive letter to the path name.
	 */
	if (getcwd(cwd, PATH_MAX + 1) == NULL)
	    return EB_ERR_FAIL_GETCWD;
	cwd[1] = '\0';
	if (PATH_MAX < strlen(cwd) + 1 + strlen(path_name))
	    return EB_ERR_TOO_LONG_FILE_NAME;
	sprintf(temporary_path_name, "%s:%s", cwd, path_name);
	strcpy(path_name, temporary_path_name);

    } else {
	/*
	 * `path_name' is has no drive letter and is a relative path.
	 * Add a drive letter and convert it to an absolute path.
	 */
	if (getcwd(cwd, PATH_MAX + 1) == NULL)
	    return EB_ERR_FAIL_GETCWD;

	if (PATH_MAX < strlen(cwd) + 1 + strlen(path_name))
	    return EB_ERR_TOO_LONG_FILE_NAME;
	sprintf(temporary_path_name, "%s\\%s", cwd, path_name);
	strcpy(path_name, temporary_path_name);
    }


    /*
     * Unless `path_name' is "?:\\", eliminate `\\' in the tail of the
     * path name.
     */
    path_name_length = strlen(path_name);
    if (3 < path_name_length && *(path_name + path_name_length - 1) = '\\')
	*(path_name + path_name_length - 1) = '\0';

    return EB_SUCCESS;
}

#endif /* DOS_FILE_PATH */


/*
 * Rewrite `directory_name' to a real directory name in the `path' directory.
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
 * Rewrite `sub_directory_name' to a real sub directory name in the
 * `path/directory_name' directory.
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

    sprintf(sub_path, F_("%s/%s", "%s\\%s"), path, directory_name);
    return eb_fix_directory_name(sub_path, sub_directory_name);
}


/*
 * Rewrite `file_name' to a real file name in the `path_name' directory.
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
 * Rewrite `file_name' to a real file name in the directory
 * `path_name/sub_directory_name'.
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

    sprintf(sub_path_name, F_("%s/%s", "%s\\%s"),
	path_name, sub_directory_name);

    return eb_fix_file_name(sub_path_name, file_name);
}


/*
 * Rewrite `file_name' to a real file name. in the directory
 *    `path_name/sub_directory_name/sub2_directory_name'
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

    sprintf(sub2_path_name, F_("%s/%s/%s", "%s\\%s\\%s"),
	path_name, sub_directory_name, sub2_directory_name);
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
	sprintf(composed_path_name, F_("%s/%s", "%s\\%s"),
	    path_name, fixed_file_name);
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
    sprintf(sub_path_name, F_("%s/%s", "%s\\%s"),
	path_name, sub_directory_name);

    if (eb_fix_file_name(sub_path_name, fixed_file_name) == 0) {
	sprintf(composed_path_name, F_("%s/%s", "%s\\%s"),
	    sub_path_name, fixed_file_name);
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
    sprintf(sub2_path_name, F_("%s/%s/%s", "%s\\%s\\%s"),
	path_name, sub_directory_name, sub2_directory_name);

    if (eb_fix_file_name(sub2_path_name, fixed_file_name) == 0) {
	sprintf(composed_path_name, F_("%s/%s", "%s\\%s"),
	    sub2_path_name, fixed_file_name);
	return 0;
    }

    return -1;
}


