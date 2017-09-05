/*                                                            -*- C -*-
 * Copyright (c) 2003
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

/*
 * ������ˡ:
 *     font <appendix-path> <subbook-index>
 * ��:
 *     font /cdrom 0
 * ����:
 *     <appendix-path> �ǻ��ꤷ�� appendix ������������ܤ����ӡ���
 *     �����ܤ�������Ƥ���Ⱦ�ѳ���������ʸ����򤹤٤�ɽ�����ޤ���
 *
 *     ���� appendix ����Ⱦ�ѳ���������ʸ�����������Ƥ��ʤ��ȡ���
 *     �顼�ˤʤ�ޤ���
 *
 *     <subbook-index> �ˤϡ�����оݤ����ܤΥ���ǥå�������ꤷ��
 *     ��������ǥå����ϡ����Ҥκǽ�����ܤ����� 0��1��2 ... ��
 *     �ʤ�ޤ���
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <eb/eb.h>
#include <eb/error.h>
#include <eb/appendix.h>

int
main(argc, argv)
    int argc;
    char *argv[];
{
    EB_Error_Code error_code;
    EB_Appendix app;
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    int subbook_count;
    int subbook_index;
    int alt_start;
    char text[EB_MAX_ALTERNATION_TEXT_LENGTH + 1];
    int i;

    /* ���ޥ�ɹ԰���������å���*/
    if (argc != 3) {
        fprintf(stderr, "Usage: %s appendix-path subbook-index\n",
            argv[0]);
        exit(1);
    }

    /* EB �饤�֥��� `app' ��������*/
    eb_initialize_library();
    eb_initialize_appendix(&app);

    /* appendix �� `app' �˷���դ��롣*/
    error_code = eb_bind_appendix(&app, argv[1]);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: failed to bind the app, %s: %s\n",
            argv[0], eb_error_message(error_code), argv[1]);
        goto die;
    }

    /* ���ܤΰ����������*/
    error_code = eb_appendix_subbook_list(&app, subbook_list,
	&subbook_count);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: failed to get the subbook list, %s\n",
            argv[0], eb_error_message(error_code));
        goto die;
    }

    /* ���ܤΥ���ǥå����������*/
    subbook_index = atoi(argv[2]);

    /*�ָ��ߤ����� (current subbook)�פ����ꡣ*/
    if (eb_set_appendix_subbook(&app, subbook_list[subbook_index])
	< 0) {
        fprintf(stderr, "%s: failed to set the current subbook, %s\n",
            argv[0], eb_error_message(error_code));
        goto die;
    }

    /* �����γ��ϰ��֤������*/
    error_code = eb_narrow_alt_start(&app, &alt_start);
    if (error_code != EB_SUCCESS) {
        fprintf(stderr, "%s: failed to get font information, %s\n",
            argv[0], eb_error_message(error_code));
        goto die;
    }

    i = alt_start;
    for (;;) {
        /* ����������ʸ����������*/
	error_code = eb_narrow_alt_character_text(&app, i, text);
	if (error_code != EB_SUCCESS) {
            fprintf(stderr, "%s: failed to get font data, %s\n",
                argv[0], eb_error_message(error_code));
            goto die;
        }

	/* ������������ʸ�������ϡ�*/
	printf("%04x: %s\n", i, text);

        /* ������ʸ���ֹ���Ŀʤ�롣*/
	error_code = eb_forward_narrow_alt_character(&app, 1, &i);
	if (error_code != EB_SUCCESS)
	    break;
    }
        
    /* appendix �� EB �饤�֥������Ѥ�λ��*/
    eb_finalize_appendix(&app);
    eb_finalize_library();
    exit(0);

    /* ���顼ȯ���ǽ�λ����Ȥ��ν�����*/
  die:
    eb_finalize_appendix(&app);
    eb_finalize_library();
    exit(1);
}