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

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#endif

#include "defs.h"

/*
 * Examine whether built library supports Pthread.
 */
int
eb_pthread_enabled()
{
#ifdef ENABLE_PTHREAD
    return 1;
#else
    return 0;
#endif
}


/*
 * These functions are compiled only when ENABLE_PTHREAD is defined.
 */
#ifdef ENABLE_PTHREAD

/*
 * Ininialize a lock manager.
 */
void
eb_initialize_lock(lock)
    EB_Lock *lock;
{
    pthread_mutex_init(&lock->lock_count_mutex, NULL);
    pthread_mutex_init(&lock->entity_mutex, NULL);
    lock->lock_count = 0;
}


/*
 * Finalize a lock manager.
 */
void
eb_finalize_lock(lock)
    EB_Lock *lock;
{
    /* Nothing to be done. */
}


/*
 * Lock an entity.
 */
void
eb_lock(lock)
    EB_Lock *lock;
{
    pthread_mutex_lock(&lock->lock_count_mutex);
    if (lock->lock_count == 0)
	pthread_mutex_lock(&lock->entity_mutex);
    lock->lock_count++;
    pthread_mutex_unlock(&lock->lock_count_mutex);
}


/*
 * Unlock an entity.
 */
void
eb_unlock(lock)
    EB_Lock *lock;
{
    pthread_mutex_lock(&lock->lock_count_mutex);
    if (0 < lock->lock_count) {
	lock->lock_count--;
	if (lock->lock_count == 0)
	    pthread_mutex_unlock(&lock->entity_mutex);
    }
    pthread_mutex_unlock(&lock->lock_count_mutex);
}

#endif /* ENABLE_PTHREAD */
