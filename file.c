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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* == Declarations ========================================================= */

static char *_bs_file_join_realpath_log_severity(
    bs_log_severity_t severity,
    const char *path_ptr,
    const char *fname_ptr,
    char *joined_realpath_ptr);

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
char *bs_file_join_realpath(
    const char *path_ptr,
    const char *fname_ptr,
    char *joined_realpath_ptr)
{
    return _bs_file_join_realpath_log_severity(
        BS_ERROR, path_ptr, fname_ptr, joined_realpath_ptr);
}

/* ------------------------------------------------------------------------- */
char *bs_file_lookup(const char *fname_ptr,
                     const char **paths_ptr_ptr,
                     int mode,
                     char *lookedup_path_ptr)
{
    for (; NULL != *paths_ptr_ptr; ++paths_ptr_ptr) {
        char *resolved_path_ptr = _bs_file_join_realpath_log_severity(
            BS_DEBUG, *paths_ptr_ptr, fname_ptr, lookedup_path_ptr);
        if (NULL == resolved_path_ptr) continue;

        // Found something and not needed to check type? We have it.
        if (0 == mode) return resolved_path_ptr;

        // Otherwise, verify type.
        struct stat stat_buf;
        if (0 == stat(resolved_path_ptr, &stat_buf)) {
            if ((stat_buf.st_mode & S_IFMT) == (mode & S_IFMT)) {
                return resolved_path_ptr;
            }
        } else {
            bs_log(BS_ERROR | BS_ERRNO, "Failed stat(%s, %p)",
                   resolved_path_ptr, &stat_buf);
        }
        if (NULL == lookedup_path_ptr) free(resolved_path_ptr);
    }

    return NULL;
}

/* == Static (local) functions ============================================= */

/* ------------------------------------------------------------------------- */
/** @ref bs_file_join_realpath. Logs realpath(3) errors with |severity|. */
char *_bs_file_join_realpath_log_severity(
    bs_log_severity_t severity,
    const char *path_ptr,
    const char *fname_ptr,
    char *joined_realpath_ptr)
{
    char joined_path[PATH_MAX + 1];
    int written_bytes = snprintf(joined_path, sizeof(joined_path),
                                 "%s/%s", path_ptr, fname_ptr);
    if (sizeof(joined_path) < (size_t)written_bytes) {
        bs_log(BS_ERROR, "Exceeds PATH_MAX (%d): %s/%s",
               PATH_MAX, path_ptr, fname_ptr);
        return NULL;
    }

    char *resolved_path_ptr = realpath(joined_path, joined_realpath_ptr);
    if (NULL == resolved_path_ptr) {
        bs_log(severity | BS_ERRNO, "Failed realpath(%s, %p)",
               joined_path, joined_realpath_ptr);
        return NULL;
    }

    return resolved_path_ptr;
}

/* == Test Functions ======================================================= */

static void test_join_realpath(bs_test_t *test_ptr);
static void test_lookup(bs_test_t *test_ptr);

const bs_test_case_t bs_file_test_cases[] = {
    { 1, "join_realpath", test_join_realpath },
    { 1, "lookup", test_lookup },
    { 0, NULL, NULL }  // sentinel.
};

/* ------------------------------------------------------------------------- */
void test_join_realpath(bs_test_t *test_ptr)
{
    char *p;

    p = bs_file_join_realpath("/proc/self/cwd", "libbase_test", NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
    free(p);

    char path[PATH_MAX];
    p = bs_file_join_realpath("/proc/self/cwd", "libbase_test", path);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
}

/* ------------------------------------------------------------------------- */
void test_lookup(bs_test_t *test_ptr)
{
    const char *paths[] = { "/anywhere", "/proc/self/cwd", NULL };
    char *p;

    p = bs_file_lookup("libbase_test", paths, 0, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
    free(p);

    p = bs_file_lookup("libbase_test", paths, S_IFBLK, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, p);

    p = bs_file_lookup("does_not_exist", paths, 0, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, p);

    char path[PATH_MAX];
    p = bs_file_lookup("libbase_test", paths, 0, path);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);

    p = bs_file_lookup("libbase_test", paths, S_IFREG, path);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, p);
}

/* == End of file.c ======================================================== */
