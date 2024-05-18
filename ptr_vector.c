/* ========================================================================= */
/**
 * @file ptr_vector.h
 *
 * Interface for a simple vector to store pointers.
 *
 * @copyright
 * Copyright 2024 Google LLC
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

#include "ptr_vector.h"

#include "assert.h"
#include "log.h"

/* == Declarations ========================================================= */

/** Initial vector size, 1024 elements. */
#define INITIAL_SIZE 1024

static bool _bs_ptr_vector_grow(bs_ptr_vector_t *ptr_vector_ptr);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool bs_ptr_vector_init(bs_ptr_vector_t *ptr_vector_ptr)
{
    ptr_vector_ptr->capacity = 0;
    ptr_vector_ptr->consumed = 0;
    ptr_vector_ptr->elements_ptr = NULL;
    _bs_ptr_vector_grow(ptr_vector_ptr);
    return true;
}

/* ------------------------------------------------------------------------- */
void bs_ptr_vector_fini(bs_ptr_vector_t *ptr_vector_ptr)
{
    if (NULL != ptr_vector_ptr->elements_ptr) {
        if (0 < ptr_vector_ptr->consumed) {
            bs_log(BS_WARNING, "Un-initializing non-empty vector at %p",
                   ptr_vector_ptr);
        }
        free(ptr_vector_ptr->elements_ptr);
        ptr_vector_ptr->elements_ptr = NULL;
    }
    ptr_vector_ptr->capacity = 0;
    ptr_vector_ptr->consumed = 0;
}

/* ------------------------------------------------------------------------- */
size_t bs_ptr_vector_size(bs_ptr_vector_t *ptr_vector_ptr)
{
    return ptr_vector_ptr->consumed;
}

/* ------------------------------------------------------------------------- */
bool bs_ptr_vector_push_back(bs_ptr_vector_t *ptr_vector_ptr,
                             void *data_ptr)
{
    while (ptr_vector_ptr->consumed >= ptr_vector_ptr->capacity) {
        if (!_bs_ptr_vector_grow(ptr_vector_ptr)) return false;
    }
    ptr_vector_ptr->elements_ptr[ptr_vector_ptr->consumed++] = data_ptr;
    return true;
}

/* ------------------------------------------------------------------------- */
bool bs_ptr_vector_erase(bs_ptr_vector_t *ptr_vector_ptr,
                         size_t pos)
{
    if (pos >= ptr_vector_ptr->consumed) return false;

    if (pos + 1 < ptr_vector_ptr->consumed) {
        memmove(ptr_vector_ptr->elements_ptr + pos,
                ptr_vector_ptr->elements_ptr + pos + 1,
                (ptr_vector_ptr->consumed - pos) * sizeof(void*));
    }
    --ptr_vector_ptr->consumed;
    return true;
}

/* ------------------------------------------------------------------------- */
void* bs_ptr_vector_at(bs_ptr_vector_t *ptr_vector_ptr, size_t pos)
{
    BS_ASSERT(pos < ptr_vector_ptr->consumed);
    return ptr_vector_ptr->elements_ptr[pos];
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/**
 * Grows the vector, by INITIAL_SIZE elements.
 *
 * @param ptr_vector_ptr
 *
 * @return true on success.
 */
bool _bs_ptr_vector_grow(bs_ptr_vector_t *ptr_vector_ptr)
{
    size_t new_size = ptr_vector_ptr->capacity + INITIAL_SIZE;
    void *new_elements_ptr = realloc(
        ptr_vector_ptr->elements_ptr, new_size * sizeof(void*));
    if (NULL == new_elements_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed realloc(%p, %zu)",
               ptr_vector_ptr->elements_ptr, new_size * sizeof(void*));
        return false;
    }
    ptr_vector_ptr->capacity = new_size;
    ptr_vector_ptr->elements_ptr = new_elements_ptr;
    return true;
}

/* == Unit tests =========================================================== */

static void basic_test(bs_test_t *test_ptr);

const bs_test_case_t bs_ptr_vector_test_cases[] = {
    { 1, "basic", basic_test },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
/** Tests basic functionality. */
void basic_test(bs_test_t *test_ptr)
{
    bs_ptr_vector_t ptr_vector;
    static char e = 'e';

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_vector_init(&ptr_vector));
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_ptr_vector_size(&ptr_vector));

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_vector_push_back(&ptr_vector, &e));
    BS_TEST_VERIFY_EQ(test_ptr, 1, bs_ptr_vector_size(&ptr_vector));
    BS_TEST_VERIFY_EQ(test_ptr, &e, bs_ptr_vector_at(&ptr_vector, 0));

    BS_TEST_VERIFY_TRUE(test_ptr, bs_ptr_vector_erase(&ptr_vector, 0));
    BS_TEST_VERIFY_EQ(test_ptr, 0, bs_ptr_vector_size(&ptr_vector));

    bs_ptr_vector_fini(&ptr_vector);
}

/* == End of ptr_vector.c ================================================== */
