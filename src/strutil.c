/* ========================================================================= */
/**
 * @file strutil.c
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

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <libbase/def.h>
#include <libbase/log.h>
#include <libbase/strutil.h>
#include <libbase/test.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
size_t bs_strappendf(
    char *buf,
    size_t buf_size,
    size_t buf_pos,
    const char *fmt_ptr, ...)
{
    va_list ap;
    va_start(ap, fmt_ptr);
    size_t rv = bs_vstrappendf(buf, buf_size, buf_pos, fmt_ptr, ap);
    va_end(ap);
    return rv;
}

/* ------------------------------------------------------------------------- */
size_t bs_vstrappendf(
    char *buf,
    size_t buf_size,
    size_t buf_pos,
    const char *fmt_ptr,
    va_list ap)
{
    if (buf_pos >= buf_size) return buf_pos;
    ssize_t rv = vsnprintf(&buf[buf_pos], buf_size - buf_pos, fmt_ptr, ap);
    return buf_pos + BS_MAX(0, rv);
}

/* ------------------------------------------------------------------------- */
size_t bs_strappend(
    char *buf,
    size_t buf_size,
    size_t buf_pos,
    const char *str_ptr)
{
    if (buf_pos >= buf_size) return buf_size;
    size_t len = strlen(str_ptr);
    if (buf_pos + len + 1 >= buf_size) len = buf_size - buf_pos - 1;
    memmove(buf + buf_pos, str_ptr, len);
    buf[buf_pos + len] = '\0';
    return buf_pos + strlen(str_ptr);;
}

/* ------------------------------------------------------------------------- */
bool bs_strconvert_uint64(
    const char *string_ptr,
    uint64_t *value_ptr,
    int base)
{
    unsigned long long        tmp_value;
    char                      *invalid_ptr;

    if ('-' == *string_ptr) {
        bs_log(BS_ERROR, "Unexpected negative value \"%s\"", string_ptr);
        return false;
    }

    invalid_ptr = NULL;
    errno = 0;
    tmp_value = strtoull(string_ptr, &invalid_ptr, base);
    if (0 != errno) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed strtoull for value \"%s\"",
               string_ptr);
        return false;
    }
    if ('\0' != *invalid_ptr && !isspace(*invalid_ptr)) {
        bs_log(BS_ERROR, "Failed strtoull for value \"%s\" at \"%s\"",
               string_ptr, invalid_ptr);
        return false;
    }

    *value_ptr = tmp_value;
    return true;
}

/* ------------------------------------------------------------------------- */
bool bs_strconvert_int64(
    const char *string_ptr,
    int64_t *value_ptr,
    int base)
{
    long long                 tmp_value;
    char                      *invalid_ptr;

    invalid_ptr = NULL;
    errno = 0;
    tmp_value = strtoll(string_ptr, &invalid_ptr, base);
    if (0 != errno) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed strtoll for value \"%s\"",
               string_ptr);
        return false;
    }
    if ('\0' != *invalid_ptr && !isspace(*invalid_ptr)) {
        bs_log(BS_ERROR, "Failed strtoll for value \"%s\" at \"%s\"",
               string_ptr, invalid_ptr);
        return false;
    }

    *value_ptr = tmp_value;
    return true;
}

/* ------------------------------------------------------------------------- */
bool bs_strconvert_double(
    const char *string_ptr,
    double *value_ptr)
{
    char *invalid_ptr = NULL;
    errno = 0;
    double tmp_value = strtod(string_ptr, &invalid_ptr);
    if (0 != errno) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed strtod(\"%s\", %p)",
               string_ptr, &invalid_ptr);
        return false;
    }
    if ('\0' != *invalid_ptr && !isspace(*invalid_ptr)) {
        bs_log(BS_ERROR, "Failed strtod(\"%s\", %p) at \"%s\"",
               string_ptr, &invalid_ptr, invalid_ptr);
        return false;
    }

    *value_ptr = tmp_value;
    return true;
}

/* ------------------------------------------------------------------------- */
bool bs_str_startswith(const char *string_ptr, const char *prefix_ptr)
{
    return 0 == strncmp(string_ptr, prefix_ptr, strlen(prefix_ptr));
}

/* == Test functions ======================================================= */

static void test_strappend(bs_test_t *test_ptr);
static void strconvert_uint64_test(bs_test_t *test_ptr);
static void strconvert_int64_test(bs_test_t *test_ptr);
static void strconvert_double_test(bs_test_t *test_ptr);
static void test_startswith(bs_test_t *test_ptr);

const bs_test_case_t          bs_strutil_test_cases[] = {
    { 1, "strappend", test_strappend },
    { 1, "strconvert_uint64", strconvert_uint64_test },
    { 1, "strconvert_int64", strconvert_int64_test },
    { 1, "strconvert_double", strconvert_double_test },
    { 1, "startswith", test_startswith },
    { 0, NULL, NULL }
};

