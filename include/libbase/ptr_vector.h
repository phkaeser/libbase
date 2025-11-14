/* ========================================================================= */
/**
 * @file ptr_vector.h
 *
 * Interface for a simple vector to store pointers.
 *
 * @copyright
 * Copyright 2024 Google LLC
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
#ifndef __LIBBASE_PTR_VECTOR_H__
#define __LIBBASE_PTR_VECTOR_H__

#include <stdbool.h>
#include <stddef.h>

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** State of a vector that stores pointers. */
typedef struct {
    /** Current capacity of the vector. Required for re-sizing. */
    size_t                    capacity;
    /** Currently consumed capacity of the bector. */
    size_t                    consumed;
    /** The elements. */
    void                      **elements_ptr;
} bs_ptr_vector_t;

/**
 * Initializes the vector.
 *
 * @param ptr_vector_ptr
 *
 * @return true on success.
 */
bool bs_ptr_vector_init(bs_ptr_vector_t *ptr_vector_ptr);

/**
 * Un-initializes the vector.
 *
 * @param ptr_vector_ptr
 */
void bs_ptr_vector_fini(bs_ptr_vector_t *ptr_vector_ptr);

/** @return  the size of the vector, ie. @ref bs_ptr_vector_t::consumed. */
size_t bs_ptr_vector_size(bs_ptr_vector_t *ptr_vector_ptr);

/**
 * Adds `data_ptr` at the end of the vector.
 *
 * @param ptr_vector_ptr
 * @param data_ptr
 *
 * @return true on success.
 */
bool bs_ptr_vector_push_back(bs_ptr_vector_t *ptr_vector_ptr,
                             void *data_ptr);

/**
 * Erases the element at pos.
 *
 * @param ptr_vector_ptr
 * @param pos
 *
 * @return true if pos was valid.
 */
bool bs_ptr_vector_erase(bs_ptr_vector_t *ptr_vector_ptr,
                         size_t pos);

/** @return the element at `pos`. It must be `pos` < size. */
void* bs_ptr_vector_at(bs_ptr_vector_t *ptr_vector_ptr, size_t pos);

/** Unit test set. */
extern const bs_test_set_t    bs_ptr_vector_test_set;

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_PTR_VECTOR_H__ */
/* == End of ptr_vector.h ================================================== */
