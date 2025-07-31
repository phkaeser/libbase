/* ========================================================================= */
/**
 * @file dllist.c
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

#include <libbase/assert.h>
#include <libbase/def.h>
#include <libbase/dllist.h>
#include <libbase/test.h>
#include <stdbool.h>
#include <stdlib.h>

static bool find_is(bs_dllist_node_t *dlnode_ptr, void *ud_ptr);
static void assert_consistency(const bs_dllist_t *list_ptr);
static bool node_orphaned(const bs_dllist_node_t *dlnode_ptr);

/* == Exported Functions =================================================== */

/* ------------------------------------------------------------------------- */
size_t bs_dllist_size(const bs_dllist_t *list_ptr)
{
    size_t                    count;
    bs_dllist_node_t          *node_ptr;

    node_ptr = list_ptr->head_ptr;
    count = 0;
    while (NULL != node_ptr) {
        node_ptr = node_ptr->next_ptr;
        count++;
    }
    return count;
}

/* ------------------------------------------------------------------------- */
bool bs_dllist_empty(const bs_dllist_t *list_ptr)
{
    return (NULL == list_ptr->head_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_dllist_push_back(bs_dllist_t *list_ptr, bs_dllist_node_t *node_ptr)
{
    BS_ASSERT(NULL == node_ptr->prev_ptr);
    BS_ASSERT(NULL == node_ptr->next_ptr);

    if (NULL != list_ptr->tail_ptr) {
        BS_ASSERT(NULL != list_ptr->head_ptr);
        BS_ASSERT(NULL == list_ptr->tail_ptr->next_ptr);

        node_ptr->prev_ptr = list_ptr->tail_ptr;
        list_ptr->tail_ptr->next_ptr = node_ptr;

        list_ptr->tail_ptr = node_ptr;

    } else {
        BS_ASSERT(NULL == list_ptr->head_ptr);

        list_ptr->head_ptr = node_ptr;
        list_ptr->tail_ptr = node_ptr;
    }
}

/* ------------------------------------------------------------------------- */
void bs_dllist_push_front(bs_dllist_t *list_ptr, bs_dllist_node_t *node_ptr)
{
    BS_ASSERT(NULL == node_ptr->prev_ptr);
    BS_ASSERT(NULL == node_ptr->next_ptr);

    if (NULL != list_ptr->head_ptr) {
        BS_ASSERT(NULL != list_ptr->tail_ptr);
        BS_ASSERT(NULL == list_ptr->head_ptr->prev_ptr);

        node_ptr->next_ptr = list_ptr->head_ptr;
        list_ptr->head_ptr->prev_ptr = node_ptr;

        list_ptr->head_ptr = node_ptr;

    } else {
        BS_ASSERT(NULL == list_ptr->head_ptr);

        list_ptr->head_ptr = node_ptr;
        list_ptr->tail_ptr = node_ptr;
    }
}


/* ------------------------------------------------------------------------- */
bs_dllist_node_t *bs_dllist_pop_back(bs_dllist_t *list_ptr)
{
    bs_dllist_node_t          *node_ptr;

    if (NULL == list_ptr->tail_ptr) {
        BS_ASSERT(NULL == list_ptr->head_ptr);
        return NULL;
    }
    BS_ASSERT(NULL != list_ptr->head_ptr);

    node_ptr = list_ptr->tail_ptr;
    list_ptr->tail_ptr = node_ptr->prev_ptr;
    if (NULL != list_ptr->tail_ptr) {
        list_ptr->tail_ptr->next_ptr = NULL;
    } else {
        BS_ASSERT(list_ptr->head_ptr == node_ptr);
        list_ptr->head_ptr = NULL;
    }
    node_ptr->prev_ptr = NULL;
    return node_ptr;
}

/* ------------------------------------------------------------------------- */
bs_dllist_node_t * bs_dllist_pop_front(bs_dllist_t *list_ptr)
{
    bs_dllist_node_t          *node_ptr;

    if (NULL == list_ptr->head_ptr) {
        BS_ASSERT(NULL == list_ptr->tail_ptr);
        return NULL;
    }
    BS_ASSERT(NULL != list_ptr->tail_ptr);

    node_ptr = list_ptr->head_ptr;
    list_ptr->head_ptr = node_ptr->next_ptr;
    if (NULL != list_ptr->head_ptr) {
        list_ptr->head_ptr->prev_ptr = NULL;
    } else {
        BS_ASSERT(list_ptr->tail_ptr == node_ptr);
        list_ptr->tail_ptr = NULL;
    }

    node_ptr->next_ptr =  NULL;
    return node_ptr;
}

/* ------------------------------------------------------------------------- */
void bs_dllist_remove(bs_dllist_t *list_ptr, bs_dllist_node_t *node_ptr)
{
    BS_ASSERT(bs_dllist_contains(list_ptr, node_ptr));
    if (NULL == node_ptr->prev_ptr) {
        BS_ASSERT(list_ptr->head_ptr == node_ptr);
        list_ptr->head_ptr = node_ptr->next_ptr;
    } else {
        node_ptr->prev_ptr->next_ptr = node_ptr->next_ptr;
    }

    if (NULL == node_ptr->next_ptr) {
        BS_ASSERT(list_ptr->tail_ptr == node_ptr);
        list_ptr->tail_ptr = node_ptr->prev_ptr;
    } else {
        node_ptr->next_ptr->prev_ptr = node_ptr->prev_ptr;
    }

    node_ptr->prev_ptr = NULL;
    node_ptr->next_ptr = NULL;
}

/* ------------------------------------------------------------------------- */
void bs_dllist_insert_node_before(
    bs_dllist_t *list_ptr,
    bs_dllist_node_t *reference_node_ptr,
    bs_dllist_node_t *new_node_ptr)
{
    BS_ASSERT(bs_dllist_contains(list_ptr, reference_node_ptr));
    BS_ASSERT(node_orphaned(new_node_ptr));

    if (NULL == reference_node_ptr->prev_ptr) {
        bs_dllist_push_front(list_ptr, new_node_ptr);
        return;
    }

    reference_node_ptr->prev_ptr->next_ptr = new_node_ptr;
    new_node_ptr->prev_ptr = reference_node_ptr->prev_ptr;

    reference_node_ptr->prev_ptr = new_node_ptr;
    new_node_ptr->next_ptr = reference_node_ptr;
}

/* ------------------------------------------------------------------------- */
bool bs_dllist_contains(
    const bs_dllist_t *list_ptr,
    bs_dllist_node_t *dlnode_ptr)
{
    return bs_dllist_find(list_ptr, find_is, dlnode_ptr) == dlnode_ptr;
}

/* ------------------------------------------------------------------------- */
bs_dllist_node_t *bs_dllist_find(
    const bs_dllist_t *list_ptr,
    bool (*func)(bs_dllist_node_t *dlnode_ptr, void *ud_ptr),
    void *ud_ptr)
{
    for (bs_dllist_node_t *dlnode_ptr = list_ptr->head_ptr;
         dlnode_ptr != NULL;
         dlnode_ptr = dlnode_ptr->next_ptr) {
        if (func(dlnode_ptr, ud_ptr)) return dlnode_ptr;
    }
    return NULL;
}

/* ------------------------------------------------------------------------- */
void bs_dllist_for_each(
    const bs_dllist_t *list_ptr,
    void (*func)(bs_dllist_node_t *dlnode_ptr, void *ud_ptr),
    void *ud_ptr)
{
    bs_dllist_node_t *dlnode_ptr = list_ptr->head_ptr;
    while (NULL != dlnode_ptr) {
        bs_dllist_node_t *current_dlnode_ptr = dlnode_ptr;
        dlnode_ptr = dlnode_ptr->next_ptr;
        func(current_dlnode_ptr, ud_ptr);
    }
}

/* ------------------------------------------------------------------------- */
bool bs_dllist_all(
    const bs_dllist_t *list_ptr,
    bool (*func)(bs_dllist_node_t *dlnode_ptr, void *ud_ptr),
    void *ud_ptr)
{
    for (bs_dllist_node_t *dlnode_ptr = list_ptr->head_ptr;
         dlnode_ptr != NULL;
         dlnode_ptr = dlnode_ptr->next_ptr) {
        if (!func(dlnode_ptr, ud_ptr)) return false;
    }
    return true;
}

/* == Local methods ======================================================== */

/* ------------------------------------------------------------------------- */
void assert_consistency(const bs_dllist_t *list_ptr)
{
    const bs_dllist_node_t *node_ptr;

    // An empty list is consistent if both head/tail are NULL.
    if (NULL == list_ptr->head_ptr || NULL == list_ptr->tail_ptr) {
        BS_ASSERT(NULL == list_ptr->head_ptr);
        BS_ASSERT(NULL == list_ptr->tail_ptr);
        return;
    }

    // Head and tail don't continue further.
    BS_ASSERT(NULL == list_ptr->head_ptr->prev_ptr);
    BS_ASSERT(NULL == list_ptr->tail_ptr->next_ptr);

    for (node_ptr = list_ptr->head_ptr;
         node_ptr != NULL;
         node_ptr = node_ptr->next_ptr) {
        if (NULL != node_ptr->next_ptr) {
            BS_ASSERT(node_ptr == node_ptr->next_ptr->prev_ptr);
        } else {
            BS_ASSERT(node_ptr == list_ptr->tail_ptr);
        }
    }
}

/* ------------------------------------------------------------------------- */
/** Returns whether dlnode_ptr == ud_ptr. */
bool find_is(bs_dllist_node_t *dlnode_ptr, void *ud_ptr)
{
    return dlnode_ptr == ud_ptr;
}

/* ------------------------------------------------------------------------- */
/**
 * Returns whether |dlnode_ptr| is orphaned, ie. not in any list.
 *
 * Note: This does not guarantee that dlnode_ptr is not member of a list,
 * merely that it refers no neighbours. The case of it being the single list
 * node cannot be distinguished.
 *
 * @param dlnode_ptr
 *
 * @reutrn Whether the list holds no references.
 */
bool node_orphaned(const bs_dllist_node_t *dlnode_ptr)
{
    return (NULL == dlnode_ptr->prev_ptr) && (NULL == dlnode_ptr->next_ptr);
}

/* == Tests =============================================================== */

static void bs_dllist_test_back(bs_test_t *test_ptr);
static void bs_dllist_test_front(bs_test_t *test_ptr);
static void bs_dllist_test_remove(bs_test_t *test_ptr);
static void bs_dllist_test_insert(bs_test_t *test_ptr);
static void bs_dllist_test_find(bs_test_t *test_ptr);
static void bs_dllist_test_for_each(bs_test_t *test_ptr);
static void bs_dllist_test_for_each_dtor(bs_test_t *test_ptr);
static void bs_dllist_test_all(bs_test_t *test_ptr);
static void bs_dllist_test_iterator(bs_test_t *test_ptr);

const bs_test_case_t          bs_dllist_test_cases[] = {
    { 1, "push/pop back", bs_dllist_test_back },
    { 1, "push/pop front", bs_dllist_test_front },
    { 1, "remove", bs_dllist_test_remove },
    { 1, "insert", bs_dllist_test_insert },
    { 1, "find", bs_dllist_test_find },
    { 1, "for_each", bs_dllist_test_for_each },
    { 1, "for_each_dtor", bs_dllist_test_for_each_dtor },
    { 1, "all", bs_dllist_test_all },
    { 1, "iterator", bs_dllist_test_iterator },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
/**
 * Tests that push_back and pop_back works.
 */
void bs_dllist_test_back(bs_test_t *test_ptr)
{
    bs_dllist_t               list1 = {};
    bs_dllist_node_t          node1 = {}, node2 = {}, node3 = {};

    bs_dllist_push_back(&list1, &node1);
    bs_dllist_push_back(&list1, &node2);
    bs_dllist_push_back(&list1, &node3);

    BS_TEST_VERIFY_EQ(test_ptr, node1.prev_ptr, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, node1.next_ptr, &node2);

    BS_TEST_VERIFY_EQ(test_ptr, node2.prev_ptr, &node1);
    BS_TEST_VERIFY_EQ(test_ptr, node2.next_ptr, &node3);

    BS_TEST_VERIFY_EQ(test_ptr, node3.prev_ptr, &node2);
    BS_TEST_VERIFY_EQ(test_ptr, node3.next_ptr, NULL);

    BS_TEST_VERIFY_EQ(test_ptr, &node3, bs_dllist_pop_back(&list1));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node3.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node3.next_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, &node2, bs_dllist_pop_back(&list1));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node2.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node2.next_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, &node1, bs_dllist_pop_back(&list1));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node1.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node1.next_ptr);

    BS_TEST_VERIFY_EQ(test_ptr, list1.head_ptr, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, list1.tail_ptr, NULL);
}

