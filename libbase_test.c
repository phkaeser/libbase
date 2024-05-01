/* ========================================================================= */
/**
 * @file libbase_test.c
 * Unit tests for libbase.
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

#include <libbase/libbase.h>

static void test_assert(bs_test_t *test_ptr);
static void test_def(bs_test_t *test_ptr);

/** Further tests of definitions, without .c file. */
static const bs_test_case_t bs_header_only_test_cases[] = {
    { 1, "assert", test_assert },
    { 1, "def", test_def },
    { 0, NULL, NULL }
};

/** Unit tests. */
const bs_test_set_t           libbase_tests[] = {
    { 1, "atomic", bs_atomic_test_cases },
    { 1, "arg", bs_arg_test_cases },
    { 1, "avltree", bs_avltree_test_cases },
    { 1, "dequeue", bs_dequeue_test_cases },
    { 1, "dllist", bs_dllist_test_cases },
    { 1, "file", bs_file_test_cases },
    { 1, "gfxbuf", bs_gfxbuf_test_cases },
    { 1, "gfxbuf_xpm", bs_gfxbuf_xpm_test_cases },
    { 1, "header_only", bs_header_only_test_cases },
    { 1, "log", bs_log_test_cases },
    { 1, "ptr_set", bs_ptr_set_test_cases },
    { 1, "ptr_stack", bs_ptr_stack_test_cases },
    { 1, "subprocess", bs_subprocess_test_cases },
    { 1, "strutil", bs_strutil_test_cases },
    { 1, "test", bs_test_test_cases },
    { 1, "time", bs_time_test_cases },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
/** Tests the functions of 'assert.h' */
void test_assert(bs_test_t *test_ptr)
{
    void *ptr = test_assert;
    BS_TEST_VERIFY_EQ(test_ptr, ptr, BS_ASSERT_NOTNULL(ptr));
}

/* ------------------------------------------------------------------------- */
/** Tests the functions of 'def.h' */
void test_def(bs_test_t *test_ptr)
{
    BS_TEST_VERIFY_EQ(test_ptr, 1, BS_MIN(1, 2));
    BS_TEST_VERIFY_EQ(test_ptr, 2, BS_MAX(1, 2));
}

#if !defined(BS_TEST_DATA_DIR)
/** Directory root for looking up test data. See @ref bs_test_resolve_path. */
#define BS_TEST_DATA_DIR "./"
#endif  // BS_TEST_DATA_DIR

/* ========================================================================= */
/** Main program, runs all unit tests. */
int main(int argc, const char **argv)
{
    const bs_test_param_t params = {
        .test_data_dir_ptr   = BS_TEST_DATA_DIR
    };

    return bs_test(libbase_tests, argc, argv, &params);
}

/* == End of libbase_test.c ================================================ */
