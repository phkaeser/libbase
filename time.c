/* ========================================================================= */
/**
 * @file time.c
 *
 * @license
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

// clock_gettime(2) is a POSIX extension, needs this macro.
#define _POSIX_C_SOURCE 199309L

#include "log.h"
#include "time.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#undef _POSIX_C_SOURCE

/* == Methods ============================================================== */

/* ------------------------------------------------------------------------- */
uint64_t bs_usec(void)
{
    struct timeval tv;
    if (0 != gettimeofday(&tv, NULL)) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed gettimeofday(%p,  NULL)\n",
               (void*)&tv);
        return 0;
    }
    return (uint64_t)tv.tv_sec * UINT64_C(1000000) +
        (uint64_t)tv.tv_usec % UINT64_C(1000000);
}

/* ------------------------------------------------------------------------- */
uint64_t bs_mono_nsec(void)
{
    struct timespec timespec;
    if (0 != clock_gettime(CLOCK_MONOTONIC, &timespec)) {
        bs_log(BS_ERROR | BS_ERRNO,
               "Failed clock_gettime(CLOCK_MONOTONIC, %p)",
               &timespec);
        return 0;
    }

    return timespec.tv_sec * UINT64_C(1000000000) +  + timespec.tv_nsec;
}

/* == Unit tests =========================================================== */

static void bs_time_test_usec(bs_test_t *test_ptr);
static void bs_time_test_nsec(bs_test_t *test_ptr);

const bs_test_case_t          bs_time_test_cases[] = {
    { 1, "time usec", bs_time_test_usec },
    { 1, "time_mono_nsec", bs_time_test_nsec },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
void bs_time_test_usec(bs_test_t *test_ptr)
{
    BS_TEST_VERIFY_NEQ(test_ptr, 0, bs_usec());
}

/* ------------------------------------------------------------------------- */
void bs_time_test_nsec(bs_test_t *test_ptr)
{
    uint64_t v1, v2;
    v1 = bs_mono_nsec();
    v2 = bs_mono_nsec();
    BS_TEST_VERIFY_TRUE(test_ptr, v1 <= v2);
}

/* == End of time.c ======================================================== */
