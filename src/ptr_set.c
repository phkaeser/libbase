/* ========================================================================= */
/**
 * @file ptr_set.c
 *
 * Interface for a simple stack to store pointers.
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

#include <libbase/avltree.h>
#include <libbase/def.h>
#include <libbase/log_wrappers.h>
#include <libbase/ptr_set.h>
#include <libbase/test.h>
#include <stdbool.h>
#include <stdlib.h>

/* == Declarations ========================================================= */

/** A set. */
struct _bs_ptr_set_t {
    /** Maps to the tree holding the set elements. */
    bs_avltree_t              *tree_ptr;
};

/** Holder for one element. */
typedef struct {
    /** The tree node. */
    bs_avltree_node_t         avlnode;
    /** Element pointer. */
    void                      *elem_ptr;
} bs_ptr_set_elem_holder_t;

static int elem_compare(
    const bs_avltree_node_t *node_ptr,
    const void *key_ptr);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bs_ptr_set_t *bs_ptr_set_create(void)
{
    bs_ptr_set_t *set_ptr = logged_calloc(1, sizeof(bs_ptr_set_t));
    if (NULL == set_ptr) return NULL;

    set_ptr->tree_ptr = bs_avltree_create(elem_compare, NULL);
    if (NULL == set_ptr->tree_ptr) {
        bs_ptr_set_destroy(set_ptr);
        return NULL;
    }

    return set_ptr;
}

/* ------------------------------------------------------------------------- */
void bs_ptr_set_destroy(bs_ptr_set_t *set_ptr)
{
    if (NULL != set_ptr->tree_ptr) {
        bs_avltree_destroy(set_ptr->tree_ptr);
        set_ptr->tree_ptr = NULL;
    }

    free(set_ptr);
}

/* ------------------------------------------------------------------------- */
bool bs_ptr_set_insert(bs_ptr_set_t *set_ptr, void *elem_ptr)
{
    bs_ptr_set_elem_holder_t *holder_ptr = logged_calloc(
        1, sizeof(bs_ptr_set_elem_holder_t));
    if (NULL == holder_ptr) return false;

    holder_ptr->elem_ptr = elem_ptr;
    bool rv = bs_avltree_insert(
        set_ptr->tree_ptr,
        holder_ptr->elem_ptr,
        &holder_ptr->avlnode,
        false);
    if (!rv) free(holder_ptr);
    return rv;
}

/* ------------------------------------------------------------------------- */
void bs_ptr_set_erase(bs_ptr_set_t *set_ptr, void *elem_ptr)
{
    bs_avltree_node_t *node_ptr = bs_avltree_delete(
        set_ptr->tree_ptr, elem_ptr);
    if (NULL != node_ptr) {
        bs_ptr_set_elem_holder_t *holder_ptr = BS_CONTAINER_OF(
            node_ptr,  bs_ptr_set_elem_holder_t, avlnode);
        free(holder_ptr);
    }
}

/* ------------------------------------------------------------------------- */
bool bs_ptr_set_contains(bs_ptr_set_t *set_ptr, void *elem_ptr)
{
    return NULL != bs_avltree_lookup(set_ptr->tree_ptr, elem_ptr);
}

/* ------------------------------------------------------------------------- */
void *bs_ptr_set_any(bs_ptr_set_t *set_ptr)
{
    bs_avltree_node_t *avlnode_ptr = bs_avltree_min(set_ptr->tree_ptr);
    if (NULL == avlnode_ptr) return NULL;

    bs_ptr_set_elem_holder_t *holder_ptr = BS_CONTAINER_OF(
        avlnode_ptr, bs_ptr_set_elem_holder_t, avlnode);
    return holder_ptr->elem_ptr;
}

/* ------------------------------------------------------------------------- */
bool bs_ptr_set_empty(bs_ptr_set_t *set_ptr)
{
    return 0 == bs_avltree_size(set_ptr->tree_ptr);
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/**
 * Compares two element pointers.
 *
 * @param node_ptr
 * @param key_ptr
 *
 * @return As specified by @ref bs_avltree_node_cmp_t.
 */
int elem_compare(const bs_avltree_node_t *node_ptr, const void *key_ptr)
{
    const bs_ptr_set_elem_holder_t *holder_ptr = BS_CONTAINER_OF(
        node_ptr, const bs_ptr_set_elem_holder_t, avlnode);
    return (size_t)key_ptr - (size_t)holder_ptr->elem_ptr;
}

/* == Test Functions ======================================================= */

static void bs_ptr_set_test(bs_test_t *test_ptr);

/** Unit test cases. */
static const bs_test_case_t   bs_ptr_set_test_cases[] = {
    { 1, "test", bs_ptr_set_test },
    { 0, NULL, NULL }
};

const bs_test_set_t bs_ptr_set_test_set = {
    true, "ptr_set", bs_ptr_set_test_cases
};

void bs_ptr_set_test(bs_test_t *test_ptr)
{
    bs_ptr_set_t *set_ptr = bs_ptr_set_create();
    BS_TEST_VERIFY_NEQ(test_ptr, set_ptr, NULL);
    void *data1_ptr = (void*)-1;
    void *data2_ptr = (void*)2;

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_set_empty(set_ptr));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_ptr_set_contains(set_ptr, data1_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_ptr_set_any(set_ptr));

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_set_insert(set_ptr, data1_ptr));

    BS_TEST_VERIFY_FALSE(test_ptr, bs_ptr_set_empty(set_ptr));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_set_contains(set_ptr, data1_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, data1_ptr, bs_ptr_set_any(set_ptr));

    BS_TEST_VERIFY_FALSE(test_ptr, bs_ptr_set_insert(set_ptr, data1_ptr));

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_set_insert(set_ptr, data2_ptr));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_set_contains(set_ptr, data1_ptr));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_set_contains(set_ptr, data2_ptr));
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, bs_ptr_set_any(set_ptr));

    bs_ptr_set_erase(set_ptr, data1_ptr);
    bs_ptr_set_erase(set_ptr, data2_ptr);

    BS_TEST_VERIFY_FALSE(test_ptr, bs_ptr_set_contains(set_ptr, data1_ptr));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_ptr_set_contains(set_ptr, data2_ptr));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_set_empty(set_ptr));

    bs_ptr_set_destroy(set_ptr);
}

/* == End of ptr_set.c ===================================================== */
