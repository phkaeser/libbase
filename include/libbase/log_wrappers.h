/* ========================================================================= */
/**
 * @file log_wrappers.h
 * Wraps common methods to conveniently log errors.
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
#ifndef __LIBBASE_LOG_WRAPPERS_H__
#define __LIBBASE_LOG_WRAPPERS_H__

#include "log.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Helper: Calls calloc(3), and wraps errors for the given file & line. */
static inline void *_logged_calloc(
    const char *filename_ptr, int line_no, size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if ((NULL == ptr) && (BS_ERROR >= bs_log_severity)) {
        bs_log_write((bs_log_severity_t)(BS_ERROR | BS_ERRNO), filename_ptr,
                     line_no, "Failed calloc(%zu, %zu)", nmemb, size);
    }
    return ptr;
}

/** Calls calloc(3) and logs error with ERROR severity. */
#define logged_calloc(nmemb, size)                      \
    _logged_calloc(__FILE__, __LINE__, nmemb, size)

/** Helper: Calls malloc(3) and logs errors for given file & line. */
static inline void *_logged_malloc(
    const char *filename_ptr, int line_no, size_t size)
{
    void *ptr = malloc(size);
    if ((NULL == ptr) && (BS_ERROR >= bs_log_severity)) {
        bs_log_write((bs_log_severity_t)(BS_ERROR | BS_ERRNO), filename_ptr,
                     line_no, "Failed malloc(%zu)", size);
    }
    return ptr;
}

/** Calls malloc(3) and logs error with ERROR severity. */
#define logged_malloc(size)                     \
    _logged_malloc(__FILE__, __LINE__, size)

/** Helper: Acts like strdup(3) and logs errors for given file & line. */
static inline char *_logged_strdup(
    const char *filename_ptr, int line_no, const char *str)
{
    size_t len = strlen(str) + 1;
    char *new_str = malloc(len);
    if (NULL != new_str) {
        memcpy(new_str, str, len);
    } else if (BS_ERROR >= bs_log_severity) {
        bs_log_write((bs_log_severity_t)(BS_ERROR | BS_ERRNO), filename_ptr,
                     line_no, "Failed strdup(%s)", str);
    }
    return new_str;
}

/** Calls strdup(3) and logs error with ERROR severity. */
#define logged_strdup(str)                      \
    _logged_strdup(__FILE__, __LINE__, str)


#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_LOG_WRAPPERS_H__ */
/* == End of log_wrappers.h ================================================== */
