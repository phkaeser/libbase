/* ========================================================================= */
/**
 * @file libbase_benchmark.c
 * Benchmarks for libbase (in the form of unit test cases).
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
const bs_test_set_t           libbase_benchmarks[] = {
    { 1, "bs_gfxbuf", bs_gfxbuf_benchmarks },
    { 0, NULL, NULL }
};

/** Main program, runs all unit tests. */
int main(int argc, const char **argv)
{
    return bs_test(libbase_benchmarks, argc, argv, NULL);
}

/* == End of libbase_benchmark.c =========================================== */
