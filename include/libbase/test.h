/* ========================================================================= */
/**
 * @file test.h
 * Declarations for building unit tests.
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
#ifndef __LIBBASE_TEST_H__
#define __LIBBASE_TEST_H__

#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "def.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Overall state of test. */
typedef struct _bs_test_t     bs_test_t;
/** State of unit test. */
typedef struct _bs_test_case_t bs_test_case_t;
/**  A test set. */
typedef struct _bs_test_set_t bs_test_set_t;

/** Test function. */
typedef void (*bs_test_fn_t)(bs_test_t *test_ptr);

/** Descriptor for a test case. */
struct _bs_test_case_t {
    /** Whether the test is enabled. */
    bool                       enabled;
    /** Name of the test, for informational purpose. */
    const char                *name_ptr;
    /** Test function for this testcase. */
    bs_test_fn_t              test_fn;
};

/** Sentinel, to add at the end of a list of @ref bs_test_case_t. */
#define BS_TEST_CASE_SENTINEL() { .enabled = false, .name_ptr = NULL }

/** Test set. */
struct _bs_test_set_t {
    /** Whether the set is enabled. */
    bool                       enabled;
    /** Name of the test set. */
    const char                *name_ptr;
    /** Array of test cases for that set. */
    const bs_test_case_t      *cases_ptr;

    /** Optional: Method to setup environment. Executed before each case. */
    void *(*setup)(void);
    /** Optional: Method to tear down environment. Executed after each case. */
    void (*teardown)(void *setup_context_ptr);
};

/** Defines a plain test set, no setup/teardown methods. */
#define BS_TEST_SET(_enabled ,_name, _cases) { \
        .enabled = _enabled,                   \
        .name_ptr = _name,                     \
        .cases_ptr = _cases                    \
    }

/** A test set with methods to setup & teardown context for each case. */
#define BS_TEST_SET_CONTEXT(_enabled ,_name, _cases, _setup, _teardown) { \
        .enabled = _enabled,                   \
        .name_ptr = _name,                     \
        .cases_ptr = _cases,                   \
        .setup = _setup,                       \
        .teardown = _teardown                  \
    }

/** Test parameters. */
typedef struct {
    /** Directory to data files. */
    const char                *test_data_dir_ptr;
} bs_test_param_t;

/**
 * Reports that the given test succeeded, and print the format string and
 * arguments to the test's report. Calling bs_test_succeed is optional, if
 * neither bs_test_fail or bs_test_succeed are called, the test counts as
 * succeeded. bs_test_fail takes precedence.
 *
 * @param test_ptr            Test state.
 * @param fmt_ptr             Format string for report message.
 * @param ...                 Additional arguments to format string.
 */
void bs_test_succeed(bs_test_t *test_ptr, const char *fmt_ptr, ...)
    __ARG_PRINTF__(2, 3);

/**
 * Reports the given test as failed for the given position.
 *
 * @param test_ptr            Test state.
 * @param fname_ptr           Filename to report the failure for.
 * @param line                line number.
 * @param fmt_ptr             Format string for report message.
 * @param ...                 Additional arguments to format string.
 */
void bs_test_fail_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    int line,
    const char *fmt_ptr, ...)
    __ARG_PRINTF__(4, 5);

/**
 * Check failure status of test.
 *
 * @return Whether the test has failed.
 */
bool bs_test_failed(bs_test_t *test_ptr);

/**
 * Returns the context returned by @ref bs_test_set_t::setup, or NULL.
 *
 * @param test_ptr
 *
 * @return The context, or NULL if @ref bs_test_set_t::setup was not provided.
 */
void *bs_test_context(bs_test_t *test_ptr);

/**
 * Runs test sets.
 *
 * @param test_sets
 * @param argc
 * @param argv
 * @param param_ptr           Optional, points to a @ref bs_test_param_t and
 *                            specifies parameters for the test environment.
 *
 * @return 0 on success or the number of failed test sets.
 */
int bs_test(
    const bs_test_set_t *test_sets,
    int argc,
    const char **argv,
    const bs_test_param_t *param_ptr);

/**
 * Runs test sets.
 *
 * @param test_set_ptrs       A NULL-terminated array of pointers to test sets.
 * @param argc
 * @param argv
 * @param param_ptr           Optional, points to a @ref bs_test_param_t and
 *                            specifies parameters for the test environment.
 *
 * @return 0 on success or the number of failed test sets.
 */
int bs_test_sets(
    const bs_test_set_t **test_set_ptrs,
    int argc,
    const char **argv,
    const bs_test_param_t *param_ptr);

