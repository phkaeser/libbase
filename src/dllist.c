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


void _bs_dllist_sort_split_and_merge(
    bs_dllist_t *list_ptr,
    size_t list_size,
    int (*compare_fn)(const bs_dllist_node_t *, const bs_dllist_node_t*));
static bs_dllist_t _bs_dllist_sort_merge(
    bs_dllist_t *left_ptr,
    bs_dllist_t *right_ptr,
    int (*compare_fn)(const bs_dllist_node_t *, const bs_dllist_node_t*));

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
void bs_dllist_append(bs_dllist_t *list_ptr, bs_dllist_t *appendix_ptr)
{
    if (bs_dllist_empty(appendix_ptr)) return;
    if (bs_dllist_empty(list_ptr)) {
        *list_ptr = *appendix_ptr;
        *appendix_ptr = (bs_dllist_t){};
        return;
    }

    list_ptr->tail_ptr->next_ptr = appendix_ptr->head_ptr;
    appendix_ptr->head_ptr->prev_ptr = list_ptr->tail_ptr;
    list_ptr->tail_ptr = appendix_ptr->tail_ptr;
    *appendix_ptr = (bs_dllist_t){};
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

/* ------------------------------------------------------------------------- */
bool bs_dllist_any(
    const bs_dllist_t *list_ptr,
    bool (*func)(bs_dllist_node_t *dlnode_ptr, void *ud_ptr),
    void *ud_ptr)
{
    for (bs_dllist_node_t *dlnode_ptr = list_ptr->head_ptr;
         dlnode_ptr != NULL;
         dlnode_ptr = dlnode_ptr->next_ptr) {
        if (func(dlnode_ptr, ud_ptr)) return true;
    }
    return false;
}

/* ------------------------------------------------------------------------- */
void bs_dllist_sort(
    bs_dllist_t *list_ptr,
    int (*compare_fn)(const bs_dllist_node_t *, const bs_dllist_node_t*))
{
    _bs_dllist_sort_split_and_merge(
        list_ptr, bs_dllist_size(list_ptr), compare_fn);
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

/* ------------------------------------------------------------------------- */
/** Implements merge sort for `list_ptr` with known size. */
void _bs_dllist_sort_split_and_merge(
    bs_dllist_t *list_ptr,
    size_t list_size,
    int (*compare_fn)(const bs_dllist_node_t *, const bs_dllist_node_t*))
{
    if (1 >= list_size) return;

    // Split the lists. Since n >= 2, we have non-zero elements on each side.
    bs_dllist_t left = {}, right = {};
    size_t left_size = 0;
    bs_dllist_node_t *dln = list_ptr->head_ptr;
    for (; left_size < list_size / 2; ++left_size) dln = dln->next_ptr;
    left = (bs_dllist_t){
        .head_ptr = list_ptr->head_ptr,
        .tail_ptr = dln->prev_ptr
    };
    right = (bs_dllist_t){
        .head_ptr = dln,
        .tail_ptr = list_ptr->tail_ptr
    };
    left.tail_ptr->next_ptr = NULL;
    right.head_ptr->prev_ptr = NULL;

    _bs_dllist_sort_split_and_merge(&left, left_size, compare_fn);
    _bs_dllist_sort_split_and_merge(&right, list_size - left_size, compare_fn);

    *list_ptr = _bs_dllist_sort_merge(&left, &right, compare_fn);
}

/* ------------------------------------------------------------------------- */
/** Sorted merge lists. `left_ptr` and `right_ptr` expected to be sorted. */
bs_dllist_t _bs_dllist_sort_merge(
    bs_dllist_t *left_ptr,
    bs_dllist_t *right_ptr,
    int (*compare_fn)(const bs_dllist_node_t *, const bs_dllist_node_t*))
{
    bs_dllist_t merged = {};
    while (NULL != left_ptr->head_ptr && NULL != right_ptr->head_ptr) {
        if (0 >= compare_fn(left_ptr->head_ptr, right_ptr->head_ptr)) {
            bs_dllist_push_back(&merged, bs_dllist_pop_front(left_ptr));
        } else {
            bs_dllist_push_back(&merged, bs_dllist_pop_front(right_ptr));
        }
    }

    bs_dllist_append(&merged, left_ptr);
    bs_dllist_append(&merged, right_ptr);
    return merged;
}

/* == Tests =============================================================== */

static void bs_dllist_test_back(bs_test_t *test_ptr);
static void bs_dllist_test_front(bs_test_t *test_ptr);
static void bs_dllist_test_remove(bs_test_t *test_ptr);
static void bs_dllist_test_insert(bs_test_t *test_ptr);
static void bs_dllist_test_append(bs_test_t *test_ptr);
static void bs_dllist_test_find(bs_test_t *test_ptr);
static void bs_dllist_test_for_each(bs_test_t *test_ptr);
static void bs_dllist_test_for_each_dtor(bs_test_t *test_ptr);
static void bs_dllist_test_all(bs_test_t *test_ptr);
static void bs_dllist_test_any(bs_test_t *test_ptr);
static void bs_dllist_test_sort(bs_test_t *test_ptr);
static void bs_dllist_test_sort_random(bs_test_t *test_ptr);
static void bs_dllist_test_iterator(bs_test_t *test_ptr);

/** Unit test cases. */
static const bs_test_case_t   bs_dllist_test_cases[] = {
    { true, "push/pop back", bs_dllist_test_back },
    { true, "push/pop front", bs_dllist_test_front },
    { true, "remove", bs_dllist_test_remove },
    { true, "insert", bs_dllist_test_insert },
    { true, "append", bs_dllist_test_append },
    { true, "find", bs_dllist_test_find },
    { true, "for_each", bs_dllist_test_for_each },
    { true, "for_each_dtor", bs_dllist_test_for_each_dtor },
    { true, "all", bs_dllist_test_all },
    { true, "any", bs_dllist_test_any },
    { true, "sort", bs_dllist_test_sort },
    { true, "sort_random", bs_dllist_test_sort_random },
    { true, "iterator", bs_dllist_test_iterator },
    { false, NULL, NULL }
};

const bs_test_set_t           bs_dllist_test_set = BS_TEST_SET(
    true, "dllist", bs_dllist_test_cases);

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
void bs_dllist_test_append(bs_test_t *test_ptr)
{
    bs_dllist_t               l1 = {}, l2 = {};
    bs_dllist_node_t          n1 = {}, n2 = {};

    bs_dllist_append(&l1, &l2);
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_dllist_size(&l1));
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_dllist_size(&l2));

    bs_dllist_push_back(&l2, &n1);
    bs_dllist_append(&l1, &l2);
    BS_TEST_VERIFY_EQ(test_ptr, 1, bs_dllist_size(&l1));
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_dllist_size(&l2));

    // Append again, must not change anything.
    bs_dllist_append(&l1, &l2);
    BS_TEST_VERIFY_EQ(test_ptr, 1, bs_dllist_size(&l1));
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_dllist_size(&l2));

    bs_dllist_push_back(&l2, &n2);
    bs_dllist_append(&l1, &l2);
    BS_TEST_VERIFY_EQ(test_ptr, 2, bs_dllist_size(&l1));
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_dllist_size(&l2));

    BS_TEST_VERIFY_EQ(test_ptr, &n1, l1.head_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, &n2, l1.tail_ptr);
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
/** Callback for @ref bs_dllist_any. Return true for 2nd call. */
static bool test_any(__UNUSED__ bs_dllist_node_t *dlnode_ptr, void *ud_ptr)
{
    int *i_ptr = ud_ptr;
    *i_ptr += 1;
    return *i_ptr == 2;
}

