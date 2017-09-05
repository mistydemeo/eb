/*
 * Copyright (c) 1997, 2000, 01  
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
 * 
 * If a directory matched to `directory_name' exists, then EB_SUCCESS is
 * returned, and `directory_name' is rewritten to that name.  Otherwise
 * EB_ERR_BAD_DIR_NAME is returned.
 */
EB_Error_Code
eb_fix_directory_name(path, directory_name)
    const char *path;
    char *directory_name;
{
    struct dirent *entry;
    DIR *dir;

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
            goto failed;

	if (strcasecmp(entry->d_name, directory_name) == 0)
	    break;
    }

    strcpy(directory_name, entry->d_name);
    closedir(dir);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (dir != NULL)
	closedir(dir);
    return EB_ERR_BAD_DIR_NAME;
}


/*
 * Rewrite `sub_directory_name' to a real sub directory name in the
 * `path/directory_name' directory.
 * 
 * If a directory matched to `sub_directory_name' exists, then EB_SUCCESS
 * is returned, and `directory_name' is rewritten to that name.  Otherwise
 * EB_ERR_BAD_FILE_NAME is returned.
 */
EB_Error_Code
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
 * 
 * If a file matched to `file_name' exists, then EB_SUCCESS is returned,
 * and `file_name' is rewritten to that name.  Otherwise EB_ERR_BAD_FILE_NAME
 * is returned.
 */
EB_Error_Code
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
            goto failed;

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

    strcpy(file_name, entry->d_name);
    closedir(dir);
    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (dir != NULL)
	closedir(dir);
    return EB_ERR_BAD_FILE_NAME;
}


/*
 * Rewrite `file_name' to a real file name in the directory
 * `path_name/sub_directory_name'.
 *
 * If a file matched to `file_name' exists, then EB_SUCCESS is returned,
 * and `file_name' is rewritten to that name.  Otherwise EB_ERR_BAD_FILE_NAME
 * is returned.
 */
EB_Error_Code
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
 * If a file matched to `file_name' exists, then EB_SUCCESS is returned,
 * and `file_name' is rewritten to that name.  Otherwise EB_ERR_BAD_FILE_NAME
 * is returned.
 */
EB_Error_Code
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
 * If a file `composed_path_name' exists, then EB_SUCCESS is returned.
 * Otherwise EB_ERR_BAD_FILE_NAME is returned.
 */
EB_Error_Code
eb_compose_path_name(path_name, file_name, suffix, composed_path_name)
    const char *path_name;
    const char *file_name;
    const char *suffix;
    char *composed_path_name;
{
    char fixed_file_name[EB_MAX_FILE_NAME_LENGTH + 1];

    sprintf(fixed_file_name, "%s%s", file_name, suffix);
    if (eb_fix_file_name(path_name, fixed_file_name) == EB_SUCCESS) {
	sprintf(composed_path_name, F_("%s/%s", "%s\\%s"),
	    path_name, fixed_file_name);
	return EB_SUCCESS;
    }

    return EB_ERR_BAD_FILE_NAME;
}


/*
 * Compose a file name
 *     `path_name/sub_directory/file_name.suffix'
 * and copy it into `composed_path_name'.
 *
 * If a file `composed_path_name' exists, then EB_SUCCESS is returned.
 * Otherwise EB_ERR_BAD_FILE_NAME is returned.
 */
EB_Error_Code
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

    if (eb_fix_file_name(sub_path_name, fixed_file_name) == EB_SUCCESS) {
	sprintf(composed_path_name, F_("%s/%s", "%s\\%s"),
	    sub_path_name, fixed_file_name);
	return EB_SUCCESS;
    }

    return EB_ERR_BAD_FILE_NAME;
}


/*
 * Compose a file name
 *     `path_name/sub_directory/sub2_directory/file_name.suffix'
 * and copy it into `composed_path_name'.
 *
 * If a file `composed_path_name' exists, then EB_SUCCESS is returned.
 * Otherwise EB_ERR_BAD_FILE_NAME is returned.
 */
