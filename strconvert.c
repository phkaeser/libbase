/* ========================================================================= */
/**
 * @file strconvert.c
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

#include "strconvert.h"

#include "log.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

/* == Exported methods ===================================================== */

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

/* == Unit tests =========================================================== */

static void bs_strconvert_uint64_test(bs_test_t *test_ptr);

const bs_test_case_t          bs_strconvert_test_cases[] = {
    { 1, "strconvert_uint64", bs_strconvert_uint64_test },
    { 0, NULL, NULL }
};

/* -- Convert uint64_t ----------------------------------------------------- */
void bs_strconvert_uint64_test(bs_test_t *test_ptr)
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

/* == End of strconvert.c ================================================== */
