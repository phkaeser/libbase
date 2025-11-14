/* ========================================================================= */
/**
 * @file dequeue.c
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

#include <libbase/dequeue.h>
#include <libbase/test.h>
#include <stdbool.h>
#include <stddef.h>

/* == Tests =============================================================== */

static void bs_dequeue_front_test(bs_test_t *test_ptr);
static void bs_dequeue_back_test(bs_test_t *test_ptr);

/** Unit test cases. */
static const bs_test_case_t   bs_dequeue_test_cases[] = {
    { true, "push front & pop", bs_dequeue_front_test },
    { true, "push back & pop", bs_dequeue_back_test },
    { false, NULL, NULL }  // sentinel.
};

const bs_test_set_t           bs_dequeue_test_set = BS_TEST_SET(
    true, "dequeue", bs_dequeue_test_cases);

/* ------------------------------------------------------------------------- */
/**
 * Tests that push front & pop works.
 */
void bs_dequeue_front_test(bs_test_t *test_ptr)
{
    bs_dequeue_t              queue = {};
    bs_dequeue_node_t         node1, node2;

    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), NULL);

    bs_dequeue_push_front(&queue, &node1);
    BS_TEST_VERIFY_EQ(test_ptr, queue.head_ptr, &node1);
    BS_TEST_VERIFY_EQ(test_ptr, node1.next_ptr, NULL);

    bs_dequeue_push_front(&queue, &node2);
    BS_TEST_VERIFY_EQ(test_ptr, queue.head_ptr, &node2);
    BS_TEST_VERIFY_EQ(test_ptr, node2.next_ptr, &node1);
    BS_TEST_VERIFY_EQ(test_ptr, node1.next_ptr, NULL);

    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), &node2);
    BS_TEST_VERIFY_EQ(test_ptr, queue.head_ptr, &node1);
    BS_TEST_VERIFY_EQ(test_ptr, node1.next_ptr, NULL);

    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), &node1);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), NULL);
}

/* ------------------------------------------------------------------------- */
/**
 * Tests that push back & pop works.
 */
void bs_dequeue_back_test(bs_test_t *test_ptr)
{
    bs_dequeue_t              queue = {};
    bs_dequeue_node_t         node1, node2, node3;

    bs_dequeue_push_back(&queue, &node1);
    bs_dequeue_push_back(&queue, &node2);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), &node1);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), &node2);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), NULL);

    // Build a node2 -> node1 -> node3 queue.
    bs_dequeue_push_back(&queue, &node1);
    bs_dequeue_push_front(&queue, &node2);
    bs_dequeue_push_back(&queue, &node3);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), &node2);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), &node1);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), &node3);
    BS_TEST_VERIFY_EQ(test_ptr, bs_dequeue_pop(&queue), NULL);
}

/* == End of dequeue.c ===================================================== */
