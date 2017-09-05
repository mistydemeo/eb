/*                                                            -*- C -*-
 * Copyright (c) 1997, 98, 2000  Motoyuki Kasahara
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
#define EB_ERR_TOO_LONG_WORD		4

#define EB_ERR_BAD_WORD			5
#define EB_ERR_EMPTY_WORD		6
#define EB_ERR_FAIL_GETCWD		7
#define EB_ERR_FAIL_OPEN_CAT		8
#define EB_ERR_FAIL_OPEN_CATAPP		9

#define EB_ERR_FAIL_OPEN_LANG		10
#define EB_ERR_FAIL_OPEN_TEXT		11
#define EB_ERR_FAIL_OPEN_FONT		12
#define EB_ERR_FAIL_OPEN_APP		13
#define EB_ERR_FAIL_READ_CAT		14

#define EB_ERR_FAIL_READ_CATAPP		15
#define EB_ERR_FAIL_READ_LANG		16
#define EB_ERR_FAIL_READ_TEXT		17
#define EB_ERR_FAIL_READ_FONT		18
#define EB_ERR_FAIL_READ_APP		19

#define EB_ERR_FAIL_SEEK_CAT		20
#define EB_ERR_FAIL_SEEK_CATAPP		21
#define EB_ERR_FAIL_SEEK_LANG		22
#define EB_ERR_FAIL_SEEK_TEXT		23
#define EB_ERR_FAIL_SEEK_FONT		24

#define EB_ERR_FAIL_SEEK_APP		25
#define EB_ERR_UNEXP_CAT		26
#define EB_ERR_UNEXP_CATAPP		27
#define EB_ERR_UNEXP_LANG		28
#define EB_ERR_UNEXP_TEXT		29

#define EB_ERR_UNEXP_FONT		30
#define EB_ERR_UNEXP_APP		31
#define EB_ERR_UNBOUND_BOOK		32
#define EB_ERR_UNBOUND_APP		33
#define EB_ERR_NO_LANG			34

#define EB_ERR_NO_SUB			35
#define EB_ERR_NO_APPSUB		36
#define EB_ERR_NO_MSG			37
#define EB_ERR_NO_FONT			38
#define EB_ERR_NO_TEXT			39

#define EB_ERR_NO_CUR_LANG		40
#define EB_ERR_NO_CUR_SUB		41
#define EB_ERR_NO_CUR_APPSUB		42
#define EB_ERR_NO_CUR_FONT		43
#define EB_ERR_NO_SUCH_LANG		44

#define EB_ERR_NO_SUCH_SUB		45
#define EB_ERR_NO_SUCH_APPSUB		46
#define EB_ERR_NO_SUCH_MSG		47
#define EB_ERR_NO_SUCH_FONT		48
#define EB_ERR_NO_SUCH_CHAR_BMP		49

#define EB_ERR_NO_SUCH_CHAR_TEXT	50
#define EB_ERR_NO_SUCH_SEARCH		51
#define EB_ERR_NO_SUCH_HOOK		52
#define EB_ERR_STOP_CODE		53
#define EB_ERR_DIFF_CONTENT		54

#define EB_ERR_NO_PREV_SEARCH		55
#define EB_ERR_NO_SUCH_MULTI_ID		56
#define EB_ERR_NO_SUCH_ENTRY_ID		57
#define EB_ERR_TOO_MANY_WORDS		58
#define EB_ERR_NO_WORD			59

#define EB_ERR_NO_CANDIDATES		60

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
const char *eb_error_message EB_P((EB_Error_Code));
const char *eb_error_message_r EB_P((EB_Error_Code, char *));

#ifdef __cplusplus
}
#endif

#endif /* not EB_ERROR_H */
