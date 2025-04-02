/* ========================================================================= */
/**
 * @file subprocess_test_sigpipe.c
 * Test executable for subprocess module: Killed by (arbitrarily chosen)
 * SIGPIPE.
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

#include <signal.h>

/** A test program that raises a signal. */
int main() {
    raise(SIGPIPE);
}

/* == End of subprocess_test_sigpipe.c ===================================== */
