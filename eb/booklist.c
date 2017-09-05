/*
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

#include "build-pre.h"
#include "eb.h"
#include "error.h"
#ifdef ENABLE_EBNET
#include "ebnet.h"
#endif
#include "build-post.h"

/*
 * Initial value of `max_entry_count' in `EB_BookList'.
 */
#define EB_INITIAL_BOOKLIST_MAX_ENTRY_COUNT	16

/*
 * Initialize a book list.
 */
void
eb_initialize_booklist(booklist)
    EB_BookList *booklist;
{
    LOG(("in: eb_initialize_booklist()"));

    booklist->entry_count = 0;
    booklist->max_entry_count = 0;
    booklist->entries = NULL;

    LOG(("out: eb_initialize_booklist()"));
}


/*
 * Finalize a book list.
 */
void
eb_finalize_booklist(booklist)
    EB_BookList *booklist;
{
    int i;

    LOG(("in: eb_finalize_booklist()"));

    if (booklist->entries != NULL) {
	for (i = 0; i < booklist->entry_count; i++) {
	    free(booklist->entries[i].name);
	    free(booklist->entries[i].title);
	}
	free(booklist->entries);
	booklist->entries = NULL;
    }
    booklist->entry_count = 0;
    booklist->max_entry_count = 0;

    LOG(("out: eb_finalize_booklist()"));
}


/*
 * Get a list of books from an EBNET server.
 */
EB_Error_Code
eb_get_booklist(booklist, path)
    EB_BookList *booklist;
    const char *path;
{
    int i;

    if (!is_ebnet_url(path))
	return EB_ERR_BAD_FILE_NAME;

    for (i = 0; i < booklist->entry_count; i++) {
	free(booklist->entries[i].name);
	free(booklist->entries[i].title);
    }

#ifdef ENABLE_EBNET
    return ebnet_get_booklist(booklist, path);
#else
    return EB_ERR_EBNET_UNSUPPORTED;
#endif
}


/*
 * Add a book entry to `booklist'.
 */
EB_Error_Code
eb_booklist_add_book(booklist, name, title)
    EB_BookList *booklist;
    const char *name;
    const char *title;
{
    int new_max_entry_count;
    EB_BookList_Entry *new_entries;
    char *new_name = NULL;
    char *new_title = NULL;
    EB_Error_Code error_code;

    LOG(("in: eb_booklist_add_book(name=%s, title=%s)", name, title));

    if (booklist->entry_count == booklist->max_entry_count) {
	if (booklist->max_entry_count == 0) {
	    new_max_entry_count = EB_INITIAL_BOOKLIST_MAX_ENTRY_COUNT;
	    new_entries = (EB_BookList_Entry *)
		malloc(sizeof(EB_BookList_Entry) * new_max_entry_count);
	} else {
	    new_max_entry_count = booklist->max_entry_count * 2;
	    new_entries = (EB_BookList_Entry *)realloc(booklist->entries,
		sizeof(EB_BookList_Entry) * new_max_entry_count);
	}
	if (new_entries == NULL) {
	    error_code = EB_ERR_MEMORY_EXHAUSTED;
	    goto failed;
	}
	booklist->max_entry_count = new_max_entry_count;
	booklist->entries = new_entries;
    }

    new_name = (char *)malloc(strlen(name) + 1);
    if (new_name == NULL) {
	error_code = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }
    strcpy(new_name, name);

    new_title = (char *)malloc(strlen(title) + 1);
    if (new_title == NULL) {
	error_code = EB_ERR_MEMORY_EXHAUSTED;
	goto failed;
    }
    strcpy(new_title, title);

    booklist->entries[booklist->entry_count].name = new_name;
    booklist->entries[booklist->entry_count].title = new_title;
    booklist->entry_count++;

    LOG(("out: eb_booklist_add_book() = %s", eb_error_string(EB_SUCCESS)));

    return EB_SUCCESS;

    /*
     * An error occurs...
     */
  failed:
    if (new_name != NULL)
	free(new_name);
    if (new_title != NULL)
	free(new_title);

    LOG(("out: eb_booklist_book_add() = %s", eb_error_string(error_code)));
    return error_code;
}


/*
 * Return the number of books in `booklist'.
 */
int
eb_booklist_book_count(booklist)
    EB_BookList *booklist;
{
    LOG(("in+out: eb_booklist_book_count() = %d", booklist->entry_count));
    return booklist->entry_count;
}


/*
 * Return title of a book entry in `booklist'.
 */
const char *
eb_booklist_book_name(booklist, book_index)
    EB_BookList *booklist;
    int book_index;
{
    const char *name;

    LOG(("in: eb_booklist_book_name()"));

    if (book_index < 0 || booklist->entry_count <= book_index)
	return NULL;
    name = booklist->entries[book_index].name;

    LOG(("out: eb_booklist_book_name() = %s",
	(name == NULL) ? "NULL" : name));

    return name;
}


/*
 * Return name of a book entry in `booklist'.
 */
const char *
eb_booklist_book_title(booklist, book_index)
    EB_BookList *booklist;
    int book_index;
{
    const char *title;

    LOG(("in: eb_booklist_book_title()"));

    if (book_index < 0 || booklist->entry_count <= book_index)
	return NULL;
    title = booklist->entries[book_index].title;

    LOG(("out: eb_booklist_book_title() = %s",
	(title == NULL) ? "NULL" : title));

    return title;
}


