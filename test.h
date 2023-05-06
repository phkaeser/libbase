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
 * Reports the given test as failed.
 *
 * @param test_ptr            Test state.
 * @param fmt_ptr             Format string for report message.
 * @param ...                 Additional arguments to format string.
 */
void bs_test_fail(bs_test_t *test_ptr, const char *fmt_ptr, ...)
    __ARG_PRINTF__(2, 3);

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

/* == Verification macros ================================================== */

/**
 * Verifies that _expr is true.
 *
 * @param _test
 * @param _expr
 */
#define BS_TEST_VERIFY_TRUE(_test, _expr) {                             \
        if (!(_expr)) {                                                 \
            bs_test_fail((_test), "%s(%d): %s not true.",               \
                         __FILE__, __LINE__, #_expr);                   \
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
            bs_test_fail((_test), "%s(%d): %s not false.",              \
                         __FILE__, __LINE__, #_expr);                   \
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
            bs_test_fail((_test), "%s(%d): %s not equal %s.",           \
                         __FILE__, __LINE__, #_a, #_b);                 \
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
            bs_test_fail((_test), "%s(%d): %s equal %s.",               \
                         __FILE__, __LINE__, #_a, #_b);                 \
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
        char _a1[4097], _b1[4097];                                      \
        snprintf(_a1, sizeof(_a1), "%s", (_a));                         \
        snprintf(_b1, sizeof(_b1), "%s", (_b));                         \
        if (0 != strcmp(_a1, _b1)) {                                    \
            size_t pos = 0;                                             \
            while (_a1[pos] && _b1[pos] && _a1[pos] == _b1[pos]) {      \
                ++pos;                                                  \
            }                                                           \
            bs_test_fail((_test),                                       \
                         "%s(%d): %s (\"%s\") not equal %s (\"%s\") at" \
                         " %zu (0x%02x != 0x%02x)", __FILE__, __LINE__, \
                         #_a, _a1, #_b, _b1, pos, _a1[pos], _b1[pos]);  \
        }                                                               \
    }

/**
 * Verifies that the string _a matches the regular expression _regex.
 *
 * @param _test
 * @param _a
 * @param _regex
 */
#define BS_TEST_VERIFY_STRMATCH(_test, _a, _regex) {                    \
    char _a1[4097], _regex1[4097];                                      \
        snprintf(_a1, sizeof(_a1), "%s", (_a));                         \
        snprintf(_regex1, sizeof(_regex1), "%s", (_regex));             \
        bs_test_t *_test1 = _test;                                      \
        regex_t regex;                                                  \
        int rv = regcomp(&regex, _regex1, REG_EXTENDED);                \
        if (0 != rv) {                                                  \
            char err_buf[512];                                          \
            regerror(rv, &regex, err_buf, sizeof(err_buf));             \
            bs_test_fail(_test1, "Failed regcomp(\"%s\"): %s",          \
                         _regex1, err_buf);                             \
        } else {                                                        \
            regmatch_t matches[1];                                      \
            rv = regexec(&regex, _a, 1, matches, 0);                    \
            regfree(&regex);                                            \
            if (REG_NOMATCH == rv) {                                    \
                bs_test_fail(_test1, "\"%s\" does not match \"%s\".",   \
                             _a1, _regex);                              \
            }                                                           \
        }                                                               \
    }

/** Test cases. */
extern const bs_test_case_t   bs_test_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_TEST_H__ */
/* == End of test.h ===================================================== */