/* -- Append to string buffer ---------------------------------------------- */
void test_strappend(bs_test_t *test_ptr)
{
    char buf[10];
    size_t in = 0, out;
    out = bs_strappendf(buf, sizeof(buf), in, "asdf");
    BS_TEST_VERIFY_EQ(test_ptr, out, 4);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdf");

    in = out;
    out = bs_strappendf(buf, sizeof(buf), in, "qwer");
    BS_TEST_VERIFY_EQ(test_ptr, out, 8);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwer");

    in = out;
    out = bs_strappendf(buf, sizeof(buf), in, "j");
    BS_TEST_VERIFY_EQ(test_ptr, out, 9);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerj");

    out = bs_strappendf(buf, sizeof(buf), in, "jk");
    BS_TEST_VERIFY_EQ(test_ptr, out, 10);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerj");

    out = bs_strappendf(buf, sizeof(buf), in, "jkl");
    BS_TEST_VERIFY_EQ(test_ptr, out, 11);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerj");

    in = out;
    out = bs_strappendf(buf, sizeof(buf), in, "uiop");
    BS_TEST_VERIFY_EQ(test_ptr, out, 11);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerj");

    in = 0;
    out = bs_strappend(buf, sizeof(buf), in, "asdf");
    BS_TEST_VERIFY_EQ(test_ptr, out, 4);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdf");

    in = out;
    out = bs_strappend(buf, sizeof(buf), in, "qwer");
    BS_TEST_VERIFY_EQ(test_ptr, out, 8);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwer");

    out = bs_strappend(buf, sizeof(buf), 8, "g");
    BS_TEST_VERIFY_EQ(test_ptr, out, 9);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerg");

    out = bs_strappend(buf, sizeof(buf), 8, "gh");
    BS_TEST_VERIFY_EQ(test_ptr, out, 10);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerg");

    out = bs_strappend(buf, sizeof(buf), 8, "ghv");
    BS_TEST_VERIFY_EQ(test_ptr, out, 11);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerg");
}

/* -- Convert uint64_t ----------------------------------------------------- */
void strconvert_uint64_test(bs_test_t *test_ptr)
{
    uint64_t                  value;

    BS_TEST_VERIFY_TRUE(test_ptr, bs_strconvert_uint64("42", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, 42);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_strconvert_uint64("43 ", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, 43);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_strconvert_uint64("44\n", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, 44);

    BS_TEST_VERIFY_TRUE(test_ptr, bs_strconvert_uint64("0", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, 0);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_strconvert_uint64("18446744073709551615", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, 18446744073709551615u);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_strconvert_uint64("0xffffffffffffffff", &value, 16));
    BS_TEST_VERIFY_EQ(test_ptr, value, 18446744073709551615u);

    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bs_strconvert_uint64("18446744073709551616", &value, 10));

    BS_TEST_VERIFY_FALSE(test_ptr, bs_strconvert_uint64("42x", &value, 10));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_strconvert_uint64("-42", &value, 10));
}

/* ------------------------------------------------------------------------- */
void strconvert_int64_test(bs_test_t *test_ptr)
{
    int64_t                   value;

    BS_TEST_VERIFY_TRUE(test_ptr, bs_strconvert_int64("0", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, 0);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_strconvert_int64("9223372036854775807", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, INT64_MAX);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_strconvert_int64("-9223372036854775808", &value, 10));
    BS_TEST_VERIFY_EQ(test_ptr, value, INT64_MIN);

    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bs_strconvert_int64("18446744073709551615", &value, 10));
}

/* ------------------------------------------------------------------------- */
void strconvert_double_test(bs_test_t *test_ptr)
{
    double                    value;

    BS_TEST_VERIFY_TRUE(test_ptr, bs_strconvert_double("0", &value));
    BS_TEST_VERIFY_EQ(test_ptr, value, 0);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_strconvert_double("2.2250738585072014e-308", &value));
    BS_TEST_VERIFY_EQ(test_ptr, value, DBL_MIN);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_strconvert_double("1.7976931348623158e+308", &value));
    BS_TEST_VERIFY_EQ(test_ptr, value, DBL_MAX);

    BS_TEST_VERIFY_FALSE(test_ptr, bs_strconvert_double("badvalue", &value));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_strconvert_double("1e+400", &value));
}

/* ------------------------------------------------------------------------- */
void test_startswith(bs_test_t *test_ptr)
{
    BS_TEST_VERIFY_TRUE(test_ptr, bs_str_startswith("asdf", "asd"));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_str_startswith("asdf", "asdfe"));

    BS_TEST_VERIFY_TRUE(test_ptr, bs_str_startswith("asdf", ""));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_str_startswith("", "asdf"));
}

/* == End of strutil.c ===================================================== */