/* ------------------------------------------------------------------------- */
/**
 * Tests that push_front and pop_front works.
 */
void bs_dllist_test_front(bs_test_t *test_ptr)
{
    bs_dllist_t               list1 = {};
    bs_dllist_node_t          node1 = {}, node2 = {}, node3 = {};

    bs_dllist_push_front(&list1, &node3);
    bs_dllist_push_front(&list1, &node2);
    bs_dllist_push_front(&list1, &node1);

    BS_TEST_VERIFY_EQ(test_ptr, node1.prev_ptr, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, node1.next_ptr, &node2);

    BS_TEST_VERIFY_EQ(test_ptr, node2.prev_ptr, &node1);
    BS_TEST_VERIFY_EQ(test_ptr, node2.next_ptr, &node3);

    BS_TEST_VERIFY_EQ(test_ptr, node3.prev_ptr, &node2);
    BS_TEST_VERIFY_EQ(test_ptr, node3.next_ptr, NULL);

    BS_TEST_VERIFY_EQ(test_ptr, &node1, bs_dllist_pop_front(&list1));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node1.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node1.next_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, &node2, bs_dllist_pop_front(&list1));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node2.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node2.next_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, &node3, bs_dllist_pop_front(&list1));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node3.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node3.next_ptr);

    BS_TEST_VERIFY_EQ(test_ptr, list1.head_ptr, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, list1.tail_ptr, NULL);
}

