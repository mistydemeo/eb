/*                                                            -*- C -*-
 * Copyright (c) 1997, 98, 99, 2000, 01
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

#ifndef EB_APPENDIX_H
#define EB_APPENDIX_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EB_BUILD_LIBRARY
#include "eb.h"
#else
#include <eb/eb.h>
#endif

/*
 * Function declarations.
 */
/* appendix.c */
void eb_initialize_appendix EB_P((EB_Appendix *));
void eb_finalize_appendix EB_P((EB_Appendix *));
void eb_suspend_appendix EB_P((EB_Appendix *));
EB_Error_Code eb_bind_appendix EB_P((EB_Appendix *, const char *));
int eb_is_appendix_bound EB_P((EB_Appendix *));
EB_Error_Code eb_appendix_path EB_P((EB_Appendix *, char *));

/* appsub.c */
EB_Error_Code eb_load_all_appendix_subbooks EB_P((EB_Appendix *));
EB_Error_Code eb_appendix_subbook_list EB_P((EB_Appendix *,
    EB_Subbook_Code *, int *));
EB_Error_Code eb_appendix_subbook EB_P((EB_Appendix *, EB_Subbook_Code *));
EB_Error_Code eb_appendix_subbook_directory EB_P((EB_Appendix *, char *));
EB_Error_Code eb_appendix_subbook_directory2 EB_P((EB_Appendix *,
    EB_Subbook_Code, char *));
EB_Error_Code eb_set_appendix_subbook EB_P((EB_Appendix *, EB_Subbook_Code));
void eb_unset_appendix_subbook EB_P((EB_Appendix *));

/* narwalt.c */
int eb_have_narrow_alt EB_P((EB_Appendix *));
EB_Error_Code eb_narrow_alt_start EB_P((EB_Appendix *, int *));
EB_Error_Code eb_narrow_alt_end EB_P((EB_Appendix *, int *));
EB_Error_Code eb_narrow_alt_character_text EB_P((EB_Appendix *, int, char *));
EB_Error_Code eb_forward_narrow_alt_character EB_P((EB_Appendix *, int,
    int *));
EB_Error_Code eb_backward_narrow_alt_character EB_P((EB_Appendix *, int,
    int *));

/* stopcode.c */
int eb_have_stop_code EB_P((EB_Appendix *));
EB_Error_Code eb_stop_code EB_P((EB_Appendix *, int *));

/* widealt.c */
int eb_have_wide_alt EB_P((EB_Appendix *));
EB_Error_Code eb_wide_alt_start EB_P((EB_Appendix *, int *));
EB_Error_Code eb_wide_alt_end EB_P((EB_Appendix *, int *));
EB_Error_Code eb_wide_alt_character_text EB_P((EB_Appendix *, int, char *));
EB_Error_Code eb_forward_wide_alt_character EB_P((EB_Appendix *, int, int *));
EB_Error_Code eb_backward_wide_alt_character EB_P((EB_Appendix *, int, int *));

/* for backward compatibility */
#define eb_initialize_all_appendix_subbooks eb_load_all_appendix_subbooks

#ifdef __cplusplus
}
#endif

#endif /* not EB_APPENDIX_H */
