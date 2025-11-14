/* ========================================================================= */
/**
 * @file atomic.c
 *
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

#include <libbase/atomic.h>
#include <libbase/test.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(__cplusplus) || defined(__clang__)

const bs_test_set_t bs_atomic_test_set = BS_TEST_SET(false, NULL, NULL);

#else  // defined(__cplusplus) || !defined(__clang__)

/* == Atomic tests ========================================================= */

static void bs_atomic_test_int32(bs_test_t *test_ptr);
static void bs_atomic_test_int64(bs_test_t *test_ptr);

/** Unit test cases. */
static const bs_test_case_t   bs_atomic_test_cases[] = {
    { true, "int32 unit tests", bs_atomic_test_int32 },
    { true, "int64 unit tests", bs_atomic_test_int64 },
    { false, NULL, NULL }
};

const bs_test_set_t bs_atomic_test_set = BS_TEST_SET(
    true, "atomic", bs_atomic_test_cases);

/* ------------------------------------------------------------------------- */
void bs_atomic_test_int32(bs_test_t *test_ptr)
{
    bs_atomic_int32_t         a = BS_ATOMIC_INT32_INIT(42);
    int32_t                   b;

    BS_TEST_VERIFY_EQ(test_ptr, 42, bs_atomic_int32_get(&a));
    bs_atomic_int32_set(&a, 27972);
    BS_TEST_VERIFY_EQ(test_ptr, 27972, bs_atomic_int32_get(&a));
    BS_TEST_VERIFY_EQ(test_ptr, 27900, bs_atomic_int32_add(&a, -72));
    BS_TEST_VERIFY_EQ(test_ptr, 27900, bs_atomic_int32_get(&a));
    b = 1234;
    bs_atomic_int32_xchg(&a, &b);
    BS_TEST_VERIFY_EQ(test_ptr, 1234, bs_atomic_int32_get(&a));
    BS_TEST_VERIFY_EQ(test_ptr, 27900, b);

    // CAS, but with non-matching old_val. Must not swap.
    BS_TEST_VERIFY_EQ(test_ptr, 1234, bs_atomic_int32_cas(&a, 4321, 2222));
    BS_TEST_VERIFY_EQ(test_ptr, 1234, bs_atomic_int32_get(&a));

    // CAS, with matching old_val. Must swap.
    BS_TEST_VERIFY_EQ(test_ptr, 1234, bs_atomic_int32_cas(&a, 4321, 1234));
    BS_TEST_VERIFY_EQ(test_ptr, 4321, bs_atomic_int32_get(&a));
}

/* ------------------------------------------------------------------------- */
void bs_atomic_test_int64(bs_test_t *test_ptr)
{
    bs_atomic_int64_t         a = BS_ATOMIC_INT64_INIT(0x0102030405060708);
    int64_t                   b;

    BS_TEST_VERIFY_EQ(test_ptr, 0x0102030405060708, bs_atomic_int64_get(&a));
    bs_atomic_int64_set(&a, 0x0807060504030201);
    BS_TEST_VERIFY_EQ(test_ptr, 0x0807060504030201, bs_atomic_int64_get(&a));
    BS_TEST_VERIFY_EQ(test_ptr, 0x1827364554637281,
                      bs_atomic_int64_add(&a, 0x1020304050607080));
    BS_TEST_VERIFY_EQ(test_ptr, 0x1827364554637281, bs_atomic_int64_get(&a));
    b = 1234;
    bs_atomic_int64_xchg(&a, &b);
    BS_TEST_VERIFY_EQ(test_ptr, 1234, bs_atomic_int64_get(&a));
    BS_TEST_VERIFY_EQ(test_ptr, 0x1827364554637281, b);

    // CAS, but with non-matching old_val. Must not swap.
    bs_atomic_int64_set(&a, 0x0807060504030201);
    BS_TEST_VERIFY_EQ(test_ptr, 0x0807060504030201,
                      bs_atomic_int64_cas(&a, 0x1122334455667788, 2222));
    BS_TEST_VERIFY_EQ(test_ptr, 0x0807060504030201, bs_atomic_int64_get(&a));

    // CAS, with matching old_val. Must swap.
    BS_TEST_VERIFY_EQ(test_ptr, 0x0807060504030201,
                      bs_atomic_int64_cas(&a, 0x1122334455667788,
                                          0x0807060504030201));
    BS_TEST_VERIFY_EQ(test_ptr, 0x1122334455667788, bs_atomic_int64_get(&a));
}
#endif

/* == End of atomic.c ===================================================== */
