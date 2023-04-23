/* ========================================================================= */
/**
 * @file ptr_stack.h
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
#ifndef __PTR_STACK_H__
#define __PTR_STACK_H__

#include <stddef.h>
#include <stdbool.h>

#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct {
    size_t                    size;
    size_t                    pos;
    void                      **data_ptr;
} bs_ptr_stack_t;

/**
 * Creates a pointer stack.
 *
 * @return Pointer to the stack, or NULL on error. Must be destroyed by calling
 *     @ref bs_btr_stack_destroy.
 */
bs_ptr_stack_t *bs_ptr_stack_create(void);

/**
 * Destroys the pointer stack.
 *
 * @param ptr_stack_ptr
 */
void bs_ptr_stack_destroy(bs_ptr_stack_t *ptr_stack_ptr);

/**
 * Initializes the pointer stack. Use this for static allocations of
 * @ref bs_ptr_stack_t. Associated resources need to be cleaned up by calling
 * @ref bs_ptr_stack_fini.
 *
 * @param ptr_stack_ptr
 *
 * @return true on success.
 */
bool bs_ptr_stack_init(bs_ptr_stack_t *ptr_stack_ptr);

/**
 * Cleans up resources of associated `ptr_stack_ptr`.
 *
 * @param ptr_stack_ptr
 */
void bs_ptr_stack_fini(bs_ptr_stack_t *ptr_stack_ptr);

/**
 * Pushes `elem_ptr` to the stack.
 *
 * @param elem_ptr            Pointer to the element to be pushed. Must not be
 *                            NULL.
 *
 * @return true on success.
 */
bool bs_ptr_stack_push(bs_ptr_stack_t *ptr_stack_ptr, void *elem_ptr);

/**
 * Pops the topmost element from the stack and returns it.
 *
 * @return Pointer to the popped element, or NULL if the stack was empty.
 */
void *bs_ptr_stack_pop(bs_ptr_stack_t *ptr_stack_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_ptr_stack_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __PTR_STACK_H__ */
/* == End of ptr_stack.h =================================================== */
