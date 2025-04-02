/* ========================================================================= */
/**
 * @file subprocess_test_success.c
 * Test executable for subprocess module: Exit successfully, with stdout/err.
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

#include <stdio.h>
#include <stdlib.h>

/** A test program that returns a successful code, if args were all correct. */
int main(int argc, char** argv) {
    if (2 != argc) {
        fprintf(stderr, "Expecting 1 arguments.\n");
        return EXIT_FAILURE;
    }

    const char *value_ptr = getenv("SUBPROCESS_ENV");
    fprintf(stdout, "test stdout: %s\nenv: %s\n",
            argv[0], value_ptr ? value_ptr : "(null)");
    fprintf(stderr, "test stderr: %s\n", argv[1]);
    return EXIT_SUCCESS;
}

/* == End of subprocess_test_success.c ===================================== */