/**
 * Tests whether the strings at `a_ptr` and `b_ptr` are equal.
 *
 * Helper method, you should use the BS_TEST_VERIFY_STREQ macro instead.
 *
 * @param test_ptr
 * @param fname_ptr
 * @param line
 * @param a_ptr
 * @param hash_a_ptr
 * @param b_ptr
 * @param hash_b_ptr
 */
void bs_test_verify_streq_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    const int line,
    const char *a_ptr,
    const char *hash_a_ptr,
    const char *b_ptr,
    const char *hash_b_ptr);

/**
 * Tests whether the string at `a_ptr` matches the regular expression.
 *
 * Helper method, you should use the BS_TEST_VERIFY_STRMATCH macro instead.
 *
 * @param fname_ptr
 * @param line
 * @param test_ptr
 * @param a_ptr
 * @param hash_a_ptr
 * @param regex_ptr
 */
void bs_test_verify_strmatch_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    const int line,
    const char *a_ptr,
    const char *hash_a_ptr,
    const char *regex_ptr);

/**
 * Tests whether the memory buffers at `a_ptr` and `b_ptr` are equal.
 *
 * Helper method, you should use the BS_TEST_VERIFY_MEMEQ macro instead.
 *
 * @param test_ptr
 * @param fname_ptr
 * @param line
 * @param a_ptr
 * @param hash_a_ptr
 * @param b_ptr
 * @param hash_b_ptr
 * @param size
 */
void bs_test_verify_memeq_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    const int line,
    const void *a_ptr,
    const char *hash_a_ptr,
    const void *b_ptr,
    const char *hash_b_ptr,
    const size_t size);

/**
 * Joins `fname_fmt_ptr` (relative to test data directory) into absolute path.
 *
 * @param test_ptr
 * @param fname_fmt_ptr       Format string, to permit constructing the path.
 *
 * @return A pointer to the resolved path or NULL on error. The memory will
 *     remain reserved throughout lifetime of `test_ptr`, and will be freed
 *     automatically.
 *     Upon error, `test_ptr` will be marked as failed.
 */
const char *bs_test_data_path(
    bs_test_t *test_ptr,
    const char *fname_fmt_ptr,
    ...) __ARG_PRINTF__(2, 3);

/**
 * Formats and joins a file path, relative to test case's temporary directory.
 *
 * Constructs a temporary directory with a lifetime bound to `test_ptr` The
 * directory and name is cached, ie. further calls to @ref bs_test_temp_path
 * within the same @ref bs_test_t will re-use the same directory.
 * When the test completes, it will attempt to rmdir() the directory. The test
 * will get marked as failed, if rmdir() fails, eg. if the directory is not
 * empty.
 *
 * @param test_ptr
 * @param fname_fmt_ptr       Format string of the file name to join, or NULL
 *                            to return just the test directory.
 *
 * @return The path name to the created directory, or NULL on error. If the
 *     function fails, `test_ptr` is marked as failed. The path name will be
 *     cleaned up during test teardown.
 */
const char *bs_test_temp_path(
    bs_test_t *test_ptr,
    const char *fname_fmt_ptr,
    ...) __ARG_PRINTF__(2, 3);

/**
 * Sets an environment variable in the process.
 *
 * This will first backup the current value of the environment variable, then
 * call getenv(3) to change it. Upon test teardown, the original value of the
 * environment variable is kept.
 * Repeated calls to @ref bs_test_setenv will not overwrite the backup stored
 * during the first call.
 *
 * @param test_ptr
 * @param name_ptr
 * @param value_fmt_ptr
 */
void bs_test_setenv(
    bs_test_t *test_ptr,
    const char *name_ptr,
    const char *value_fmt_ptr, ...) __ARG_PRINTF__(3, 4);

/* == Verification macros ================================================== */

/**
 * Reports the test as failed, at current position.
 *
 * @param _test
 */
#define BS_TEST_FAIL(_test, ...) {                                      \
        bs_test_fail_at((_test), __FILE__, __LINE__, __VA_ARGS__);      \
    }

/** Verifies that _expr is true, and returns (stops tests) if not. */
#define BS_TEST_VERIFY_TRUE_OR_RETURN(_test, _expr) { \
        BS_TEST_VERIFY_TRUE(_test, _expr);            \
        if (bs_test_failed(test_ptr)) return;         \
    }

/**
 * Verifies that _expr is true.
 *
 * @param _test
 * @param _expr
 */
#define BS_TEST_VERIFY_TRUE(_test, _expr) {                             \
        if (!(_expr)) {                                                 \
            bs_test_fail_at((_test), __FILE__, __LINE__,                \
                            "%s not true.", #_expr);                    \
        }                                                               \
    }

