/* ========================================================================= */
/**
 * @file assert.h
 * Assertions.
 *
 * @license
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
#ifndef __BS_ASSERT_H__
#define __BS_ASSERT_H__

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#define BS_ABORT() { __asm __volatile ( "int $3;" ); }
#else
#include <signal.h>
#define BS_ABORT() { raise(SIGTRAP); }
#endif

#if defined(BS_ASSERT)
#undef BS_ASSERT
#endif

#define BS_ASSERT(_expr) {                                              \
        if (!(_expr)) {                                                 \
            bs_log(BS_FATAL, "ASSERT failed: %s", #_expr);              \
            BS_ABORT();                                                 \
        }                                                               \
    }

#endif /* __BS_ASSERT_H__ */
/* == End of assert.h ====================================================== */
