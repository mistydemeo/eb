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
#define EB_ERR_FAIL_OPEN_LANG		12
#define EB_ERR_FAIL_OPEN_TEXT		13
#define EB_ERR_FAIL_OPEN_FONT		14

#define EB_ERR_FAIL_OPEN_APP		15
#define EB_ERR_FAIL_OPEN_BINARY		16
#define EB_ERR_FAIL_READ_CAT		17
#define EB_ERR_FAIL_READ_CATAPP		18
#define EB_ERR_FAIL_READ_LANG		19

#define EB_ERR_FAIL_READ_TEXT		20
#define EB_ERR_FAIL_READ_FONT		21
#define EB_ERR_FAIL_READ_APP		22
#define EB_ERR_FAIL_READ_BINARY		23
#define EB_ERR_FAIL_SEEK_CAT		24

#define EB_ERR_FAIL_SEEK_CATAPP		25
#define EB_ERR_FAIL_SEEK_LANG		26
#define EB_ERR_FAIL_SEEK_TEXT		27
#define EB_ERR_FAIL_SEEK_FONT		28
#define EB_ERR_FAIL_SEEK_APP		29

#define EB_ERR_FAIL_SEEK_BINARY		30
#define EB_ERR_UNEXP_CAT		31
#define EB_ERR_UNEXP_CATAPP		32
#define EB_ERR_UNEXP_LANG		33
#define EB_ERR_UNEXP_TEXT		34

#define EB_ERR_UNEXP_FONT		35
#define EB_ERR_UNEXP_APP		36
#define EB_ERR_UNEXP_BINARY		37
#define EB_ERR_UNBOUND_BOOK		38
#define EB_ERR_UNBOUND_APP		39

#define EB_ERR_NO_LANG			40
#define EB_ERR_NO_SUB			41
#define EB_ERR_NO_APPSUB		42
#define EB_ERR_NO_MSG			43
#define EB_ERR_NO_FONT			44

#define EB_ERR_NO_TEXT			45
#define EB_ERR_NO_CUR_LANG		46
#define EB_ERR_NO_CUR_SUB		47
#define EB_ERR_NO_CUR_APPSUB		48
#define EB_ERR_NO_CUR_FONT		49

#define EB_ERR_NO_CUR_BINARY		50
#define EB_ERR_NO_SUCH_LANG		51
#define EB_ERR_NO_SUCH_SUB		52
#define EB_ERR_NO_SUCH_APPSUB		53
#define EB_ERR_NO_SUCH_MSG		54

#define EB_ERR_NO_SUCH_FONT		55
#define EB_ERR_NO_SUCH_CHAR_BMP		56
#define EB_ERR_NO_SUCH_CHAR_TEXT	57
#define EB_ERR_NO_SUCH_SEARCH		58
#define EB_ERR_NO_SUCH_HOOK		59

#define EB_ERR_NO_SUCH_BINARY		60
#define EB_ERR_STOP_CODE		61
#define EB_ERR_DIFF_CONTENT		62
#define EB_ERR_NO_PREV_SEARCH		63
#define EB_ERR_NO_SUCH_MULTI_ID		64

#define EB_ERR_NO_SUCH_ENTRY_ID		65
#define EB_ERR_TOO_MANY_WORDS		66
#define EB_ERR_NO_WORD			67
#define EB_ERR_NO_CANDIDATES		68

/*
 * The number of error codes.
 */
#define EB_NUMBER_OF_ERRORS		69

/*
 * The maximum length of an error message.
 */
#define EB_MAX_ERROR_MESSAGE_LENGTH	127

/*
 * Function declarations.
 */
/* error.c */
const char *eb_error_message EB_P((EB_Error_Code));

#ifdef __cplusplus
}
#endif

#endif /* not EB_ERROR_H */
