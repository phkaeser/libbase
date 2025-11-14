/* ========================================================================= */
/**
 * @file ptr_stack.c
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
#include <libbase/log.h>
#include <libbase/log_wrappers.h>
#include <libbase/ptr_stack.h>
#include <libbase/test.h>
#include <stdbool.h>
#include <stdlib.h>

/* == Declarations ========================================================= */

/** Initial stack size, 1024 elements. */
#define INITIAL_SIZE 1024

static bool grow_stack(bs_ptr_stack_t *ptr_stack_ptr);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bs_ptr_stack_t *bs_ptr_stack_create(void)
{
    bs_ptr_stack_t *ptr_stack_ptr = logged_calloc(1, sizeof(bs_ptr_stack_t));
    if (NULL == ptr_stack_ptr) return NULL;

    if (!bs_ptr_stack_init(ptr_stack_ptr)) {
        bs_ptr_stack_destroy(ptr_stack_ptr);
        return NULL;
    }
    return ptr_stack_ptr;
}

/* ------------------------------------------------------------------------- */
void bs_ptr_stack_destroy(bs_ptr_stack_t *ptr_stack_ptr)
{
    bs_ptr_stack_fini(ptr_stack_ptr);
    free(ptr_stack_ptr);
}

/* ------------------------------------------------------------------------- */
bool bs_ptr_stack_init(bs_ptr_stack_t *ptr_stack_ptr)
{
    ptr_stack_ptr->pos = 0;
    ptr_stack_ptr->size = 0;
    ptr_stack_ptr->data_ptr = NULL;
    return grow_stack(ptr_stack_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_ptr_stack_fini(bs_ptr_stack_t *ptr_stack_ptr)
{
    if (NULL != ptr_stack_ptr->data_ptr) {
        if (0 < ptr_stack_ptr->pos) {
            bs_log(BS_WARNING, "Destroying non-empty ptr_stack at %p",
                   ptr_stack_ptr);
        }
        free(ptr_stack_ptr->data_ptr);
        ptr_stack_ptr->data_ptr = NULL;
    }

    ptr_stack_ptr->size = 0;
    ptr_stack_ptr->pos = 0;
}

/* ------------------------------------------------------------------------- */
bool bs_ptr_stack_push(bs_ptr_stack_t *ptr_stack_ptr, void *elem_ptr)
{
    BS_ASSERT(NULL != elem_ptr);
    if (ptr_stack_ptr->pos + 1 >= ptr_stack_ptr->size) {
        if (!grow_stack(ptr_stack_ptr)) return false;
    }

    ptr_stack_ptr->data_ptr[ptr_stack_ptr->pos++] = elem_ptr;
    return true;
}

/* ------------------------------------------------------------------------- */
void *bs_ptr_stack_pop(bs_ptr_stack_t *ptr_stack_ptr)
{
    if (0 >= ptr_stack_ptr->pos) return NULL;
    return ptr_stack_ptr->data_ptr[--ptr_stack_ptr->pos];
}

/* ------------------------------------------------------------------------- */
void *bs_ptr_stack_peek(bs_ptr_stack_t *ptr_stack_ptr,
                        size_t index)
{
    if (index >= ptr_stack_ptr->pos) return NULL;
    return ptr_stack_ptr->data_ptr[ptr_stack_ptr->pos - index - 1];
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/** Grows the stack by INITIAL_SIZE elements. */
bool grow_stack(bs_ptr_stack_t *ptr_stack_ptr)
{
    size_t new_size = ptr_stack_ptr->size + INITIAL_SIZE;
    void *new_data_ptr = realloc(
        ptr_stack_ptr->data_ptr, new_size * sizeof(void*));
    if (NULL == new_data_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed realloc(%p, %zu)",
               ptr_stack_ptr->data_ptr, new_size * sizeof(void*));
        return false;
    }
    ptr_stack_ptr->size = new_size;
    ptr_stack_ptr->data_ptr = new_data_ptr;
    return true;
}

/* == Unit tests =========================================================== */

static void basic_test(bs_test_t *test_ptr);
static void large_test(bs_test_t *test_ptr);

/** Unit test cases. */
static const bs_test_case_t   bs_ptr_stack_test_cases[] = {
    { true, "basic", basic_test },
    { true, "large", large_test },
    BS_TEST_CASE_SENTINEL()
};

const bs_test_set_t bs_ptr_stack_test_set = BS_TEST_SET(
    true, "ptr_stack", bs_ptr_stack_test_cases);

/* ------------------------------------------------------------------------- */
/** Basic functionality: init, empty pop, push, pop, empty pop, fini. */
void basic_test(bs_test_t *test_ptr)
{
    bs_ptr_stack_t ptr_stack;
    static void *elem_ptr = (void*)basic_test;  // Just anything, actually.

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_stack_init(&ptr_stack));

    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_ptr_stack_pop(&ptr_stack));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_stack_push(&ptr_stack, elem_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, elem_ptr, bs_ptr_stack_peek(&ptr_stack, 0));
    BS_TEST_VERIFY_EQ(test_ptr, elem_ptr, bs_ptr_stack_pop(&ptr_stack));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_ptr_stack_pop(&ptr_stack));

    bs_ptr_stack_fini(&ptr_stack);
}

/* ------------------------------------------------------------------------- */
/** Basic functionality: init, empty pop, push, pop, empty pop, fini. */
void large_test(bs_test_t *test_ptr)
{
    bs_ptr_stack_t ptr_stack;
    char elem[2 * INITIAL_SIZE];

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_stack_init(&ptr_stack));

    for (size_t i = 0; i < 2 * INITIAL_SIZE; ++i) {
        BS_TEST_VERIFY_TRUE(test_ptr,
                            bs_ptr_stack_push(&ptr_stack, &elem[i]));
    }

    for (size_t i = 0; i < 2 * INITIAL_SIZE; ++i) {
        BS_TEST_VERIFY_EQ(
            test_ptr,
            &elem[2 * INITIAL_SIZE - 1 - i],
            bs_ptr_stack_peek(&ptr_stack, i));
    }
    BS_TEST_VERIFY_EQ(
        test_ptr, NULL,
        bs_ptr_stack_peek(&ptr_stack, 2 * INITIAL_SIZE));
    BS_TEST_VERIFY_EQ(
        test_ptr, NULL,
        bs_ptr_stack_peek(&ptr_stack, 2 * INITIAL_SIZE + 1));

    for (size_t i = 0; i < 2 * INITIAL_SIZE; ++i) {
        void *ptr = bs_ptr_stack_pop(&ptr_stack);

        if (&elem[2 * INITIAL_SIZE - 1 - i] != ptr) {
            bs_log(BS_ERROR, "%zu: (expected) %p != %p (stacked)", i,
                   &elem[2 * INITIAL_SIZE - 1 - i], ptr);
        }

        BS_TEST_VERIFY_EQ(
            test_ptr,
            &elem[2 * INITIAL_SIZE - 1 - i],
            ptr); //            bs_ptr_stack_pop(&ptr_stack));
    }

    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_ptr_stack_pop(&ptr_stack));

    bs_ptr_stack_fini(&ptr_stack);
}

/* == End of ptr_stack.c =================================================== */
