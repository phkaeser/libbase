/* ========================================================================= */
/**
 * @file strutil.h
 * Utility functions for working with strings in C.
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
#ifndef __LIBBASE_STRUTIL_H__
#define __LIBBASE_STRUTIL_H__

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>

#include "def.h"
#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Appends a formatted string to `buf` at `buf_pos`, respecting `buf_size`.
 *
 * @param buf                 Points to the buffer destined to hold the result.
 *                            The formatted string will be written to
 *                            `&buf[buf_pos]`.
 * @param buf_size            Size of `buf`. The written bytes, inculding the
 *                            terminating NUL character, will not exceed
 *                            `buf_size`.
 * @param buf_pos             Position where to write to, within `buf`. Use
 *                            this for conveniently chaining `strappendf`.
 * @param fmt_ptr             Format string. See printf for documentation.
 * @param ...                 Further arguments.
 *
 * @return The position of where the trailining NUL character was written to.
 *     The number of written characters is `buf_size` minus the return value.
 *     If the buffer was too small for holding all output (including the NUL
 *     terminator), the return value will be larger or equal than buf_size.
 *     It is undefined by how much larger the return value is.
 */
size_t bs_strappendf(
    char *buf,
    size_t buf_size,
    size_t buf_pos,
    const char *fmt_ptr, ...) __ARG_PRINTF__(4, 5);

/** Same as @ref bs_strappendf, with a va_list argument. */
size_t bs_vstrappendf(
    char *buf,
    size_t buf_size,
    size_t buf_pos,
    const char *fmt_ptr,
    va_list ap) __ARG_PRINTF__(4, 0);

/** Appends a non-formatted string. See @ref bs_strappendf for arguments. */
size_t bs_strappend(
    char *buf,
    size_t buf_size,
    size_t buf_pos,
    const char *str_ptr);

/**
 * Converts a uint64_t from |string_ptr| with |base| and stores it in
 * |value_ptr|.
 *
 * The string is considered valid if it is fully consumed, or if the conversion
 * ends parsing at a whitespace character. Unlike stroull(3), a leading minus
 * sign is detected and returned as failure.
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

/**
 * Converts a int64_t from |string_ptr| with |base| and stores it in
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
bool bs_strconvert_int64(
    const char *string_ptr,
    int64_t *value_ptr,
    int base);

/**
 * Converts a double from |string_ptr| and stores it in |value_ptr|.
 *
 * The string is considered valid if it is fully consumed, or if the conversion
 * ends parsing at a whitespace character.
 *
 * @param string_ptr
 * @param value_ptr
 *
 * @return true on success
 */
bool bs_strconvert_double(
    const char *string_ptr,
    double *value_ptr);

/** @return Whether `string_ptr` starts with `prefix_ptr`. */
bool bs_str_startswith(const char *string_ptr, const char *prefix_ptr);

/**
 * Creates a new string holding the format string and outputs.
 *
 * @param fmt_ptr
 * @param ...                 Further arguments.
 *
 * @return A pointer to a string of appropriate size, including terminating NUL
 *     character. Must be released by calling free(). Upon error, NULL is
 *     returned.
 */
char *bs_strdupf(const char *fmt_ptr, ...) __ARG_PRINTF__(1, 2);

/** Test cases. */
extern const bs_test_case_t   bs_strutil_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_STRUTIL_H__ */
/* == End of strutil.h ================================================== */
