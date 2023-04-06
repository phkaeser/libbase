/* ========================================================================= */
/**
 * @file sock.h
 * Some helper functions to work with sockets.
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
#ifndef __SOCK_H__
#define __SOCK_H__

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Sets the blocking property of the file descriptor |fd|.
 *
 * @param fd
 * @param blocking Have O_NONBLOCK cleared if true, and set if false.
 *
 * @return Whether the call succeeded.
 */
bool bs_sock_set_blocking(int fd, bool blocking);

/**
 * Waits up to |msec| until the file descriptor |fd| has data to read.
 *
 * @param fd
 * @param msec
 *
 * @return a positive value if data is available, 0 if the call timed out, and a
 * negative value if there was an error. The error is logged, and errno is set.
 */
int bs_sock_poll_read(int fd, int msec);

/**
 * Read |count| bytes from |fd| into |buf_ptr|, respecting the timeout given
 * by |msec|.
 *
 * @param fd
 * @param buf_ptr
 * @param count
 * @param msec                Timeout in milliseconds. A negative value
 *                            specifies an infinite timeout.
 *
 * @return Number of bytes read. The return value will always be less or equal
 * than |count|. Returns a negative value on error. Errors will be logged, but
 * errno is retained.
 * If |count| is non-positive, 0 is returned.
 * If the connection was closed, a negative value is returned and errno is
 * set to EPIPE.
 */
ssize_t bs_sock_read(int fd, void *buf_ptr, size_t count, int msec);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __SOCK_H__ */
/* == End of sock.h ======================================================== */
