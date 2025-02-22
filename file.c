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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "log.h"
#include "strutil.h"

/* == Declarations ========================================================= */

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

/* ------------------------------------------------------------------------- */
char *bs_file_resolve_path(
    const char *path_ptr,
    char *resolved_path_buf_ptr)
{
    char expanded_path[PATH_MAX];
    if (bs_str_startswith(path_ptr, "~/")) {
        const char *home_dir_ptr = getenv("HOME");
        if (NULL == home_dir_ptr) {
            bs_log(BS_WARNING, "Failed getenv(\"HOME\") for path %s",
                   path_ptr);
        } else {
            size_t i = bs_strappend(expanded_path, PATH_MAX, 0, home_dir_ptr);
            i = bs_strappend(expanded_path, PATH_MAX, i, "/");
            i = bs_strappend(expanded_path, PATH_MAX, i, path_ptr + 2);
            if (i >= PATH_MAX) {
                errno = ENAMETOOLONG;
                return NULL;
            }
            path_ptr = &expanded_path[0];
        }
    }

    return realpath(path_ptr, resolved_path_buf_ptr);
}

/* ------------------------------------------------------------------------- */
char *bs_file_join_resolve_path(
    const char *path_ptr,
    const char *fname_ptr,
    char *resolved_path_buf_ptr)
{
    char path[PATH_MAX];
    size_t pos = bs_strappend(path, PATH_MAX, 0, path_ptr);
    pos = bs_strappend(path, PATH_MAX, pos, "/");
    pos = bs_strappend(path, PATH_MAX, pos, fname_ptr);
    if (pos > PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    return bs_file_resolve_path(path, resolved_path_buf_ptr);
}

/* ------------------------------------------------------------------------- */
char *bs_file_resolve_and_lookup_from_paths(
    const char *fname_ptr,
    const char **paths_ptr_ptr,
    int mode,
    char *resolved_path_buf_ptr)
{
    for (; NULL != *paths_ptr_ptr; ++paths_ptr_ptr) {
        char *resolved_path_ptr = bs_file_join_resolve_path(
            *paths_ptr_ptr, fname_ptr, resolved_path_buf_ptr);
        if (NULL == resolved_path_ptr) continue;

        // Found something and not needed to check type? We have it.
        if (0 == mode) return resolved_path_ptr;

        // Otherwise, verify type.
        struct stat stat_buf;
        if (0 == stat(resolved_path_ptr, &stat_buf)) {
            if ((stat_buf.st_mode & S_IFMT) == (mode & S_IFMT)) {
                return resolved_path_ptr;
            }
        }
        if (NULL == resolved_path_buf_ptr) free(resolved_path_ptr);
    }
    return NULL;
}

/* == Static (local) functions ============================================= */

/* == Test Functions ======================================================= */

static void test_resolve_path(bs_test_t *test_ptr);
static void test_join_resolve_path(bs_test_t *test_ptr);
static void test_lookup(bs_test_t *test_ptr);

const bs_test_case_t bs_file_test_cases[] = {
    { 1, "resolve_path", test_resolve_path },
    { 1, "join_resolve_path", test_join_resolve_path },
    { 1, "lookup", test_lookup },
    { 0, NULL, NULL }  // sentinel.
};

/* ------------------------------------------------------------------------- */
void test_resolve_path(bs_test_t *test_ptr)
{
    char *p;

    p = bs_file_resolve_path("/etc/../etc/passwd", NULL);
    BS_TEST_VERIFY_STREQ(test_ptr, "/etc/passwd", p);
    free(p);

    char path[PATH_MAX];
    p = bs_file_resolve_path("/etc/../etc/passwd", path);
    BS_TEST_VERIFY_STREQ(test_ptr, "/etc/passwd", p);
    BS_TEST_VERIFY_EQ(test_ptr, p, &path[0]);

    p = bs_file_resolve_path("~/", path);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
}

/* ------------------------------------------------------------------------- */
void test_join_resolve_path(bs_test_t *test_ptr)
{
    char *p;
    p = bs_file_join_resolve_path("/etc/../etc", "passwd", NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
    free(p);
}

/* ------------------------------------------------------------------------- */
void test_lookup(bs_test_t *test_ptr)
{
    const char *paths[] = { NULL, NULL, NULL };
    char path[PATH_MAX];
    char *p;

#if defined(__LIBBASE_LINUX)
    // The '/proc/self' moniker works only on Linux.
    paths[0] = "/anywhere";
    paths[1] = "/proc/self/cwd";

    p = bs_file_resolve_and_lookup_from_paths(
        "libbase_test", paths, 0, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
    free(p);

    p = bs_file_resolve_and_lookup_from_paths(
        "libbase_test", paths, S_IFBLK, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, p);

    p = bs_file_resolve_and_lookup_from_paths(
        "does_not_exist", paths, 0, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, p);

    // Lookup into provided |path| arg.
    p = bs_file_resolve_and_lookup_from_paths(
        "libbase_test", paths, 0, path);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);

    p = bs_file_resolve_and_lookup_from_paths(
        "libbase_test", paths, S_IFREG, path);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
#endif  // __LIBBASE_LINUX

    paths[0] = "~/";
    paths[1] = NULL;
    p = bs_file_resolve_and_lookup_from_paths(
        "", paths, S_IFDIR, path);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
}

/* == End of file.c ======================================================== */