/** Verifies that _expr is false, and reutrns (stop tests) if not. */
#define BS_TEST_VERIFY_FALSE_OR_RETURN(_test, _expr) {  \
        BS_TEST_VERIFY_FALSE(_test, _expr);             \
        if (bs_test_failed(test_ptr)) return;           \
    }

/**
 * Verifies that _expr is false.
 *
 * @param _test
 * @param _expr
 */
#define BS_TEST_VERIFY_FALSE(_test, _expr) {                            \
        if (_expr) {                                                    \
            bs_test_fail_at((_test), __FILE__, __LINE__,                \
                            "%s not false.", #_expr);                   \
        }                                                               \
    }

/** Verifies that _a == _b, and reutrns (stop tests) if not. */
#define BS_TEST_VERIFY_EQ_OR_RETURN(_test, _a, _b) {    \
        BS_TEST_VERIFY_EQ(_test, _a, _b);               \
        if (bs_test_failed(test_ptr)) return;           \
    }

/**
 * Verifies that _a == _b
 *
 * @param _test
 * @param _a
 * @param _b
 */
#define BS_TEST_VERIFY_EQ(_test, _a, _b) {                              \
        if (!((_a) == (_b))) {                                          \
            bs_test_fail_at((_test), __FILE__, __LINE__,                \
                            "%s not equal %s.", #_a, #_b);              \
        }                                                               \
    }

/** Verifies that _a != _b, and reutrns (stop tests) if not. */
#define BS_TEST_VERIFY_NEQ_OR_RETURN(_test, _a, _b) { \
        BS_TEST_VERIFY_NEQ(_test, _a, _b);            \
        if (bs_test_failed(_test)) return;            \
    }

/**
 * Verifies that _a != _b
 *
 * @param _test
 * @param _a
 * @param _b
 */
#define BS_TEST_VERIFY_NEQ(_test, _a, _b) {                             \
        if ((_a) == (_b)) {                                             \
            bs_test_fail_at((_test), __FILE__, __LINE__,                \
                            "%s equal %s.", #_a, #_b);                  \
        }                                                               \
    }

/** Verifies that the strings _a == _b. Returns (stop test) if not. */
#define BS_TEST_VERIFY_STREQ_OR_RETURN(_test, _a, _b) { \
        BS_TEST_VERIFY_STREQ(_test, _a, _b);            \
        if (bs_test_failed(test_ptr)) return;           \
    }

/**
 * Verifies that the strings _a == _b.
 *
 * @param _test
 * @param _a
 * @param _b
 */
#define BS_TEST_VERIFY_STREQ(_test, _a, _b) {                           \
        bs_test_verify_streq_at(                                        \
            (_test), __FILE__, __LINE__, _a, #_a, _b, #_b);             \
    }

/** Verifies that the string _a matches _regex.. Returns (stop test) if not. */
#define BS_TEST_VERIFY_STRMATCH_OR_RETURN(_test, _a, _regex) { \
        BS_TEST_VERIFY_STRMATCH(_test, _a, _regex);            \
        if (bs_test_failed(test_ptr)) return;                  \
    }

/**
 * Verifies that the string _a matches the regular expression _regex.
 *
 * @param _test
 * @param _a
 * @param _regex
 */
#define BS_TEST_VERIFY_STRMATCH(_test, _a, _regex) {                    \
        bs_test_verify_strmatch_at(                                     \
            (_test), __FILE__, __LINE__, _a, #_a, _regex);              \
    }

/** Verifies that the memory buffers _a == _b. Returns (stop tests) if not. */
#define BS_TEST_VERIFY_MEMEQ_OR_RETURN(_test, _a, _b, _size) {  \
        BS_TEST_VERIFY_MEMEQ(_test, _a, _b, _size);             \
        if (bs_test_failed(test_ptr)) return;                   \
    }

/**
 * Verifies that the memory buffers _a == _b.
 *
 * @param _test
 * @param _a
 * @param _b
 * @param _size
 */
#define BS_TEST_VERIFY_MEMEQ(_test, _a, _b, _size) {                    \
        bs_test_verify_memeq_at(                                        \
            (_test), __FILE__, __LINE__, _a, #_a, _b, #_b, _size);      \
    }

/** Unit test set. */
extern const bs_test_set_t   bs_test_test_set;
/** Test set with setup and teardown of a context. */
extern const bs_test_set_t bs_test_test_setup_teardown_set;

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_TEST_H__ */
/* == End of test.h ===================================================== */
