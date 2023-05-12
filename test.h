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
    /** 0 if the test is disabled */
    int                       enabled;
    /** Name of the test, for informational purpose. */
    const char                *name_ptr;
    /** Test function for this testcase. */
    bs_test_fn_t              test_fn;
};

/** Test set. */
struct _bs_test_set_t {
    /** 0 if the set is disabled. */
    int                       enabled;
    /** Name of the test set. */
    const char                *name_ptr;
    /** Array of test cases for that set. */
    const bs_test_case_t      *case_ptr;
};

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
 * Runs test sets.
 *
 * @return 0 on success or the number of failed test sets.
 */
int bs_test(const bs_test_set_t *test_sets,
            int argc, const char **argv);

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
 * Resolves a relative path into an absolute, with configured test directory.
 *
 * If fname_ptr is already an absolute path, this will return the resolved path
 * using realpath(3). Otherwise, it is joined with the configured test
 * directory, and resolved using realpath(3).
 *
 * @param fname_ptr
 *
 * @return A pointer to the resolved path or NULL on error. It points to a
 *     thread-local static store and does not need to be free()-ed. It may get
 *     overwritten by the next call to @ref bs_test_resolve_path.
 */
const char *bs_test_resolve_path(const char *fname_ptr);

/* == Verification macros ================================================== */

/**
 * Reports the test as failed, at current position .
 *
 * @param _test
 */
#define BS_TEST_FAIL(_test, ...) {                                      \
        bs_test_fail_at((_test), __FILE__, __LINE__, __VA_ARGS__);      \
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

/**
 * Verifies that the strings _a != _b.
 *
 * @param _test
 * @param _a
 * @param _b
 */
#define BS_TEST_VERIFY_STREQ(_test, _a, _b) {                           \
        bs_test_verify_streq_at(                                        \
            (_test), __FILE__, __LINE__, _a, #_a, _b, #_b);             \
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

/** Test cases. */
extern const bs_test_case_t   bs_test_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_TEST_H__ */
/* == End of test.h ===================================================== */