/* ------------------------------------------------------------------------- */
void bs_dllist_test_remove(bs_test_t *test_ptr)
{
    bs_dllist_t               list = {};
    bs_dllist_node_t          node1 = {}, node2 = {}, node3 = {}, node4 = {};

    BS_TEST_VERIFY_TRUE(test_ptr, node_orphaned(&node1));

    // Sequence: 1, 2, 3, 4.
    bs_dllist_push_back(&list, &node1);
    assert_consistency(&list);
    bs_dllist_push_back(&list, &node2);
    assert_consistency(&list);
    bs_dllist_push_back(&list, &node3);
    assert_consistency(&list);
    bs_dllist_push_back(&list, &node4);
    assert_consistency(&list);

    BS_TEST_VERIFY_EQ(test_ptr, 4, bs_dllist_size(&list));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_empty(&list));
    BS_TEST_VERIFY_FALSE(test_ptr, node_orphaned(&node1));

    bs_dllist_remove(&list, &node3);
    // Now: 1, 2, 4.
    assert_consistency(&list);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node3.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node3.next_ptr);

    bs_dllist_remove(&list, &node1);
    // Now: 2, 4.
    assert_consistency(&list);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node1.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node1.next_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, node_orphaned(&node1));

    bs_dllist_remove(&list, &node4);
    // Now: 2.
    assert_consistency(&list);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node4.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node4.next_ptr);

    bs_dllist_remove(&list, &node2);
    assert_consistency(&list);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node2.prev_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, node2.next_ptr);

    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_dllist_size(&list));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_empty(&list));
}

