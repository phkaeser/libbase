/* ========================================================================= */
/**
 * @file avltree.h
 * Implements an AVL tree, with nodes provided such they can be embedded in
 * the element's struct.
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
#ifndef __LIBBASE_AVLTREE_H__
#define __LIBBASE_AVLTREE_H__

#include "test.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** The tree. */
typedef struct _bs_avltree_t bs_avltree_t;
/** A tree node. */
typedef struct _bs_avltree_node_t bs_avltree_node_t;

/** Actual struct of the tree node. */
struct _bs_avltree_node_t {
    /** Back-link to the parent node. NULL for the root node. */
    bs_avltree_node_t         *parent_ptr;
    /** Links to the left (smaller) node. NULL if this is the smallest node. */
    bs_avltree_node_t         *left_ptr;
    /** Links to the left (greater) node. NULL if this is the greatest node. */
    bs_avltree_node_t         *right_ptr;
    /** Node balance. */
    int8_t                    balance;
};

/**
 * Functor type to compare two avltree nodes.
 *
 * @param node_ptr
 * @param key_ptr
 *
 * @return
 * - a negative value if node_ptr is less than key_ptr
 * - 0 if node equals key
 * - a positive value if node_ptr is greater than key_ptr
 */
typedef int (*bs_avltree_node_cmp_t)(const bs_avltree_node_t *node_ptr,
                                     const void *key_ptr);

/**
 * Functor type to destroy an avltree node.
 *
 * @param node_ptr
 */
typedef void (*bs_avltree_node_destroy_t)(bs_avltree_node_t *node_ptr);

/**
 * Creates a tree.
 *
 * @param cmp
 * @param destroy Optionally, a functor to destroy nodes. If destroy is NULL,
 *     the tree will not destroy any nodes when overwriting or flushing.
 *
 * @return A pointer to the tree, or NULL on error.
 */
bs_avltree_t *bs_avltree_create(bs_avltree_node_cmp_t cmp,
                                bs_avltree_node_destroy_t destroy);

/**
 * Destroys the tree. Also destroys all remaining elements.
 *
 * @param tree_ptr
 */
void bs_avltree_destroy(bs_avltree_t *tree_ptr);

/** Returns the node matching |key_ptr| in the tree. */
bs_avltree_node_t *bs_avltree_lookup(bs_avltree_t *tree_ptr,
                                     const void *key_ptr);

/**
 * Inserts a node into the tree.
 *
 * @param tree_ptr
 * @param key_ptr The key for this node, required for insert.
 * @param node_ptr The node.
 * @param do_overwrite Whether to overwrite any already-existing node at the
 *     specified |key_ptr|. The overwritten node will be destroyed, if a
 *     destroy method was specified.
 *
 * @return Whether the insert succeeded. It can fail if |do_overwrite| is false
 *     and a node at |key_ptr| already exists.
 */
bool bs_avltree_insert(bs_avltree_t *tree_ptr,
                       const void *key_ptr,
                       bs_avltree_node_t *node_ptr,
                       bool do_overwrite);

/**
 * Deletes a node from the tree.
 *
 * @param tree_ptr
 * @param key_ptr The key for the node that is to be deleted from the tree.
 *
 * @return A pointer to the deleted node, or NULL if it was not found.
 *     Note: The node will NOT be destroyed.
 */
bs_avltree_node_t *bs_avltree_delete(bs_avltree_t *tree_ptr,
                                     const void *key_ptr);

/** Returns the minimum node of the tree. */
bs_avltree_node_t *bs_avltree_min(bs_avltree_t *tree_ptr);

/** Returns the maximum node of the tree. */
bs_avltree_node_t *bs_avltree_max(bs_avltree_t *tree_ptr);

/** Returns the size of the tree. */
size_t bs_avltree_size(const bs_avltree_t *tree_ptr);

/** Returns the next-larger node from the tree, seen from |node_ptr|. */
bs_avltree_node_t *bs_avltree_node_next(bs_avltree_t *tree_ptr,
                                        bs_avltree_node_t *node_ptr);
/** Returns the next-smaller node from the tree, seen from |node_ptr|. */
bs_avltree_node_t *bs_avltree_node_prev(bs_avltree_t *tree_ptr,
                                        bs_avltree_node_t *node_ptr);

/**
 * Helper: Comparator to compare two pointers.
 *
 * To be used for @ref bs_avltree_node_cmp_t, after the comparison function
 * looks up the key from the `node_ptr` argument.
 *
 * @param node_key_ptr        The key obtained for the node under comparison.
 * @param key_ptr             As passed from @ref bs_avltree_node_tmp_t.
 *
 * @return See @ref bs_avltree_node_cmp_t.
 */
int bs_avltree_cmp_ptr(const void *node_key_ptr,
                       const void *key_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_avltree_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_AVLTREE_H__ */
/* == End of avltree.h ===================================================== */
