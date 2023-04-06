/* ========================================================================= */
/**
 * @file strconvert.h
 * Methods to convert values from from strings to base type(s).
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
#ifndef __STRCONVERT_H__
#define __STRCONVERT_H__

#include "test.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Converts a uint64_t from |string_ptr| with |base| and stores it in
 * |value_ptr|.
 *
 * The string is considered valid if it is fully consumed, or if the conversion
 * ends parsing at a whitespace character.
 *
 * @param string_ptr
 * @param value_ptr
 * @param base
 *
 * @return true on success.
 */
bool bs_strconvert_uint64(
    const char *string_ptr,
    uint64_t *value_ptr,
    int base);

/** Unit tests. */
extern const bs_test_case_t   bs_strconvert_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __STRCONVERT_H__ */
/* == End of strconvert.h ================================================== */
