/* ========================================================================= */
/**
 * @file file.c
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

#include "file.h"

#include "log.h"

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
ssize_t bs_file_read_buffer(
    const char *fname_ptr,
    char *buf_ptr,
    size_t buf_len)
{
    ssize_t                   read_bytes;
    int                       fd;

    fd = open(fname_ptr, O_RDONLY);
    if (0 > fd) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed open(%s, O_RDONLY)",
               fname_ptr);
        return -1;
    }

    read_bytes = read(fd, buf_ptr, buf_len);
    if (0 > read_bytes) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed read(%d, %p, %zu) from %s",
               fd, buf_ptr, buf_len - 1, fname_ptr);
    } if ((size_t)read_bytes >= buf_len) {
        bs_log(BS_WARNING | BS_ERRNO,
               "Read %zd >= %zu bytes. Too much data in %s",
               read_bytes, buf_len, fname_ptr);
        read_bytes = -1;
    } else {
        buf_ptr[read_bytes] = '\0';
    }

    if (0 != close(fd)) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed close(%d) for %s", fd,
               fname_ptr);
        return -1;
    }

    return read_bytes;
}

/* ------------------------------------------------------------------------- */
ssize_t bs_file_write_buffer(
    const char *fname_ptr,
    const char *buf_ptr,
    size_t buf_len)
{
    ssize_t                   written_bytes;
    int                       fd;

    fd = open(fname_ptr, O_WRONLY);
    if (0 > fd) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed open(%s, O_WRONLY)", fname_ptr);
        return -1;
    }

    written_bytes = write(fd, buf_ptr, buf_len);
    if (0 > written_bytes) {
        bs_log(BS_ERROR | BS_ERRNO, "Falied write(%d, %p, %zu) to %s",
               fd, buf_ptr, buf_len, fname_ptr);
    } else if ((size_t)written_bytes != buf_len) {
        bs_log(BS_ERROR, "Incomplete write(%d, %p, %zu): %zd bytes to %s",
               fd, buf_ptr, buf_len, written_bytes, fname_ptr);
        written_bytes = -1;
    }

    if (0 != close(fd)) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed close(%d) for %s", fd,
               fname_ptr);
        return -1;
    }

    return written_bytes;
}

/* == End of file.c ======================================================== */
