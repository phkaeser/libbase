/* ========================================================================= */
/**
 * @file sock.c
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

#include "def.h"
#include "log.h"
#include "sock.h"
#include "time.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>

/* == Methods ============================================================== */

/* ------------------------------------------------------------------------- */
bool bs_sock_set_blocking(int fd, bool blocking)
{
    int flags, rv;

    flags = fcntl(fd, F_GETFL);
    if (-1 == flags) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed fcntl(%d, F_GETFL)", fd);
        return false;
    }

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    rv = fcntl(fd, F_SETFL, flags);
    if (0 != rv) {
        bs_log(BS_ERROR | BS_ERRNO,
               "Failed fcntl(%d, F_SETFL, 0x%x)", fd, flags);
        return false;
    }

    return true;
}

/* ------------------------------------------------------------------------- */
int bs_sock_poll_read(int fd, int msec)
{
    struct pollfd             poll_fd;
    int                       rv;

    poll_fd.fd = fd;
    poll_fd.events = POLLIN;
    rv = poll(&poll_fd, 1, msec);
    if (0 > rv) {
        rv = errno;
        bs_log(BS_ERROR | BS_ERRNO, "Failed poll(%d, 1, %d)", fd, msec);
        errno = rv;
        return -1;
    }
    return rv;
}

/* ------------------------------------------------------------------------- */
ssize_t bs_sock_read(int fd, void *buf_ptr, size_t count, int msec)
{
    ssize_t                   consumed_bytes, read_bytes;
    int                       rv;
    uint8_t                   *byte_buf_ptr;
    uint64_t                  start_msec;
    int                       timeout;

    /* catch boundary conditions on count */
    if (INT32_MAX < count) {
        errno = EINVAL;
        return -1;
    }
    if (0 >= count) {
        return 0;
    }

    if (0 > msec) {
        start_msec = bs_usec() / 1000;
    } else {
        start_msec = 0;
    }
    timeout = msec;

    consumed_bytes = 0;
    byte_buf_ptr = (uint8_t*)buf_ptr;
    while ((size_t)consumed_bytes < count) {
        if (0 <= msec) {
            uint64_t now_msec = bs_usec() / 1000;
            if (start_msec + msec > now_msec) {
                timeout = BS_MIN(now_msec - (start_msec + msec),
                                 (uint64_t)INT32_MAX);
            } else {
                timeout = 0;
            }
        }

        rv = bs_sock_poll_read(fd, timeout);
        if (0 > rv) {
            return -1;
        } else if (0 == rv) {
            /* no more data */
            return consumed_bytes;
        }

        read_bytes = read(fd, &byte_buf_ptr[consumed_bytes],
                          count - consumed_bytes);
        if (0 == read_bytes) {
            /* connection was closed, return prescribed error. */
            errno = EPIPE;
            return -1;
        }
        if ((0 > read_bytes) &&
            (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
            rv = errno;
            bs_log(BS_ERROR | BS_ERRNO, "Failed read(%d, %p, %zu)",
                   fd, &byte_buf_ptr[consumed_bytes], count - consumed_bytes);
            errno = rv;
            return -1;
        }

        consumed_bytes += read_bytes;
    }
    return consumed_bytes;
}

/* == End of sock.c ======================================================== */
