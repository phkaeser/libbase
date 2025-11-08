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

#include <sys/types.h>
#include "test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Reads the contents from |fname_ptr| into |buf_ptr|, up to |buf_len|-1 bytes.
 * It will add a trailing NUL character to buf_ptr, for convenience.
 *
 * @param fname_ptr
 * @param buf_ptr
 * @param buf_len
 *
 * @return A negative value on error, or the number of bytes read on success.
 *     Any error will be logged.
 */
// TODO(kaeser@gubbe.ch): Merge with dynbuf.
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
// TODO(kaeser@gubbe.ch): Merge with dynbuf and bs_dynbuf_write_file.
ssize_t bs_file_write_buffer(
    const char *fname_ptr,
    const char *buf_ptr,
    size_t buf_len);

/**
 * Resolves the real path to `path_ptr`, with home directory expansion.
 *
 * @param path_ptr            Path to join from. Paths that start with "~/"
 *                            will be expanded with getenv("HOME").
 * @param resolved_path_buf_ptr If specified as NULL, this method will allocate
 *                            a buffer of suitable length, and return the
 *                            pointer to it. The caller then needs to release
 *                            that buffer by calling free(3).
 *                            Otherwise, the resolved path is stored here.
 *
 * @return Upon success, a pointer to the path is returned. If
 *     resolved_realpath_ptr was NULL, it must be released by calling free(3).
 *     NULL is returned if the the path resolution (realpath) failed, or if
 *     the expanded path was too long. errno will be set appropriately in both
 *     cases (See realpath(3) for the former; ENAMETOOLONG for the latter).
 */
char *bs_file_resolve_path(
    const char *path_ptr,
    char *resolved_path_buf_ptr);

/**
 * Joins and resolves `path_ptr` and `fname_ptr`, with home dir expansion.
 *
 * @param path_ptr            Path to use for joining. Paths that start with
 *                            "~/" will be expanded with getenv("HOME").
 * @param fname_ptr           Filename to join the path on.
 * @param resolved_path_buf_ptr See the `resolved_path_buf_ptr` argument to
 *                            @ref bs_file_resolve_path.
 *
 * @return Upon success, a pointer to the joined, expanded and resolved path,
 *     of NULL on error. See @ref bs_file_resolve_path for details.
 */
char *bs_file_join_resolve_path(
    const char *path_ptr,
    const char *fname_ptr,
    char *resolved_path_buf_ptr);

/**
 * Looks up a file from the set of provided paths, with path resolutio and
 * home directory expansion..
 *
 * @param fname_ptr           Name of the file that shall be looked up.
 * @param paths_ptr_ptr       A NULL-terminated array of paths to search. Paths
 *                            starting with "~/" will be expanded as described
 *                            by @ref bs_file_join_resolve_path.
 * @param mode                Optional, indicates to only consider these types
 *                            of the file. Matches the `st_mode` field of stat.
 * @param resolved_path_buf_ptr See the `resolved_path_buf_ptr` argument to
 *                            @ref bs_file_resolve_path.
 *
 * @return Upon success, a pointer to the joined, expanded and resolved path of
 *     the first file found from a path & file name combination.
 *     Otherwise, NULL on error, with errno set appropriately and for the last
 *     of the attempted paths. See @ref bs_file_resolve_path for details.
 */
char *bs_file_resolve_and_lookup_from_paths(
    const char *fname_ptr,
    const char **paths_ptr_ptr,
    int mode,
    char *resolved_path_buf_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_file_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_FILE_H__ */
/* == End of file.h ======================================================== */
