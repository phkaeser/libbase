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

#include <errno.h>
#include <fcntl.h>
#include <libbase/assert.h>
#include <libbase/log.h>
#include <libbase/sock.h>
#include <libbase/strutil.h>
#include <libbase/test.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


/* == Data ================================================================= */

bs_log_severity_t      bs_log_severity = BS_WARNING;

static const char*     _severity_names[5] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

static int             _log_fd = 2;

static const char *_strip_prefix(const char *path_ptr);

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
    size_t pos = 0;

    const char *color_attr_ptr = "";
    switch (severity & 0x7f) {
    case BS_DEBUG: color_attr_ptr = "\e[90m"; break;  // Dark gray foreground.
    case BS_INFO: color_attr_ptr = "\e[37m"; break;  // Light gray foreground.
    case BS_WARNING: color_attr_ptr = "\e[1;93m"; break;  // Yellow & bold.
    case BS_ERROR: color_attr_ptr = "\e[1;91m"; break;  // Bright red & bold.
    case BS_FATAL: color_attr_ptr = "\e[1;97;41m"; break; // White on red,bold.
    }
    char *reset_ptr = "";
    if (*color_attr_ptr != '\0') {
        reset_ptr = "\e[0m";
    }

    struct timeval tv;
    if (0 != gettimeofday(&tv, NULL)) {
        memset(&tv, 0, sizeof(tv));
    }
    struct tm *tm_ptr = localtime(&tv.tv_sec);
    pos = bs_strappendf(
        buf, BS_LOG_MAX_BUF_SIZE, pos,
        "%04d-%02d-%02d %02d:%02d:%02d.%03d (%s%s%s) \e[90m%s:%d\e[0m ",
        tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday,
        tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec,
        (int)(tv.tv_usec / 1000),
        color_attr_ptr,
        _severity_names[severity & 0x7f],
        reset_ptr,
        _strip_prefix(file_name_ptr), line_num);

    pos = bs_strappendf(buf, BS_LOG_MAX_BUF_SIZE, pos, "%s", color_attr_ptr);
    pos = bs_vstrappendf(buf, BS_LOG_MAX_BUF_SIZE, pos, fmt_ptr, ap);
    if (severity & BS_ERRNO) {
        pos = bs_strappendf(
            buf, BS_LOG_MAX_BUF_SIZE, pos,
            ": errno(%d): %s", errno, strerror(errno));
    }
    pos = bs_strappendf(buf, BS_LOG_MAX_BUF_SIZE, pos, "%s", reset_ptr);
    if (pos >= BS_LOG_MAX_BUF_SIZE) {
        pos = BS_LOG_MAX_BUF_SIZE;
        buf[BS_LOG_MAX_BUF_SIZE - 3] = '.';
        buf[BS_LOG_MAX_BUF_SIZE - 2] = '.';
        buf[BS_LOG_MAX_BUF_SIZE - 1] = '.';
    }
    buf[pos++] = '\n';

    size_t written_bytes = 0;
    while (written_bytes < pos) {
        ssize_t more_bytes = write(_log_fd, &buf[written_bytes],
                                   pos - written_bytes);
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

/** Strips leading relative (or absolute) path prefix. */
const char *_strip_prefix(const char *path_ptr)
{
    if (*path_ptr == '.') ++path_ptr;
    if (*path_ptr == '/') ++path_ptr;
    return path_ptr;
}

/* == Test functions ======================================================= */

/* ------------------------------------------------------------------------- */
/**
 * Helper method: Test that |expected_str| can be read from the descriptor
 * |fd|. Will verify length and report to |test_ptr|.
 *
 * @param test_ptr
 * @param fname_ptr
 * @param line
 * @param fd
 * @param expected_str
 */
void verify_log_output_equals_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    int line,
    int fd,
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
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "Absurdly long string: %zu bytes!", expected_len);
        return;
    }

    ssize_t available_len = bs_sock_read(fd, buf, expected_len, 10);
    if (0 > available_len) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "Failed reading \"%s\" from %d", expected_str, fd);
        return;
    }

    if ((size_t)available_len != expected_len) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "Only found %zd bytes, expected %zu (\"%.*s\" != \"%s\"",
            available_len, expected_len, (int)available_len, buf,
            expected_str);
        return;
    }

    BS_TEST_VERIFY_STREQ(test_ptr, expected_str, &buf[timestamp_len + 1]);

    if (0 != bs_sock_poll_read(fd, 10)) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "Unexpected extra (> %zu) bytes found reading \"%s\"",
            expected_len, expected_str);
        return;
    }
}

