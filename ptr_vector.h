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

/** Unit tests. */
extern const bs_test_case_t   bs_ptr_vector_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_PTR_VECTOR_H__ */
/* == End of ptr_vector.h ================================================== */