/** Tests @ref bs_dllist_any. */
void bs_dllist_test_any(bs_test_t *test_ptr)
{
    bs_dllist_t list = {};
    bs_dllist_node_t n1 = {}, n2 = {}, n3 = {};
    int calls = 0;

    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_any(&list, test_any, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 0, calls);

    bs_dllist_push_back(&list, &n1);
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dllist_any(&list, test_any, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 1, calls);

    bs_dllist_push_back(&list, &n2);
    calls = 0;
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_any(&list, test_any, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 2, calls);

    bs_dllist_push_back(&list, &n3);
    calls = 0;
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dllist_any(&list, test_any, &calls));
    BS_TEST_VERIFY_EQ(test_ptr, 2, calls);
}

/* ------------------------------------------------------------------------- */
/** Test struct, for sorting. Node with a value. */
struct _bs_dllist_test_sort_node {
    /** List node. */
    bs_dllist_node_t          dlnode;
    /** Value. */
    int                       i;
    /** Position. */
    size_t                    pos;
};
/** Comparator, for testing. */
static int _bs_dllist_test_sort_cmp(
    const bs_dllist_node_t *left_node_ptr,
    const bs_dllist_node_t *right_node_ptr) {
    struct _bs_dllist_test_sort_node *lnp = BS_CONTAINER_OF(
        left_node_ptr, struct _bs_dllist_test_sort_node, dlnode);
    struct _bs_dllist_test_sort_node *rnp = BS_CONTAINER_OF(
        right_node_ptr, struct _bs_dllist_test_sort_node, dlnode);
    if (lnp->i < rnp->i) {
        return -1;
    } else if (lnp->i > rnp->i) {
        return 1;
    }
    return 0;
}

