/* ========================================================================= */
/**
 * @file thread.c
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

#include "thread.h"

#include "assert.h"
#include "log.h"

#include <errno.h>
#include <sys/time.h>

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool bs_mutex_init(pthread_mutex_t *mutex_ptr)
{
    int rv = pthread_mutex_init(mutex_ptr, NULL);
    if (0 != rv) {
        errno = rv;
        bs_log(BS_ERROR | BS_ERRNO, "Failed pthread_mutex_init(%p, NULL)",
               mutex_ptr);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
void bs_mutex_destroy(pthread_mutex_t *mutex_ptr)
{
    int rv = pthread_mutex_destroy(mutex_ptr);
    if (0 != rv) {
        errno = rv;
        bs_log(BS_ERROR | BS_ERRNO, "Failed pthread_mutex_destroy(%p)",
               mutex_ptr);
        BS_ABORT();
    }
}

/* ------------------------------------------------------------------------- */
void bs_mutex_lock(pthread_mutex_t *mutex_ptr)
{
    int rv = pthread_mutex_lock(mutex_ptr);
    if (0 != rv) {
        errno = rv;
        bs_log(BS_FATAL | BS_ERRNO, "Failed pthread_mutex_lock(%p)",
               mutex_ptr);
        BS_ABORT();
    }
}

/* ------------------------------------------------------------------------- */
void bs_mutex_unlock(pthread_mutex_t *mutex_ptr)
{
    int rv = pthread_mutex_unlock(mutex_ptr);
    if (0 != rv) {
        errno = rv;
        bs_log(BS_FATAL | BS_ERRNO, "Failed pthread_mutex_unlock(%p)",
               mutex_ptr);
        BS_ABORT();
    }
}

/* ------------------------------------------------------------------------- */
bool bs_cond_init(pthread_cond_t *condition_ptr)
{
    int rv = pthread_cond_init(condition_ptr, NULL);
    if (0 != rv) {
        errno = rv;
        bs_log(BS_ERROR | BS_ERRNO, "Failed pthread_cond_init(%p)",
               condition_ptr);
        return false;
    }
    return true;
 }

/* ------------------------------------------------------------------------- */
void bs_cond_destroy(pthread_cond_t *condition_ptr)
{
    int rv = pthread_cond_destroy(condition_ptr);
    if (0 != rv) {
        errno = rv;
        bs_log(BS_FATAL | BS_ERRNO, "Failed pthread_cond_destroy(%p)",
               condition_ptr);
        BS_ABORT();
    }
}

/* ------------------------------------------------------------------------- */
void bs_cond_broadcast(pthread_cond_t *condition_ptr)
{
    int rv = pthread_cond_broadcast(condition_ptr);
    if (0 != rv) {
        errno = rv;
        bs_log(BS_FATAL | BS_ERRNO, "Failed pthread_cond_broadcast(%p)",
               condition_ptr);
        BS_ABORT();
    }
}

/* ------------------------------------------------------------------------- */
bool bs_cond_timedwait(pthread_cond_t *condition_ptr,
                       pthread_mutex_t *mutex_ptr,
                       uint64_t usec)
{
    struct                    timeval tv;
    struct                    timespec ts;
    int                       rv;

    if (0 != gettimeofday(&tv, NULL)) {
        bs_log(BS_FATAL | BS_ERRNO, "Failed gettimeofday(%p, NULL)", &tv);
        BS_ABORT();
    }
    ts.tv_sec = tv.tv_sec + (tv.tv_usec + usec) / 1000000;
    ts.tv_nsec = ((tv.tv_usec + usec) % 1000000) * 1000;

    rv = pthread_cond_timedwait(condition_ptr, mutex_ptr, &ts);
    if (0 != rv && ETIMEDOUT != rv) {
        errno = rv;
        bs_log(BS_FATAL | BS_ERRNO,
               "Failed pthread_cond_timedwait(%p, %p, %"PRIu64")",
               condition_ptr, mutex_ptr, usec);
        BS_ABORT();
    }
    return rv == 0;
}

/* == End of thread.c ====================================================== */
