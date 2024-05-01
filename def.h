/* ========================================================================= */
/**
 * @file def.h
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
#ifndef __LIBBASE_DEF_H__
#define __LIBBASE_DEF_H__

// For offsetof.
#include <stddef.h>

/* == Definitions for arguments et.al. ===================================== */

#if !defined(__UNUSED__)
/** Compiler hint, indicating unused elements. */
#define __UNUSED__ __attribute__ ((unused))
#endif

#if !defined(__ARG_PRINTF__)
/** Compiler hint, indicating these are printf-style format args. */
#define __ARG_PRINTF__(_fmtidx, _paridx) \
    __attribute__ ((format(printf, _fmtidx, _paridx)))
#endif

/* == Function-like definitions ============================================ */

/** Returns the lesser of (a, b) */
#if !defined(BS_MIN)
#define BS_MIN(a, b)                                \
    ({                                              \
        __typeof__(a) __a = (a);                    \
        __typeof__(b) __b = (b);                    \
        __a < __b ? __a : __b;                      \
    })
#endif  // !defined(BS_MIN)

/** Returns the greater of (a, b) */
#if !defined(BS_MAX)
#define BS_MAX(a, b)                                \
    ({                                              \
        __typeof__(a) __a = (a);                    \
        __typeof__(b) __b = (b);                    \
        __a > __b ? __a : __b;                      \
    })
#endif  // !defined(BS_MAX)

/** Helper to retrieve the base container, given a pointer to an element. */
#define BS_CONTAINER_OF(elem_ptr, container_type, elem_field)   \
    (container_type*)(                                          \
        (uint8_t*)(elem_ptr) -                                  \
        offsetof(container_type, elem_field))

#endif /* __LIBBASE_DEF_H__ */
/* == End of def.h ========================================================= */
