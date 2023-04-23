/* ========================================================================= */
/**
 * @file ptr_set.h
 *
 * Interface for a simple stack to store pointers.
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
#ifndef __PTR_SET_H__
#define __PTR_SET_H__

#include <stdbool.h>

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Handle for a set. */
typedef struct _bs_ptr_set_t bs_ptr_set_t;

/**
 * Creates the set.
 *
 * @return Pointer to the set, or NULL on error
. Must be freed by calling
 *     @ref bs_ptr_set_destroy.
 */
bs_ptr_set_t *bs_ptr_set_create(void);

/**
 * Destroys a set previously created with @ref bs_ptr_set_create.
 *
 * @param set_ptr
 */
void bs_ptr_set_destroy(bs_ptr_set_t *set_ptr);

/**
 * Inserts `elem_ptr` into `set_ptr`.
 *
 * @param set_ptr
 * @param elem_ptr
 *
 * @return true iff the insert worked, false if there already exists the same
 *     `elem_ptr` in the set or if there was an allocation error.
 */
bool bs_ptr_set_insert(bs_ptr_set_t *set_ptr, void *elem_ptr);

/**
 * Erases `elem_ptr` from `set_ptr`.
 *
 * @param set_ptr
 * @param elem_ptr
 */
void bs_ptr_set_erase(bs_ptr_set_t *set_ptr, void *elem_ptr);

/**
 * Returns whether `set_ptr` contains `elem_ptr`.
 *
 * @param set_ptr
 * @param elem_ptr
 *
 * @return whether `set_ptr` contains `elem_ptr`.
 */
bool bs_ptr_set_contains(bs_ptr_set_t *set_ptr, void *elem_ptr);

/**
 * Returns whether `set_ptr` is empty.
 *
 * @param set_ptr
 *
 * @return Whether the set is empty.
 */
bool bs_ptr_set_empty(bs_ptr_set_t *set_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_ptr_set_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __PTR_SET_H__ */
/* == End of ptr_set.h ===================================================== */