static void test_strip_prefix(bs_test_t *test_ptr);
static void test_log(bs_test_t *test_ptr);

const bs_test_case_t          bs_log_test_cases[] = {
    { 1, "basename", test_strip_prefix },
    { 1, "log", test_log },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
void test_strip_prefix(bs_test_t *test_ptr)
{
    BS_TEST_VERIFY_STREQ(test_ptr, "", _strip_prefix(""));
    BS_TEST_VERIFY_STREQ(test_ptr, "base", _strip_prefix("base"));
    BS_TEST_VERIFY_STREQ(test_ptr, "base", _strip_prefix("/base"));
    BS_TEST_VERIFY_STREQ(test_ptr, "base", _strip_prefix("./base"));
    BS_TEST_VERIFY_STREQ(test_ptr, "a/path/to/base", _strip_prefix("/a/path/to/base"));
    BS_TEST_VERIFY_STREQ(test_ptr, "", _strip_prefix("./"));
    BS_TEST_VERIFY_STREQ(test_ptr, ".", _strip_prefix("/."));
}

/* ------------------------------------------------------------------------- */
void test_log(bs_test_t *test_ptr)
{
    char                      expected_output[BS_LOG_MAX_BUF_SIZE + 1];
    bs_log_severity_t         backup_severity;

    backup_severity = bs_log_severity;

    int fds[2];
    if (0 != pipe(fds)) {
        BS_TEST_FAIL(
            test_ptr,
            "Failed pipe(%p): errno(%d): %s", fds, errno, strerror(errno));
        return;
    }
    if (!bs_sock_set_blocking(fds[0], false)) {
        BS_TEST_FAIL(
            test_ptr,
            "Failed bs_sock_set_blocking(%d, false)", fds[0]);
        return;
    }

    _log_fd = fds[1];
    snprintf(expected_output, sizeof(expected_output),
             "(\e[1;93mWARNING\e[0m) \e[90mlog.c:%d\e[0m \e[1;93mtest 42\e[0m\n",
             __LINE__ + 1);
    bs_log(BS_WARNING, "test %d", 42);
    verify_log_output_equals_at(
        test_ptr, __FILE__, __LINE__, fds[0], expected_output);

    snprintf(expected_output, sizeof(expected_output),
             "(\e[1;91mERROR\e[0m) \e[90mlog.c:%d\e[0m \e[1;91mtest 43"
             ": errno(%d): Permission denied\e[0m\n",
             __LINE__ + 2, EACCES);
    errno = EACCES;
    bs_log(BS_ERROR | BS_ERRNO, "test %d", 43);
    verify_log_output_equals_at(
        test_ptr, __FILE__, __LINE__, fds[0], expected_output);

    bs_log(BS_INFO, "test %d", 44);
    verify_log_output_equals_at(
        test_ptr, __FILE__, __LINE__, fds[0], NULL);

    bs_log_severity = BS_INFO;
    snprintf(expected_output, sizeof(expected_output),
             "(\e[37mINFO\e[0m) \e[90mlog.c:%d\e[0m \e[37mtest 45\e[0m\n",
             __LINE__ + 1);
    bs_log(BS_INFO, "test %d", 45);
    verify_log_output_equals_at(
        test_ptr, __FILE__, __LINE__, fds[0], expected_output);

    _log_fd = 2;

    close(fds[0]);
    close(fds[1]);
    bs_log_severity = backup_severity;
}

/* == End of log.c ========================================================= */
