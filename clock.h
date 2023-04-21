/* ========================================================================= */
/**
 * @file clock.h
 * Methods for retrieving system time, respectively clock counter.
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
#ifndef __BS_CLOCK_H__
#define __BS_CLOCK_H__

#include "test.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Returns the current time, in microseconds since epoch. */
uint64_t bs_usec(void);

/** Returns a monotonous time counter in nsec, as CLOCK_MONOTONIC. */
uint64_t bs_mono_nsec(void);

/** Unit tests. */
extern const bs_test_case_t   bs_clock_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __BS_CLOCK_H__ */
/* == End of clock.h ======================================================= */