/* ------------------------------------------------------------------------- */
void bs_dllist_test_insert(bs_test_t *test_ptr)
{
    bs_dllist_t               list = {};
    bs_dllist_node_t          node1 = {}, node2 = {}, node3 = {};

    BS_TEST_VERIFY_TRUE(test_ptr, node_orphaned(&node1));

    bs_dllist_push_back(&list, &node1);
    bs_dllist_insert_node_before(&list, &node1, &node2);
    assert_consistency(&list);
    BS_TEST_VERIFY_EQ(test_ptr, list.head_ptr, &node2);
    BS_TEST_VERIFY_EQ(test_ptr, node2.next_ptr, &node1);

    bs_dllist_insert_node_before(&list, &node1, &node3);
    assert_consistency(&list);
    BS_TEST_VERIFY_EQ(test_ptr, list.head_ptr, &node2);
    BS_TEST_VERIFY_EQ(test_ptr, node2.next_ptr, &node3);
    BS_TEST_VERIFY_EQ(test_ptr, node3.next_ptr, &node1);
}

/* ------------------------------------------------------------------------- */
/** Function to use in the test for @ref bs_dllist_any. */
static bool test_find(bs_dllist_node_t *dlnode_ptr, void *ud_ptr)
{
    return dlnode_ptr == ud_ptr;
}

/* ------------------------------------------------------------------------- */
void bs_dllist_test_find(bs_test_t *test_ptr)
{
    bs_dllist_t               list = {};
    bs_dllist_node_t          n1 = {}, n2 = {};

    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_dllist_find(&list, test_find, &n1));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_contains(&list, &n1));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_contains(&list, &n2));
    bs_dllist_push_back(&list, &n1);
    BS_TEST_VERIFY_EQ(test_ptr, &n1, bs_dllist_find(&list, test_find, &n1));
    BS_TEST_VERIFY_NEQ(test_ptr, &n2, bs_dllist_find(&list, test_find, &n2));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_contains(&list, &n1));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_contains(&list, &n2));
    bs_dllist_push_back(&list, &n2);
    BS_TEST_VERIFY_EQ(test_ptr, &n1, bs_dllist_find(&list, test_find, &n1));
    BS_TEST_VERIFY_EQ(test_ptr, &n2, bs_dllist_find(&list, test_find, &n2));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_contains(&list, &n1));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_contains(&list, &n2));
}

