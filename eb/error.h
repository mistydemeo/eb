/*                                                            -*- C -*-
 * Copyright (c) 1997, 98, 2000, 01  
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

#ifndef EB_ERROR_H
#define EB_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EB_BUILD_LIBRARY
#include "defs.h"
#else
#include <eb/defs.h>
#endif

/*
 * Error codes.
 */
#define EB_SUCCESS			0
#define EB_ERR_MEMORY_EXHAUSTED		1
#define EB_ERR_EMPTY_FILE_NAME		2
#define EB_ERR_TOO_LONG_FILE_NAME	3
#define EB_ERR_BAD_FILE_NAME		4

#define EB_ERR_BAD_DIR_NAME		5
#define EB_ERR_TOO_LONG_WORD		6
#define EB_ERR_BAD_WORD			7
#define EB_ERR_EMPTY_WORD		8
#define EB_ERR_FAIL_GETCWD		9

#define EB_ERR_FAIL_OPEN_CAT		10
#define EB_ERR_FAIL_OPEN_CATAPP		11
#define EB_ERR_FAIL_OPEN_TEXT		12
#define EB_ERR_FAIL_OPEN_FONT		13
#define EB_ERR_FAIL_OPEN_APP		14

#define EB_ERR_FAIL_OPEN_BINARY		15
#define EB_ERR_FAIL_READ_CAT		16
#define EB_ERR_FAIL_READ_CATAPP		17
#define EB_ERR_FAIL_READ_TEXT		18
#define EB_ERR_FAIL_READ_FONT		19

#define EB_ERR_FAIL_READ_APP		20
#define EB_ERR_FAIL_READ_BINARY		21
#define EB_ERR_FAIL_SEEK_CAT		22
#define EB_ERR_FAIL_SEEK_CATAPP		23
#define EB_ERR_FAIL_SEEK_TEXT		24

#define EB_ERR_FAIL_SEEK_FONT		25
#define EB_ERR_FAIL_SEEK_APP		26
#define EB_ERR_FAIL_SEEK_BINARY		27
#define EB_ERR_UNEXP_CAT		28
#define EB_ERR_UNEXP_CATAPP		29

#define EB_ERR_UNEXP_TEXT		30
#define EB_ERR_UNEXP_FONT		31
#define EB_ERR_UNEXP_APP		32
#define EB_ERR_UNEXP_BINARY		33
#define EB_ERR_UNBOUND_BOOK		34

#define EB_ERR_UNBOUND_APP		35
#define EB_ERR_NO_SUB			36
#define EB_ERR_NO_APPSUB		37
#define EB_ERR_NO_FONT			38
#define EB_ERR_NO_TEXT			39

#define EB_ERR_NO_CUR_SUB		40
#define EB_ERR_NO_CUR_APPSUB		41
#define EB_ERR_NO_CUR_FONT		42
#define EB_ERR_NO_CUR_BINARY		43
#define EB_ERR_NO_SUCH_SUB		44

#define EB_ERR_NO_SUCH_APPSUB		45
#define EB_ERR_NO_SUCH_FONT		46
#define EB_ERR_NO_SUCH_CHAR_BMP		47
#define EB_ERR_NO_SUCH_CHAR_TEXT	48
#define EB_ERR_NO_SUCH_SEARCH		49

#define EB_ERR_NO_SUCH_HOOK		50
#define EB_ERR_NO_SUCH_BINARY		51
#define EB_ERR_DIFF_CONTENT		52
#define EB_ERR_NO_PREV_SEARCH		53
#define EB_ERR_NO_SUCH_MULTI_ID		54

#define EB_ERR_NO_SUCH_ENTRY_ID		55
#define EB_ERR_TOO_MANY_WORDS		56
#define EB_ERR_NO_WORD			57
#define EB_ERR_NO_CANDIDATES		58
#define EB_ERR_END_OF_CONTENT		59
#define EB_ERR_NO_PREV_SEEK		60

/*
 * The number of error codes.
 */
#define EB_NUMBER_OF_ERRORS		61

/*
 * The maximum length of an error message.
 */
#define EB_MAX_ERROR_MESSAGE_LENGTH	127

/*
 * Function declarations.
 */
/* error.c */
const char *eb_error_string EB_P((EB_Error_Code));
const char *eb_error_message EB_P((EB_Error_Code));

#ifdef __cplusplus
}
#endif

#endif /* not EB_ERROR_H */
