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

#include "strutil.h"

#include "log.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

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
bool bs_strconvert_uint64(
    const char *string_ptr,
    uint64_t *value_ptr,
    int base)
{
    unsigned long long        tmp_value;
    char                      *invalid_ptr;

    invalid_ptr = NULL;
    errno = 0;
    tmp_value = strtoull(string_ptr, &invalid_ptr, base);
    if (0 != errno) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed strtoull for value \"%s\"",
               string_ptr);
        return false;
    }
    if ('\0' != *invalid_ptr && !isspace(*invalid_ptr))  {
        bs_log(BS_ERROR, "Failed strtoull for value \"%s\" at \"%s\"",
               string_ptr, invalid_ptr);
        return false;
    }

    *value_ptr = tmp_value;
    return true;
}

/* == Test functions ======================================================= */

static void test_strappend(bs_test_t *test_ptr);
static void strconvert_uint64_test(bs_test_t *test_ptr);

const bs_test_case_t          bs_strutil_test_cases[] = {
    { 1, "strappend", test_strappend },
    { 1, "strconvert_uint64", strconvert_uint64_test },
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
    out = bs_strappendf(buf, sizeof(buf), in, "jkl");
    BS_TEST_VERIFY_EQ(test_ptr, out, 11);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerj");

    in = out;
    out = bs_strappendf(buf, sizeof(buf), in, "uiop");
    BS_TEST_VERIFY_EQ(test_ptr, out, 11);
    BS_TEST_VERIFY_STREQ(test_ptr, buf, "asdfqwerj");
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
}

/* == End of strutil.c ===================================================== */