EB_Error_Code
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

    if (eb_fix_file_name(sub2_path_name, fixed_file_name) == EB_SUCCESS) {
	sprintf(composed_path_name, F_("%s/%s", "%s\\%s"),
	    sub2_path_name, fixed_file_name);
	return EB_SUCCESS;
    }

    return EB_ERR_BAD_FILE_NAME;
}


/*
 * Compose movie file name from argv[], and copy the result to
 * `composed_file_name'.  Note that upper letters are converted to lower
 * letters.
 *
 * If a file `composed_path_name' exists, then EB_SUCCESS is returned.
 * Otherwise EB_ERR_BAD_FILE_NAME is returned.
 */
EB_Error_Code
eb_compose_movie_file_name(argv, composed_file_name)
    const unsigned int *argv;
    char *composed_file_name;
{
    unsigned short jis_characters[EB_MAX_DIRECTORY_NAME_LENGTH];
    const unsigned int *arg_p;
    char *composed_p;
    unsigned short c;
    int i;

    /*
     * Initialize `jis_characters[]'.
     */
    for (i = 0, arg_p = argv;
	 i + 1 < EB_MAX_DIRECTORY_NAME_LENGTH; i += 2, arg_p++) {
	jis_characters[i]     = (*arg_p >> 16) & 0xffff;
	jis_characters[i + 1] = (*arg_p)       & 0xffff;
    }
    if (i < EB_MAX_DIRECTORY_NAME_LENGTH)
	jis_characters[i]     = (*arg_p >> 16) & 0xffff;

    /*
     * Compose file name.
     */
    for (i = 0, composed_p = composed_file_name;
	 i < EB_MAX_DIRECTORY_NAME_LENGTH; i++, composed_p++) {
	c = jis_characters[i];
	if (c == 0x2121 || c == 0x0000)
	    break;
	if ((0x2330 <= c && c <= 0x2339) || (0x2361 <= c && c <= 0x237a))
	    *composed_p = c & 0xff;
	else if (0x2341 <= c && c <= 0x235a)
	    *composed_p = (c | 0x20) & 0xff;
	else
	    return EB_ERR_BAD_FILE_NAME;
    }

    *composed_p = '\0';

    return EB_SUCCESS;
}


/*
 * Decompose movie file name into argv[].  This is the reverse of
 * eb_compose_movie_file_name().  Note that lower letters are converted
 * to upper letters.
 *
 * EB_SUCCESS is returned upon success, EB_ERR_BAD_FILE_NAME otherwise.
 */
EB_Error_Code
eb_decompose_movie_file_name(argv, composed_file_name)
    unsigned int *argv;
    const char *composed_file_name;
{
    unsigned short jis_characters[EB_MAX_DIRECTORY_NAME_LENGTH];
    unsigned int *arg_p;
    const char *composed_p;
    int i;

    /*
     * Initialize `jis_characters[]'.
     */
    for (i = 0; i < EB_MAX_DIRECTORY_NAME_LENGTH; i++)
	jis_characters[i] = 0x0000;

    /*
     * Set jis_characters[].
     */
    for (i = 0, composed_p = composed_file_name;
	 i < EB_MAX_DIRECTORY_NAME_LENGTH && *composed_p != '\0';
	 i++, composed_p++) {
	if ('0' <= *composed_p && *composed_p <= '9')
	    jis_characters[i] = 0x2330 + (*composed_p - '0');
	else if ('A' <= *composed_p && *composed_p <= 'Z')
	    jis_characters[i] = 0x2341 + (*composed_p - 'A');
	else if ('a' <= *composed_p && *composed_p <= 'z')
	    jis_characters[i] = 0x2341 + (*composed_p - 'a');
	else
	    return EB_ERR_BAD_FILE_NAME;
    }
    if (*composed_p != '\0')
	return EB_ERR_BAD_FILE_NAME;

    /*
     * Compose file name.
     */
    for (i = 0, arg_p = argv;
	 i + 1 < EB_MAX_DIRECTORY_NAME_LENGTH; i += 2, arg_p++) {
	*arg_p = (jis_characters[i] << 16) | jis_characters[i + 1];
    }
    if (i < EB_MAX_DIRECTORY_NAME_LENGTH) {
	*arg_p++ = jis_characters[i] << 16;
    }
    *arg_p = '\0';

    return EB_SUCCESS;
}


