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

#include "test.h"

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


/**
 * Joins `path_ptr` and `fname_ptr` and resolves the real path to it.
 *
 * @param path_ptr
 * @param fname_ptr
 * @param joined_realpath_ptr If specified as NULL; then bs_file_join_realpath
 *                            will allocate a buffer of suitable length, and
 *                            return the pointer to it. The caller then needs
 *                            to release that buffer by calling free(3).
 *
 * @return NULL if the joined path is too long, or the file cannot be resolved.
 *     Upon success, a pointer to the path is returned. If joined_realpath_ptr
 *     was NULL, it must be released by calling free(3).
 */
char *bs_file_join_realpath(
    const char *path_ptr,
    const char *fname_ptr,
    char *joined_realpath_ptr);

/**
 * Looks up a file from the set of provided paths.
 *
 * @param fname_ptr           Name of the file that shall be looked up.
 * @param paths_ptr_ptr       A NULL-terminated array of paths to search.
 * @param mode                Optional, indicates to only consider these types
 *                            of the file. Matches the `st_mode` field of stat.
 * @param lookedup_path_ptr   See the `joined_realpath_ptr` to @ref
 *                            bs_file_join_realpath.
 *
 * @return NULL if no suitable file was found. Upon success, a pointer to the
 *     resolved real path is returned. If `lookedup_path_ptr` was NULL; the
 *     returned value must be released by calling free(3).
 */
char *bs_file_lookup(const char *fname_ptr,
                     const char **paths_ptr_ptr,
                     int mode,
                     char *lookedup_path_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_file_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_FILE_H__ */
/* == End of file.h ======================================================== */
