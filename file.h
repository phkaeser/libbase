/* ========================================================================= */
/**
 * @file file.h
 * Convenience methods for reading a buffer from a file, respectively writing
 * a buffer to a file.
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
#ifndef __LIBBASE_FILE_H__
#define __LIBBASE_FILE_H__

#include <stddef.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Reads the contents from |fname_ptr| into |buf_ptr|, up to |buf_len|-1 bytes.
 * It will att a trailing NUL character to buf_ptr, for convenience.
 *
 * @param fname_ptr
 * @param buf_ptr
 * @param buf_len
 *
 * @return A negative value on error, or the number of bytes read on success.
 *     Any error will be logged.
 */
ssize_t bs_file_read_buffer(
    const char *fname_ptr,
    char *buf_ptr,
    size_t buf_len);

/**
 * Writes |buf_len| bytes from |buf_ptr| into the file |fname_ptr|.
 *
 * @param fname_ptr
 * @param buf_ptr
 * @param buf_len
 *
 * @return A negative value on error, or the number of bytes written.
 *     Any error will be logged.
 */
ssize_t bs_file_write_buffer(
    const char *fname_ptr,
    const char *buf_ptr,
    size_t buf_len);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_FILE_H__ */
/* == End of file.h ======================================================== */
