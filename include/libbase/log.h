/* ========================================================================= */
/**
 * @file log.h
 * Logging library, to stderr or a log file. Used for verbose error reporting.
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
#ifndef __LIBBASE_LOG_H__
#define __LIBBASE_LOG_H__

#include "def.h"
#include "test.h"

#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Maximum size of one log message, excluding terminating NUL. */
#define BS_LOG_MAX_BUF_SIZE 4096

/** Severity for logging. */
typedef enum {
    BS_DEBUG = 0,
    BS_INFO = 1,
    BS_WARNING = 2,
    BS_ERROR = 3,
    /** Always log. No matter the current severity. And: abort(). */
    BS_FATAL = 4,

    /** Can be OR-ed to above values, to report strerror & errno. */
    BS_ERRNO = 0x80
} bs_log_severity_t;

/** Current severity for logging. Only equal-or-higher severity is logged. */
extern bs_log_severity_t      bs_log_severity;

/** Actually write a log message. Helper to bs_log. */
void bs_log_write(bs_log_severity_t severity,
                  const char *file_name_ptr,
                  int line_num,
                  const char *fmt_ptr, ...)
    __ARG_PRINTF__(4, 5);

/** Same as bs_log_write(), but with va_list arg. */
void bs_log_vwrite(bs_log_severity_t severity,
                   const char *file_name_ptr,
                   int line_num,
                   const char *fmt_ptr, va_list ap)
    __ARG_PRINTF__(4, 0);

/** Initialize logging system to write to |log_filename_ptr| at |severity|. */
bool bs_log_init_file(const char *log_filename_ptr,
                      bs_log_severity_t severity);

/** Returns whether log outut will happen for `severity`. */
static inline bool bs_will_log(bs_log_severity_t severity)
{
    return  ((severity & 0x7f) >= bs_log_severity ||
             (severity & 0x7f) == BS_FATAL);
}


/** Write a log message, at specified severity. */
#define bs_log(_severity, ...) {                                        \
        bs_log_severity_t _tmp_sev = (_severity);                       \
        if (bs_will_log(_tmp_sev)) {                                    \
            bs_log_write(_tmp_sev, __FILE__, __LINE__, __VA_ARGS__);    \
        }                                                               \
    }

/** Unit tests. */
extern const bs_test_case_t   bs_log_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_LOG_H__ */
/* == End of log.h ========================================================= */
