/* ========================================================================= */
/**
 * @file thread.h
 * Wrappers around libpthread, to consolidate logging and error handling.
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
#ifndef __BS_THREAD_H__
#define __BS_THREAD_H__

#include <pthread.h>
#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Initializes as PTHREAD_MUTEX_NORMAL mutex, with error handling. */
bool bs_mutex_init(pthread_mutex_t *mutex_ptr);
/** Destroys the mutex, with error handling. Aborts on error. */
void bs_mutex_destroy(pthread_mutex_t *mutex_ptr);

/** Locks the mutex, with error handling: aborts on error. */
void bs_mutex_lock(pthread_mutex_t *mutex_ptr);
/** Unlocks the mutex, with error handling: aborts on error. */
void bs_mutex_unlock(pthread_mutex_t *mutex_ptr);

/** Initializes the condition, with default attributes. With error handling. */
bool bs_cond_init(pthread_cond_t *condition_ptr);
/** Destroys the condition, with error handling: abors on error. */
void bs_cond_destroy(pthread_cond_t *condition_ptr);

/** Broadcasts the condition. */
void bs_cond_broadcast(pthread_cond_t *condition_ptr);
/**
 * Waits for condition, with error handling. Returns true, if the condition
 * was signalled or broadcasted, and false if it timed out.
 */
bool bs_cond_timedwait(pthread_cond_t *condition_ptr,
                       pthread_mutex_t *mutex_ptr,
                       uint64_t usec);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __BS_THREAD_H__ */
/* == End of thread.h ====================================================== */
