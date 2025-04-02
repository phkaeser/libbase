/* ========================================================================= */
/**
 * @file subprocess_test_hang.c
 * Test executable for subprocess module: Keep looping, does not exit.
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

#include <poll.h>
#include <stddef.h>
#include <stdbool.h>

/** A program that will never exit. */
int main() {
    while (true) {
        poll(NULL, 0, 100);
    }
}

/* == End of subprocess_test_hang.c ======================================== */
