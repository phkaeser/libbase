/* ========================================================================= */
/**
 * @file log.c
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

#include "assert.h"
#include "log.h"
#include "sock.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

/* == Data ================================================================= */

bs_log_severity_t      bs_log_severity = BS_WARNING;

static const char*     _severity_names[5] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

static int             _log_fd = 2;

static const char *_basename(const char *path_ptr);

/* == Functions ============================================================ */

/* ------------------------------------------------------------------------- */
bool bs_log_init_file(const char *log_filename_ptr,
                      bs_log_severity_t severity)
{
    int fd = open(log_filename_ptr, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
    if (0 > fd) {
        bs_log(BS_ERROR | BS_ERRNO,
               "Failed open(%s, O_CREATE | O_WRONLY, S_IWUSR | S_IRUSR)",
               log_filename_ptr);
        return false;
    }
    _log_fd = fd;
    bs_log_severity = severity;
    return true;
}

/* ------------------------------------------------------------------------- */
void bs_log_write(bs_log_severity_t severity,
                  const char *file_name_ptr,
                  int line_num,
                  const char *fmt_ptr, ...)
{
    va_list                   ap;
    va_start(ap, fmt_ptr);
    bs_log_vwrite(severity, file_name_ptr, line_num, fmt_ptr, ap);
    va_end(ap);
}

/* ------------------------------------------------------------------------- */
/** Writes the log mesage, taking a va_list argument. */
void bs_log_vwrite(bs_log_severity_t severity,
                   const char *file_name_ptr,
                   int line_num,
                   const char *fmt_ptr, va_list ap)
{
    char buf[BS_LOG_MAX_BUF_SIZE + 1];
    int len;

    struct timeval tv;
    if (0 != gettimeofday(&tv, NULL)) {
        memset(&tv, 0, sizeof(tv));
    }
    struct tm *tm_ptr = localtime(&tv.tv_sec);

    len = snprintf(buf, BS_LOG_MAX_BUF_SIZE,
                   "%04d-%02d-%02d %02d:%02d:%02d.%03d %s:%d (%s) ",
                   tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday,
                   tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec,
                   (int)(tv.tv_usec / 1000),
                   _basename(file_name_ptr), line_num,
                   _severity_names[severity & 0x7f]);

    if (len < BS_LOG_MAX_BUF_SIZE) {
        len += vsnprintf(&buf[len], BS_LOG_MAX_BUF_SIZE - len, fmt_ptr, ap);
    }
    if (severity & BS_ERRNO && len < BS_LOG_MAX_BUF_SIZE) {
        len += snprintf(&buf[len], BS_LOG_MAX_BUF_SIZE - len,
                        ": errno(%d): %s", errno, strerror(errno));
    }
    if (len >= BS_LOG_MAX_BUF_SIZE) {
        len = BS_LOG_MAX_BUF_SIZE;
        buf[BS_LOG_MAX_BUF_SIZE - 3] = '.';
        buf[BS_LOG_MAX_BUF_SIZE - 2] = '.';
        buf[BS_LOG_MAX_BUF_SIZE - 1] = '.';
    }
    buf[len++] = '\n';

    ssize_t written_bytes = 0;
    while (written_bytes < len) {
        ssize_t more_bytes = write(_log_fd, &buf[written_bytes],
                                   len - written_bytes);
        if (0 > more_bytes) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            abort();
        }
        written_bytes += more_bytes;
    }

    if (severity == BS_FATAL) BS_ABORT();
}


/* == Local (static) methods =============================================== */
const char *_basename(const char *path_ptr)
{
    const char *start_ptr;
    const char *pos_ptr;

    start_ptr = path_ptr;
    pos_ptr = start_ptr;
    while (*pos_ptr) {
        if (*pos_ptr == '/') start_ptr = pos_ptr + 1;
        pos_ptr++;
    }
    return start_ptr;
}

/* == Test functions ======================================================= */

/* ------------------------------------------------------------------------- */
/**
 * Helper method: Test that |expected_str| can be read from the descriptor
 * |fd|. Will verify length and report to |test_ptr|.
 *
 * @param test_ptr
 * @param fd
 * @param expected_str
 */