/** Tests @ref bs_dllist_sort. */
void bs_dllist_test_sort(bs_test_t *test_ptr)
{
    bs_dllist_t list = {};
    struct _bs_dllist_test_sort_node n1 = { .i = 1 }, n2 = { .i = 2 },
        n3 = { .i = 3 }, n4 = { .i = 4 }, other1 = { .i = 1 };

    bs_dllist_sort(&list, _bs_dllist_test_sort_cmp);
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_dllist_size(&list));

    bs_dllist_push_back(&list, &n3.dlnode);
    bs_dllist_sort(&list, _bs_dllist_test_sort_cmp);
    BS_TEST_VERIFY_EQ(test_ptr, &n3.dlnode, bs_dllist_pop_front(&list));

    bs_dllist_push_back(&list, &n4.dlnode);
    bs_dllist_push_back(&list, &n2.dlnode);
    bs_dllist_sort(&list, _bs_dllist_test_sort_cmp);
    BS_TEST_VERIFY_EQ(test_ptr, &n2.dlnode, bs_dllist_pop_front(&list));
    BS_TEST_VERIFY_EQ(test_ptr, &n4.dlnode, bs_dllist_pop_front(&list));

    bs_dllist_push_back(&list, &n4.dlnode);
    bs_dllist_push_back(&list, &n2.dlnode);
    bs_dllist_push_back(&list, &n3.dlnode);
    bs_dllist_push_back(&list, &n1.dlnode);
    bs_dllist_sort(&list, _bs_dllist_test_sort_cmp);
    BS_TEST_VERIFY_EQ(test_ptr, &n1.dlnode, bs_dllist_pop_front(&list));
    BS_TEST_VERIFY_EQ(test_ptr, &n2.dlnode, bs_dllist_pop_front(&list));
    BS_TEST_VERIFY_EQ(test_ptr, &n3.dlnode, bs_dllist_pop_front(&list));
    BS_TEST_VERIFY_EQ(test_ptr, &n4.dlnode, bs_dllist_pop_front(&list));

    bs_dllist_push_back(&list, &n1.dlnode);
    bs_dllist_push_back(&list, &other1.dlnode);
    bs_dllist_sort(&list, _bs_dllist_test_sort_cmp);
    BS_TEST_VERIFY_EQ(test_ptr, &n1.dlnode, bs_dllist_pop_front(&list));
    BS_TEST_VERIFY_EQ(test_ptr, &other1.dlnode, bs_dllist_pop_front(&list));
}

/* ------------------------------------------------------------------------- */
/** Tests randomized sort. */
void bs_dllist_test_sort_random(bs_test_t *test_ptr)
{
    bs_dllist_t list = {};
    static const size_t n = 1024;
    struct _bs_dllist_test_sort_node v[n];

    srand(12345);
    for (size_t i = 0; i < n; ++i) {
        v[i] = (struct _bs_dllist_test_sort_node){ .i = rand(), .pos = i };
        bs_dllist_push_back(&list, &v[i].dlnode);
    }

    bs_dllist_sort(&list, _bs_dllist_test_sort_cmp);

    bs_dllist_node_t *n1 = bs_dllist_pop_front(&list);
    for (bs_dllist_node_t *n2 = bs_dllist_pop_front(&list);
         n2 != NULL;
         n1 = n2, n2 = bs_dllist_pop_front(&list)) {
        struct _bs_dllist_test_sort_node *v1 = BS_CONTAINER_OF(
            n1, struct _bs_dllist_test_sort_node, dlnode);
        struct _bs_dllist_test_sort_node *v2 = BS_CONTAINER_OF(
            n2, struct _bs_dllist_test_sort_node, dlnode);
        BS_TEST_VERIFY_TRUE(test_ptr, v1->i <= v2->i);
        if (v1->pos == v2->pos) BS_TEST_VERIFY_EQ(test_ptr, v1->pos, v2->pos);
    }
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
