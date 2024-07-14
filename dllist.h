/* ========================================================================= */
/**
 * @file dllist.h
 * Provides a double-linked list.
 *
 * @copyright
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __LIBBASE_DLLIST_H__
#define __LIBBASE_DLLIST_H__

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** A doubly-linked list. */
typedef struct _bs_dllist_t bs_dllist_t;
/** A node in a doubly-linked list. */
typedef struct _bs_dllist_node_t bs_dllist_node_t;

/** Details of the list. */
struct _bs_dllist_t {
    /** Head of the double-linked list. NULL if empty. */
    bs_dllist_node_t          *head_ptr;
    /** Tail of the double-linked list. NULL if empty. */
    bs_dllist_node_t          *tail_ptr;
};

/** Details of said node. */
struct _bs_dllist_node_t {
    /** The previous node, or NULL if this is the only node. */
    bs_dllist_node_t          *prev_ptr;
    /** The next node, or NULL if this is the only node. */
    bs_dllist_node_t          *next_ptr;
};

/** Returns the number of elements in |list_ptr|. */
size_t bs_dllist_size(const bs_dllist_t *list_ptr);
/** Returns whether the list is empty. */
bool bs_dllist_empty(const bs_dllist_t *list_ptr);

/** Adds |node_ptr| at the back of |list_ptr|. */
void bs_dllist_push_back(bs_dllist_t *list_ptr, bs_dllist_node_t *node_ptr);
/** Adds |node_ptr| at the front of |list_ptr|. */
void bs_dllist_push_front(bs_dllist_t *list_ptr, bs_dllist_node_t *node_ptr);

/** Retrieves the node from the back of |list_ptr|, or NULL if empty. */
bs_dllist_node_t *bs_dllist_pop_back(bs_dllist_t *list_ptr);
/** Retrieves the node from the front of |list_ptr|, or NULL if empty. */
bs_dllist_node_t * bs_dllist_pop_front(bs_dllist_t *list_ptr);

/** Removes the node from the list. */
void bs_dllist_remove(bs_dllist_t *list_ptr, bs_dllist_node_t *node_ptr);

/** Inserts `new_node_ptr` into the list, before `reference_node_ptr`. */
void bs_dllist_insert_node_before(
    bs_dllist_t *list_ptr,
    bs_dllist_node_t *reference_node_ptr,
    bs_dllist_node_t *new_node_ptr);

/** Returns whether |list_ptr| contains |dlnode_ptr|. */
bool bs_dllist_contains(
    const bs_dllist_t *list_ptr,
    bs_dllist_node_t *dlnode_ptr);

/**
 * Returns the node for which |func()| is true.
 *
 * @param list_ptr
 * @param func
 * @param ud_ptr
 *
 * @return A pointer to the corresponding @ref bs_dllist_node_t or NULL.
 */
bs_dllist_node_t *bs_dllist_find(
    const bs_dllist_t *list_ptr,
    bool (*func)(bs_dllist_node_t *dlnode_ptr, void *ud_ptr),
    void *ud_ptr);

/**
 * Runs |func()| for each of the nodes in |list_ptr|.
 *
 * The list iterator is kept safe, so it is permitted to remove the called-back
 * node from the list: Useful for eg. destroying all list elements.
 *
 * @param list_ptr
 * @param func
 * @param ud_ptr
 */
void bs_dllist_for_each(
    const bs_dllist_t *list_ptr,
    void (*func)(bs_dllist_node_t *dlnode_ptr, void *ud_ptr),
    void *ud_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_dllist_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_DLLIST_H__ */
/* == End of dllist.h ====================================================== */