void verify_log_output_equals(bs_test_t *test_ptr, int fd,
                              const char *expected_str)
{
    char buf[BS_LOG_MAX_BUF_SIZE + 1];
    memset(buf, 0, sizeof(buf));

    size_t timestamp_len = strlen("YYYY-MM-DD hh:mm:ss.ccc");
    size_t expected_len = 0;
    if (NULL != expected_str) {
        expected_len = strlen(expected_str) + timestamp_len + 1;
    } else {
        expected_str = "";
    }
    if (BS_LOG_MAX_BUF_SIZE < expected_len || INT_MAX < expected_len) {
        bs_test_fail(test_ptr, "Absurdly long string: %zu bytes!",
                     expected_len);
        return;
    }

    ssize_t available_len = bs_sock_read(fd, buf, expected_len, 10);
    if (0 > available_len) {
        bs_test_fail(test_ptr, "Failed reading \"%s\" from %d", expected_str,
                     fd);
        return;
    }

    if ((size_t)available_len != expected_len) {
        bs_test_fail(test_ptr,
                     "Only found %zd bytes, expected %zu (\"%.*s\" != \"%s\"",
                     available_len, expected_len, (int)available_len, buf,
                     expected_str);
        return;
    }

    BS_TEST_VERIFY_STREQ(test_ptr, expected_str, &buf[timestamp_len + 1]);

    if (0 != bs_sock_poll_read(fd, 10)) {
        bs_test_fail(test_ptr,
                     "Unexpected extra (> %zu) bytes found reading \"%s\"",
                     expected_len, expected_str);
        return;
    }
}

static void test_basename(bs_test_t *test_ptr);
static void test_log(bs_test_t *test_ptr);

const bs_test_case_t          bs_log_test_cases[] = {
    { 1, "basename", test_basename },
    { 1, "log", test_log },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
void test_basename(bs_test_t *test_ptr)
{
    BS_TEST_VERIFY_STREQ(test_ptr, "", _basename(""));
    BS_TEST_VERIFY_STREQ(test_ptr, "base", _basename("base"));
    BS_TEST_VERIFY_STREQ(test_ptr, "base", _basename("/base"));
    BS_TEST_VERIFY_STREQ(test_ptr, "base", _basename("/a/path/to/base"));
    BS_TEST_VERIFY_STREQ(test_ptr, "", _basename("/a/path/to/base/"));
}

/* ------------------------------------------------------------------------- */
void test_log(bs_test_t *test_ptr)
{
    char                      expected_output[BS_LOG_MAX_BUF_SIZE + 1];
    bs_log_severity_t         backup_severity;

    backup_severity = bs_log_severity;

    int fds[2];
    if (0 != pipe(fds)) {
        bs_test_fail(test_ptr, "Failed pipe(%p): errno(%d): %s", fds, errno,
                     strerror(errno));
        return;
    }
    if (!bs_sock_set_blocking(fds[0], false)) {
        bs_test_fail(test_ptr, "Failed bs_sock_set_blocking(%d, false)", fds[0]);
        return;
    }

    _log_fd = fds[1];
    snprintf(expected_output, sizeof(expected_output),
             "log.c:%d (WARNING) test 42\n", __LINE__ + 1);
    bs_log(BS_WARNING, "test %d", 42);
    verify_log_output_equals(test_ptr, fds[0], expected_output);

    snprintf(expected_output, sizeof(expected_output),
             "log.c:%d (ERROR) test 43: errno(%d): Permission denied\n",
             __LINE__ + 2, EACCES);
    errno = EACCES;
    bs_log(BS_ERROR | BS_ERRNO, "test %d", 43);
    verify_log_output_equals(test_ptr, fds[0], expected_output);

    bs_log(BS_INFO, "test %d", 44);
    verify_log_output_equals(test_ptr, fds[0], NULL);

    bs_log_severity = BS_INFO;
    snprintf(expected_output, sizeof(expected_output),
             "log.c:%d (INFO) test 45\n", __LINE__ + 1);
    bs_log(BS_INFO, "test %d", 45);
    verify_log_output_equals(test_ptr, fds[0], expected_output);

    _log_fd = 2;

    close(fds[0]);
    close(fds[1]);
    bs_log_severity = backup_severity;
}

/* == End of log.c ========================================================= */
