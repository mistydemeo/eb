/*
 * Copyright (c) 2000  Motoyuki Kasahara
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

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#ifndef ENABLE_PTHREAD
#define pthread_mutex_lock(m)
#define pthread_mutex_unlock(m)
#endif

/*
 * Mutex for gettext function call.
 */
#if defined(ENABLE_NLS) && defined(ENABLE_PTHREAD)
static pthread_mutex_t gettext_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


/*
 * Thread safe version of gettext().
 */
char *
eb_gettext_r(message_id, buffer, buffer_size)
    const char *message_id;
    char *buffer;
    size_t buffer_size;
{
    const char *message;

    pthread_mutex_lock(&gettext_mutex);
#ifdef ENABLE_NLS
    message = gettext(message_id);
#else
    message = message_id;
#endif /* ENABLE_NLS */
    strncpy(buffer, message, buffer_size);
    pthread_mutex_unlock(&gettext_mutex);
    *(buffer + buffer_size) = '\0';

    return buffer;
}


/*
 * Thread safe version of dgettext().
 */
char *
eb_dgettext_r(domain_name, message_id, buffer, buffer_size)
    const char *domain_name;
    const char *message_id;
    char *buffer;
    size_t buffer_size;
{
    const char *message;

    pthread_mutex_lock(&gettext_mutex);
#ifdef ENABLE_NLS
    message = dgettext(domain_name, message_id);
#else
    message = message_id;
#endif /* ENABLE_NLS */
    strncpy(buffer, message, buffer_size);
    pthread_mutex_unlock(&gettext_mutex);
    *(buffer + buffer_size) = '\0';

    return buffer;
}


/*
 * Thread safe version of dcgettext().
 */
char *
eb_dcgettext_r(domain_name, message_id, buffer, buffer_size, category)
    const char *domain_name;
    const char *message_id;
    int category;
    char *buffer;
    size_t buffer_size;
{
    const char *message;

    pthread_mutex_lock(&gettext_mutex);
#ifdef ENABLE_NLS
    message = dcgettext(domain_name, message_id, category);
#else
    message = message_id;
#endif /* ENABLE_NLS */
    strncpy(buffer, message, buffer_size);
    pthread_mutex_unlock(&gettext_mutex);
    *(buffer + buffer_size) = '\0';

    return buffer;
}


/*
 * Thread safe version of textdomain().
 */
char *
eb_textdomain_r(domain_name)
    const char *domain_name;
{
    char *return_value;

#ifdef ENABLE_NLS
    pthread_mutex_lock(&gettext_mutex);
    return_value = textdomain(domain_name);
    pthread_mutex_unlock(&gettext_mutex);
#else
    return_value = (char *)domain_name;
#endif /* ENABLE_NLS */

    return return_value;
}


/*
 * Thread safe version of bindtextdomain().
 */
char *
eb_bindtextdomain_r(domain_name, directory_name)
    const char *domain_name;
    const char *directory_name;
{
    char *return_value;

#ifdef ENABLE_NLS
    pthread_mutex_lock(&gettext_mutex);
    return_value = bindtextdomain(domain_name, directory_name);
    pthread_mutex_unlock(&gettext_mutex);
#else
    return_value = (char *)directory_name;
#endif /* ENABLE_NLS */

    return return_value;
}


