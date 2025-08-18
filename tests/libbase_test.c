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
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(BS_TEST_DATA_DIR)
/** Directory root for looking up test data. See @ref bs_test_resolve_path. */
#define BS_TEST_DATA_DIR "./"
#endif  // BS_TEST_DATA_DIR

#if !defined(BS_TEST_BINARY_DIR)
#define BS_TEST_BINARY_DIR "."
#endif  // BS_TEST_BINARY_DIR

static void test_assert(bs_test_t *test_ptr);
static void test_def(bs_test_t *test_ptr);

/** Further tests of definitions, without .c file. */
static const bs_test_case_t bs_header_only_test_cases[] = {
    { 1, "assert", test_assert },
    { 1, "def", test_def },
    { 0, NULL, NULL }
};

static void test_failure(bs_test_t *test_ptr);
static void test_sigpipe(bs_test_t *test_ptr);
static void test_success(bs_test_t *test_ptr);
static void test_success_cmdline(bs_test_t *test_ptr);
static void test_success_twice(bs_test_t *test_ptr);

static const char             *test_args[] = { "binary", "alpha", NULL };
const bs_test_case_t          bs_subprocess_extra_test_cases[] = {
    { 1, "failure", test_failure },
    { 1, "sigpipe", test_sigpipe },
    { 1, "success", test_success },
    { 1, "success_cmdline", test_success_cmdline },
    { 1, "success_twice", test_success_twice },
    { 0, NULL, NULL }
};

/** Unit tests. */
const bs_test_set_t           libbase_tests[] = {
    { 1, "atomic", bs_atomic_test_cases },
    { 1, "arg", bs_arg_test_cases },
    { 1, "avltree", bs_avltree_test_cases },
    { 1, "dequeue", bs_dequeue_test_cases },
    { 1, "dllist", bs_dllist_test_cases },
    { 1, "dynbuf", bs_dynbuf_test_cases },
    { 1, "file", bs_file_test_cases },
    { 1, "gfxbuf", bs_gfxbuf_test_cases },
    { 1, "gfxbuf_xpm", bs_gfxbuf_xpm_test_cases },
    { 1, "header_only", bs_header_only_test_cases },
    { 1, "log", bs_log_test_cases },
    { 1, "ptr_set", bs_ptr_set_test_cases },
    { 1, "ptr_stack", bs_ptr_stack_test_cases },
    { 1, "ptr_vector", bs_ptr_vector_test_cases },
    { 1, "subprocess", bs_subprocess_test_cases },
    { 1, "subprocess_extra", bs_subprocess_extra_test_cases },
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

/* ------------------------------------------------------------------------- */
 void test_failure(bs_test_t *test_ptr)
{
    test_args[0] = BS_TEST_BINARY_DIR "/subprocess_test_failure";
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        test_args[0], test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, 42, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_sigpipe(bs_test_t *test_ptr)
{
    test_args[0] = BS_TEST_BINARY_DIR "/subprocess_test_sigpipe";
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        test_args[0], test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, INT_MIN, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, SIGPIPE, signal_number);
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_success(bs_test_t *test_ptr)
{
    const bs_subprocess_environment_variable_t envs[] = {
        { "SUBPROCESS_ENV", "WORKS" },
        { NULL, NULL }
    };
    test_args[0] = BS_TEST_BINARY_DIR "/subprocess_test_success";
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        test_args[0], test_args, envs);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, 0, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);

    int stdout_fd, stderr_fd;
    bs_subprocess_get_fds(sp_ptr, NULL, &stdout_fd, &stderr_fd);
    bs_dynbuf_t stdout_buf;
    bs_dynbuf_init(&stdout_buf, 1024, SIZE_MAX);
    bs_dynbuf_read(&stdout_buf, stdout_fd);
    bs_dynbuf_t stderr_buf;
    bs_dynbuf_init(&stderr_buf, 1024, SIZE_MAX);
    bs_dynbuf_read(&stderr_buf, stderr_fd);

    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "test stdout: subprocess_test_success\nenv: WORKS\n",
        stdout_buf.data_ptr);
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "test stderr: alpha\n",
        stderr_buf.data_ptr);
    bs_subprocess_destroy(sp_ptr);
    bs_dynbuf_fini(&stderr_buf);
    bs_dynbuf_fini(&stdout_buf);
}

/* ------------------------------------------------------------------------- */
void test_success_cmdline(bs_test_t *test_ptr)
{
    bs_subprocess_t *sp_ptr = bs_subprocess_create_cmdline(
        "SUBPROCESS_ENV=CMD " BS_TEST_BINARY_DIR "/subprocess_test_success alpha");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, 0, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);

    int stdout_fd, stderr_fd;
    bs_subprocess_get_fds(sp_ptr, NULL, &stdout_fd, &stderr_fd);
    bs_dynbuf_t stdout_buf;
    bs_dynbuf_init(&stdout_buf, 1024, SIZE_MAX);
    bs_dynbuf_read(&stdout_buf, stdout_fd);
    bs_dynbuf_t stderr_buf;
    bs_dynbuf_init(&stderr_buf, 1024, SIZE_MAX);
    bs_dynbuf_read(&stderr_buf, stderr_fd);

    BS_TEST_VERIFY_STREQ(
        test_ptr, "test stdout: subprocess_test_success\nenv: CMD\n",
        stdout_buf.data_ptr);
    BS_TEST_VERIFY_STREQ(
        test_ptr, "test stderr: alpha\n",
        stderr_buf.data_ptr);
    bs_subprocess_destroy(sp_ptr);
    bs_dynbuf_fini(&stderr_buf);
    bs_dynbuf_fini(&stdout_buf);
}

/* ------------------------------------------------------------------------- */
void test_success_twice(bs_test_t *test_ptr)
{
    test_args[0] = BS_TEST_BINARY_DIR "/subprocess_test_success";
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        test_args[0], test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);

    for (int i = 0; i < 2; ++i) {
        // Verify that starting the process again works.
        BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

        int exit_status, signal_number;
        while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
            // Don't busy-loop -- just wait a little.
            poll(NULL, 0, 10);
        }
        BS_TEST_VERIFY_EQ(test_ptr, 0, exit_status);
        BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);

        int stdout_fd, stderr_fd;
        bs_subprocess_get_fds(sp_ptr, NULL, &stdout_fd, &stderr_fd);
        bs_dynbuf_t stdout_buf;
        bs_dynbuf_init(&stdout_buf, 1024, SIZE_MAX);
        bs_dynbuf_read(&stdout_buf, stdout_fd);
        bs_dynbuf_t stderr_buf;
        bs_dynbuf_init(&stderr_buf, 1024, SIZE_MAX);
        bs_dynbuf_read(&stderr_buf, stderr_fd);

        BS_TEST_VERIFY_STREQ(
            test_ptr,
            "test stdout: subprocess_test_success\nenv: (null)\n",
            stdout_buf.data_ptr);
        BS_TEST_VERIFY_STREQ(
            test_ptr,
            "test stderr: alpha\n",
            stderr_buf.data_ptr);

        bs_dynbuf_fini(&stderr_buf);
        bs_dynbuf_fini(&stdout_buf);
    }
    bs_subprocess_destroy(sp_ptr);
}

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
