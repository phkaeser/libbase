/* ========================================================================= */
/**
 * @file dequeue.h
 * Provides a double-ended queue.
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
#ifndef __DEQUEUE_H__
#define __DEQUEUE_H__

#include "assert.h"
#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** A double-ended queue. */
typedef struct _bs_dequeue_t bs_dequeue_t;
/** A node in a double-ended queue. */
typedef struct _bs_dequeue_node_t bs_dequeue_node_t;

/** Details of the queue. */
struct _bs_dequeue_t {
    /** Head of the queue. */
    bs_dequeue_node_t          *head_ptr;
    /** Tail of the queue. */
    bs_dequeue_node_t          *tail_ptr;
};

/** Details of the node. */
struct _bs_dequeue_node_t {
    /** Next node. */
    bs_dequeue_node_t          *next_ptr;
};

/** Pushes |node_ptr| to the front of the double-ended |queue_ptr|. */
static inline void bs_dequeue_push_front(
    bs_dequeue_t *queue_ptr, bs_dequeue_node_t *node_ptr)
{
    node_ptr->next_ptr = queue_ptr->head_ptr;
    queue_ptr->head_ptr = node_ptr;

    if (NULL == queue_ptr->tail_ptr) {
        BS_ASSERT(NULL == node_ptr->next_ptr);
        queue_ptr->tail_ptr = node_ptr;
    }
}

/** Pushes |node_ptr| to the back the double-ended |queue_ptr|. */
static inline void bs_dequeue_push_back(
    bs_dequeue_t *queue_ptr, bs_dequeue_node_t *node_ptr)
{
    node_ptr->next_ptr = NULL;
    if (NULL != queue_ptr->tail_ptr) {
        BS_ASSERT(NULL == queue_ptr->tail_ptr->next_ptr);
        queue_ptr->tail_ptr->next_ptr = node_ptr;
    } else {
        BS_ASSERT(NULL == queue_ptr->head_ptr);
        queue_ptr->head_ptr = node_ptr;
    }
    queue_ptr->tail_ptr = node_ptr;
}

/** Pops the (head) node of the double-ended queue at |queue_ptr|. */
static inline bs_dequeue_node_t *bs_dequeue_pop(bs_dequeue_t *queue_ptr)
{
    bs_dequeue_node_t *node_ptr = queue_ptr->head_ptr;
    if (NULL == queue_ptr->head_ptr) {
        BS_ASSERT(NULL == queue_ptr->tail_ptr);
    } else {
        queue_ptr->head_ptr = queue_ptr->head_ptr->next_ptr;
        if (queue_ptr->tail_ptr == node_ptr) {
            BS_ASSERT(NULL == queue_ptr->head_ptr);
            queue_ptr->tail_ptr = NULL;
        }
        node_ptr->next_ptr = NULL;
    }
    return node_ptr;
}

/** Unit tests. */
extern const bs_test_case_t   bs_dequeue_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __DEQUEUE_H__ */
/* == End of dequeue.h ===================================================== */
