/* ========================================================================= */
/**
 * @file assert.h
 * Assertions.
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
#ifndef __LIBBASE_ASSERT_H__
#define __LIBBASE_ASSERT_H__

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#define BS_ABORT() { __asm __volatile ( "int $3;" ); }
#else
#include <signal.h>
/** Triggers an abort, ie. fatal error. */
#define BS_ABORT() { raise(SIGTRAP); }
#endif

#if defined(BS_ASSERT)
#undef BS_ASSERT
#endif

/** An assertion, triggers a fatal error if `_expr` is false. */
#define BS_ASSERT(_expr) do {                                           \
        if (!(_expr)) {                                                 \
            bs_log(BS_FATAL, "ASSERT failed: %s", #_expr);              \
            BS_ABORT();                                                 \
        }                                                               \
    } while (0)

/** Asserts that _expr is not NULL, and returns the value of _expr. */
#define BS_ASSERT_NOTNULL(_expr)                                        \
    ({                                                                  \
        __typeof__(_expr) __expr = (_expr);                             \
        BS_ASSERT(NULL != __expr);                                      \
        __expr;                                                         \
    })

#endif /* __LIBBASE_ASSERT_H__ */
/* == End of assert.h ====================================================== */