/* ------------------------------------------------------------------------- */
/** Function to use in the test for @ref bs_dllist_for_each. */
static void test_for_each(
    __UNUSED__ bs_dllist_node_t *dlnode_ptr,
    void *ud_ptr)
{
    *((int*)ud_ptr) += 1;
}

/** Tests @ref bs_dllist_for_each. */
void bs_dllist_test_for_each(bs_test_t *test_ptr)
{
    bs_dllist_t               list = {};
    bs_dllist_node_t          n1 = {}, n2 = {};
    int                       outcome = 0;

    bs_dllist_for_each(&list, test_for_each, &outcome);
    BS_TEST_VERIFY_EQ(test_ptr, 0, outcome);

    bs_dllist_push_back(&list, &n1);
    outcome = 0;
    bs_dllist_for_each(&list, test_for_each, &outcome);
    BS_TEST_VERIFY_EQ(test_ptr, 1, outcome);

    bs_dllist_push_back(&list, &n2);
    outcome = 0;
    bs_dllist_for_each(&list, test_for_each, &outcome);
    BS_TEST_VERIFY_EQ(test_ptr, 2, outcome);
}

/* ------------------------------------------------------------------------- */
/** Helper: dtor for the allocated @ref bs_dllist_node_t. */
static void test_for_each_dtor(bs_dllist_node_t *dlnode_ptr, void *ud_ptr)
{
    bs_dllist_remove(ud_ptr, dlnode_ptr);
    free(dlnode_ptr);
}

/** Runs bs_dllist_for_each with a dtor, ensuring no invalid access. */
void bs_dllist_test_for_each_dtor(bs_test_t *test_ptr)
{
    bs_dllist_t list = {};

    for (int i = 0; i < 5; ++i) {
        bs_dllist_node_t *node_ptr = BS_ASSERT_NOTNULL(
            calloc(1, sizeof(bs_dllist_node_t)));
        bs_dllist_push_back(&list, node_ptr);
    }
    BS_TEST_VERIFY_EQ(test_ptr, 5, bs_dllist_size(&list));

    bs_dllist_for_each(&list, test_for_each_dtor, &list);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_empty(&list));
}

/* ------------------------------------------------------------------------- */
/** Callback for @ref bs_dllist_all. */
static bool test_all(__UNUSED__ bs_dllist_node_t *dlnode_ptr, void *ud_ptr)
{
    int *i_ptr = ud_ptr;
    *i_ptr += 1;
    return *i_ptr != 2;
}

/** Tests @ref bs_dllist_all. */
void bs_dllist_test_all(bs_test_t *test_ptr)
{
    bs_dllist_t list = {};
    bs_dllist_node_t n1 = {}, n2 = {}, n3 = {};
    int calls = 0;

    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_all(&list, test_all, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 0, calls);

    bs_dllist_push_back(&list, &n1);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_all(&list, test_all, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 1, calls);

    bs_dllist_push_back(&list, &n2);
    calls = 0;
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_all(&list, test_all, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 2, calls);

    bs_dllist_push_back(&list, &n3);
    calls = 0;
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_all(&list, test_all, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 2, calls);
}

/* ------------------------------------------------------------------------- */
void bs_dllist_test_iterator(bs_test_t *test_ptr)
{
    bs_dllist_t list = {};
    bs_dllist_node_t n1 = {}, n2 = {};
    bs_dllist_push_back(&list, &n1);
    bs_dllist_push_back(&list, &n2);

    bs_dllist_node_iterator_t it = bs_dllist_node_iterator_forward;
    BS_TEST_VERIFY_EQ(test_ptr, NULL, it(NULL));
    BS_TEST_VERIFY_EQ(test_ptr, &n2, it(&n1));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, it(&n2));

    it = bs_dllist_node_iterator_backward;
    BS_TEST_VERIFY_EQ(test_ptr, NULL, it(NULL));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, it(&n1));
    BS_TEST_VERIFY_EQ(test_ptr, &n1, it(&n2));
}

/* == End of dllist.c ===================================================== */
