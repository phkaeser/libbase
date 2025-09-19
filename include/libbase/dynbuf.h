/* ========================================================================= */
/**
 * @file dynbuf.h
 *
 * @copyright
 * Copyright 2025 Google LLC
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
 * Copyright (c) 2025 by Philipp Kaeser <kaeser@gubbe.ch>
 */
#ifndef __DYNBUF_H__
#define __DYNBUF_H__

#include <stdlib.h>
#include <stdbool.h>

#include "libbase/test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** A dynamically growing buffer. Useful for reading input. */
typedef struct {
    /** Points to the data area. */
    void                      *data_ptr;
    /** Current length of actual data. */
    size_t                    length;
    /** Capacity of the buffer. */
    size_t                    capacity;
    /** Max permitted capacity of the buffer. */
    size_t                    max_capacity;
    /** Whether it was initialized from an unmanaged data buffer. */
    bool                      unmanaged;
} bs_dynbuf_t;

/**
 * Initializes the buffer.
 *
 * @param dynbuf_ptr
 * @param initial_capacity
 * @param max_capacity
 *
 * @return true on success.
 */
bool bs_dynbuf_init(
    bs_dynbuf_t *dynbuf_ptr,
    size_t initial_capacity,
    size_t max_capacity);

/**
 * Initializes the buffer from an unowned and statically-sized data.
 *
 * @param dynbuf_ptr
 * @param data_ptr            Must outlive dynbuf_ptr.
 * @param capacity
 */
void bs_dynbuf_init_unmanaged(
    bs_dynbuf_t *dynbuf_ptr,
    void *data_ptr,
    size_t capacity);

/**
 * Un-initializes the buffer. Frees @ref bs_dynbuf_t::data_ptr.
 *
 * @param dynbuf_ptr
 */
void bs_dynbuf_fini(bs_dynbuf_t *dynbuf_ptr);

/**
 * Allocates a buffer. Calls into @ref bs_dynbuf_init.
 *
 * @param initial_capacity
 * @param max_capacity
 *
 * @return Pointer to a @ref bs_dynbuf_t. Must be released by calling
 *     @ref bs_dynbuf_destroy.
 */
bs_dynbuf_t *bs_dynbuf_create(
    size_t initial_capacity,
    size_t max_capacity);

/**
 * Destroys the dynamic buffer.
 *
 * @param dynbuf_ptr
 */
void bs_dynbuf_destroy(bs_dynbuf_t *dynbuf_ptr);

/** @return whether the buffer is full. */
bool bs_dynbuf_full(bs_dynbuf_t *dynbuf_ptr);

/**
 * Clears the buffer's contents: Resets length.
 *
 * @param dynbuf_ptr
 */
void bs_dynbuf_clear(bs_dynbuf_t *dynbuf_ptr);

/**
 * Reads from the file descriptor into the dynamic buffer.
 *
 * Grows the buffer as needed. Reads until reaching the end of the file, or
 * (in case of a non-blocking socket descriptor) until no more data is
 * currently available.
 *
 * @param dynbuf_ptr
 * @param fd
 *
 * @return 0 if having reached the end of the file, 1 the end of the file was
 * not reached yet, and -1 on error.
 */
int bs_dynbuf_read(bs_dynbuf_t *dynbuf_ptr, int fd);

/**
 * Appends data to the buffer.
 *
 * @param dynbuf_ptr
 * @param data_ptr
 * @param len
 *
 * @return true on success.
 */
bool bs_dynbuf_append(
    bs_dynbuf_t *dynbuf_ptr,
    const void *data_ptr,
    size_t len);

/**
 * Appends a char to the buffer.
 *
 * @param dynbuf_ptr
 * @param c
 *
 * @return true on success.
 */
bool bs_dynbuf_append_char(
    bs_dynbuf_t *dynbuf_ptr,
    char c);

/** Unit tests. */
extern const bs_test_case_t   bs_dynbuf_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __DYNBUF_H__ */
/* == End of dynbuf.h ================================================== */
