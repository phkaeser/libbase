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

/** Unit tests. */
const bs_test_set_t           libbase_tests[] = {
    { 1, "bs_atomic", bs_atomic_test_cases },
    { 1, "bs_arg", bs_arg_test_cases },
    { 1, "bs_avltree", bs_avltree_test_cases },
    { 1, "bs_dequeue", bs_dequeue_test_cases },
    { 1, "bs_dllist", bs_dllist_test_cases },
    { 1, "bs_log", bs_log_test_cases },
    { 1, "bs_ptr_set", bs_ptr_set_test_cases },
    { 1, "bs_ptr_stack", bs_ptr_stack_test_cases },
    { 1, "bs_subprocess", bs_subprocess_test_cases },
    { 1, "bs_strutil", bs_strutil_test_cases },
    { 1, "bs_test", bs_test_test_cases },
    { 1, "bs_time", bs_time_test_cases },
    { 0, NULL, NULL }
};

/** Main program, runs all unit tests. */
int main(int argc, const char **argv)
{
    return bs_test(libbase_tests, argc, argv);
}

/* == End of libbase_test.c ================================================== */
