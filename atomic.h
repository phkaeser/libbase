/* ========================================================================= */
/**
 * @file atomic.h
 * Methods for atomic access to a few basic types.
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
#ifndef __LIBBASE_ATOMIC_H__
#define __LIBBASE_ATOMIC_H__

#include "test.h"

#include <stdint.h>

#if defined(__cplusplus)

// C++ (gcc) is not compatible with <stdatomic.h>. When using C++, please
// resort to using C++ STL <atomic> definitions instead.
//
// Therefore, the __cplusplus #if is just an empty block.

#else  // defined(__cplusplus)

/* == Types and Definitions ================================================ */

#if (defined(__GNUC__) || defined(__clang__)) && defined(i386)
#define __BS_ATOMIC_GCC_ASM_i386
#elif (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
#define __BS_ATOMIC_GCC_ASM_x86_64

#else

/** C11 supports atomics. We expect support for at least 32-bit atomics. */
#define __BS_ATOMIC_C11_STDATOMIC
#include <stdatomic.h>

#if defined(ATOMIC_LLONG_LOCK_FREE)

/** We can use C11 definitions for 64-bit atomics. */
#define __BS_ATOMIC_C11_STDATOMIC_INT64
#else  // defined(ATOMIC_LLONG_LOCK_FREE)
/** fall back to use a mutex for 64-bit atomics. */
#define __BS_ATOMIC_INT64_MUTEX
#endif  // defined(ATOMIC_LLONG_LOCK_FREE)

#endif  //  (defined(__GNUC__) || defined(__clang__)) && (i386 || __x86_64__).

#if defined(__BS_ATOMIC_INT64_MUTEX)
#include <pthread.h>
static pthread_mutex_t        _bs_atomic_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif  // defined(__BS_ATOMIC_INT64_MUTEX)

/**
 * Initializes a bs_atomic_int32_t to value v.
 */
#define BS_ATOMIC_INT32_INIT(v)  { (v) }

/**
 * Initializes a bs_atomic_int64_t to value v.
 */
#define BS_ATOMIC_INT64_INIT(v)  { (v) }

/** An atomically accessible 32-bit integer. */
typedef struct {
#if defined(__BS_ATOMIC_C11_STDATOMIC)
    /** The actual value. */
    atomic_int_least32_t      v;
#else  // defined (__BS_ATOMIC_C11_STDATOMIC)
    /** The actual value. */
    int32_t                   v;
#endif  // defined (__BS_ATOMIC_C11_STDATOMIC)
} bs_atomic_int32_t;

/** An atomically accessible 64-bit integer. */
typedef struct {
#if defined(__BS_ATOMIC_C11_STDATOMIC_INT64)
    /** The actual value . */
    atomic_int_least64_t      v;
#else  // defined (__BS_ATOMIC_C11_STDATOMIC_INT64)
    /** The actual value . */
    int64_t                   v;
#endif  // defined (__BS_ATOMIC_C11_STDATOMIC_INT64)
} bs_atomic_int64_t;

/* == 32-bit integer ======================================================= */

/* ------------------------------------------------------------------------- */
/** Set atomic value to v. */
static inline void bs_atomic_int32_set(bs_atomic_int32_t *a_ptr, int32_t v)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386) || defined(__BS_ATOMIC_GCC_ASM_x86_64)

    // A 32-bit assignment is atomic on i386 */
    a_ptr->v = v;

#elif defined(__BS_ATOMIC_C11_STDATOMIC)

    atomic_store(&a_ptr->v, v);

#else
#error "Unsupported compiler or architecture."
#endif
}

/* ------------------------------------------------------------------------- */
/** Get value of atomic. */
static inline int32_t bs_atomic_int32_get(bs_atomic_int32_t *a_ptr)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386) || defined(__BS_ATOMIC_GCC_ASM_x86_64)

    /* A 32-bit read is atomic on i386 */
    return a_ptr->v;

#elif defined(__BS_ATOMIC_C11_STDATOMIC)

    return atomic_load(&a_ptr->v);

#else
#error "Unsupported compiler or architecture."
#endif

}

/* ------------------------------------------------------------------------- */
/** Add value to atomic. */
static inline int32_t bs_atomic_int32_add(bs_atomic_int32_t *a_ptr, int32_t v)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386) || defined(__BS_ATOMIC_GCC_ASM_x86_64)
    int32_t old_v = v;
    __asm__ volatile (
        "lock; \n"
        "xaddl %0, %1; \n"
        : "=r" (v)
        : "m" (a_ptr->v), "0" (v)
        : "memory"
        );
    return v + old_v;
#elif defined(__BS_ATOMIC_C11_STDATOMIC)

    return atomic_fetch_add(&a_ptr->v, v) + v;

#else
#error "Unsupported compiler or architecture."
#endif
}

/* ------------------------------------------------------------------------- */
/**
 * Compare-And-Swap value with atomic.
 *
 * If the returned value is not equal to old_val, no exchange was done.
 *
 * @param a_ptr
 * @param new_val             Value to assign to atomic.
 * @param old_val             Value to compare for.
 *
 * @return value of atomic.
 */
static inline int32_t bs_atomic_int32_cas(bs_atomic_int32_t *a_ptr,
                                          int32_t new_val, int32_t old_val)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386) || defined(__BS_ATOMIC_GCC_ASM_x86_64)
    int32_t curr_val;
    __asm__ volatile (
        "lock; \n"
        "cmpxchgl %2, %1; \n"
        : "=a" (curr_val), "=m" (a_ptr->v)
        : "r" (new_val), "m" (a_ptr->v), "0" (old_val)
        );
    return curr_val;
#elif defined(__BS_ATOMIC_C11_STDATOMIC)

    if (atomic_compare_exchange_strong(&a_ptr->v, &old_val, new_val)) {
        return old_val;
    } else {
        return atomic_load(&a_ptr->v);
    }

#else
#error "Unsupported compiler or architecture."
#endif
}

/* ------------------------------------------------------------------------- */
/** Exchange value with atomic. */
static inline void bs_atomic_int32_xchg(bs_atomic_int32_t *a_ptr,
                                        int32_t *v_ptr)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386) || defined(__BS_ATOMIC_GCC_ASM_x86_64)
    __asm__ volatile (
        "lock; \n"
        "xchgl %0, %%eax; \n"
        : "+m" (a_ptr->v), "=a" (*v_ptr)
        : "m" (a_ptr->v), "a" (*v_ptr)
        );
#elif defined(__BS_ATOMIC_C11_STDATOMIC)

    *v_ptr = atomic_exchange(&a_ptr->v, *v_ptr);

#else
#error "Unsupported compiler or architecture."
#endif
}

/* == 64-bit integer ======================================================= */

/* ------------------------------------------------------------------------- */
/** Set atomic value to v. */
static inline void bs_atomic_int64_set(bs_atomic_int64_t *a_ptr, int64_t v)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386)
    __asm__ volatile (
        "movl %%ebx, %%esi; \n"         /* backup ebx, because -fPIC */
        "movl %%eax, %%ebx; \n"         /* v goes into ebx:ecx */
        "movl %%edx, %%ecx; \n"
        "1: \n"
        "movl (%%edi), %%eax; \n"       /* load atom value into edx:eax */
        "movl 4(%%edi), %%edx; \n"
        "lock cmpxchg8b (%%edi); \n"
        "jnz 1b; \n"                    /* spin until write was atomic. */
        "movl %%esi, %%ebx; \n"
        :
        : "D" (&a_ptr->v), "A" (v) /* "A" is edx:eax */
        : "ecx", "memory", "esi"
        );
#elif defined(__BS_ATOMIC_GCC_ASM_x86_64)
    __asm__ volatile (
        "movq %1, %0; \n"
        : "=m" (a_ptr->v)
        : "r" (v)
        : "memory"
        );
#elif defined(__BS_ATOMIC_C11_STDATOMIC_INT64)

    atomic_store(&a_ptr->v, v);

#elif defined(__BS_ATOMIC_INT64_MUTEX)

    pthread_mutex_lock(&_bs_atomic_mutex);
    a_ptr->v = v;
    pthread_mutex_unlock(&_bs_atomic_mutex);

#else
#error "Unsupported compiler or architecture."
#endif
}

/* ------------------------------------------------------------------------- */
/** Get value of atomic. */
static inline int64_t bs_atomic_int64_get(bs_atomic_int64_t *a_ptr)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386)
    int64_t rv = 0;
    __asm __volatile (
        "movl %%ebx, %%esi; \n"         /* backup ebx, becuase -fPIC */
        "1: \n"
        "movl (%%edi), %%eax; \n"       /* load atom value into edx:eax */
        "movl 4(%%edi), %%edx; \n"
        "movl %%eax, %%ebx; \n"         /* store in ebx:ecx for verfication */
        "movl %%edx, %%ecx; \n"
        "lock cmpxchg8b (%%edi); \n"
        "jnz 1b; \n"                    /* spin until read was atomic */
        "movl %%esi, %%ebx; \n"
        : "=A" (rv)  /* "A" is edx:eax */
        : "D" (&a_ptr->v)
        : "ecx", "memory", "esi"
        );
    return rv;
#elif defined(__BS_ATOMIC_GCC_ASM_x86_64)
    int64_t rv = 0;
    __asm__ volatile (
        "movq %1, %0; \n"
        : "=r" (rv)
        : "m" (a_ptr->v)
        );
    return rv;
#elif defined(__BS_ATOMIC_C11_STDATOMIC_INT64)

    return atomic_load(&a_ptr->v);

#elif defined(__BS_ATOMIC_INT64_MUTEX)

    int64_t rv;
    pthread_mutex_lock(&_bs_atomic_mutex);
    rv = a_ptr->v;
    pthread_mutex_unlock(&_bs_atomic_mutex);
    return rv;

#else
#error "Unsupported compiler or architecture."
#endif
}

/* ------------------------------------------------------------------------- */
/** Add value to atomic. */
static inline int64_t bs_atomic_int64_add(bs_atomic_int64_t *a_ptr, int64_t v)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386)
    int64_t rv;
    __asm__ volatile (
        "pushl %%ebx; \n"               /* must backup ebx, because -fPIC */
        "1: \n"

        "movl (%%edi), %%eax; \n"       /* current atomic into edx:eax */
        "movl 4(%%edi), %%edx; \n"
        "movl %%eax, %%ebx; \n"         /* for verification, copy to ecx:ebx */
        "movl %%edx, %%ecx; \n"
        "addl (%%esi), %%ebx; \n"       /* add 'v' */
        "adcl 4(%%esi), %%ecx; \n"

        "lock cmpxchg8b (%%edi); \n"
        "jnz 1b; \n"                    /* spin until results op was atomic */

        "movl %%ebx, %%eax; \n"         /* copy result to edx:eax */
        "movl %%ecx, %%edx; \n"
        "popl %%ebx; \n"
        : "=A" (rv)
        : "D" (&a_ptr->v), "S" (&v)
        : "ecx", "memory"
        );
    return rv;
#elif defined(__BS_ATOMIC_GCC_ASM_x86_64)
    int64_t old_v = v;
    __asm__ volatile (
        "lock; \n"
        "xaddq %0, %1; \n"
        : "=r" (v)
        : "m" (a_ptr->v), "0" (v)
        : "memory"
        );
    return v + old_v;
#elif defined(__BS_ATOMIC_C11_STDATOMIC_INT64)

    return atomic_fetch_add(&a_ptr->v, v) + v;

#elif defined(__BS_ATOMIC_INT64_MUTEX)

    int64_t rv;
    pthread_mutex_lock(&_bs_atomic_mutex);
    a_ptr->v +=v;
    rv = a_ptr->v;
    pthread_mutex_unlock(&_bs_atomic_mutex);
    return rv;

#else
#error "Unsupported compiler or architecture."
#endif
}

/* ------------------------------------------------------------------------- */
/**
 * Compare-And-Swap value with atomic.
 *
 * If the returned value is not equal to old_val, no exchange was done.
 *
 * @param a_ptr
 * @param new_val             Value to assign to atomic.
 * @param old_val             Value to compare for.
 *
 * @return value of atomic.
 * */
static inline int64_t bs_atomic_int64_cas(bs_atomic_int64_t *a_ptr,
                                          int64_t new_val, int64_t old_val)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386)
    int64_t rv;
    __asm __volatile (
        "movl 4(%%esi), %%ecx; \n"
        "movl (%%esi), %%esi; \n"
        "xchgl %%esi, %%ebx; \n"
        "lock cmpxchg8b (%%edi); \n"
        "xchgl %%esi, %%ebx; \n"
        : "=A" (rv)
        : "D" (&a_ptr->v), "A" (old_val), "S" (&new_val)
        : "ecx", "memory"
        );
    return rv;
#elif defined(__BS_ATOMIC_GCC_ASM_x86_64)
    int64_t curr_val;
    __asm__ volatile (
        "lock;  \n"
        "cmpxchgq %2, %1; \n"
        : "=a" (curr_val), "=m" (a_ptr->v)
        : "r" (new_val), "m" (a_ptr->v), "0" (old_val)
        : "memory"
        );
    return curr_val;
#elif defined(__BS_ATOMIC_C11_STDATOMIC_INT64)

    if (atomic_compare_exchange_strong(&a_ptr->v, &old_val, new_val)) {
        return old_val;
    } else {
        return atomic_load(&a_ptr->v);
    }

#elif defined(__BS_ATOMIC_INT64_MUTEX)

    int64_t rv;
    pthread_mutex_lock(&_bs_atomic_mutex);
    if (a_ptr->v == old_val) {
        a_ptr->v = new_val;
        rv = old_val;
    } else {
        rv = a_ptr->v;
    }
    pthread_mutex_unlock(&_bs_atomic_mutex);
    return rv;

#else
#error "Unsupported compiler or architecture."
#endif
}

/* ------------------------------------------------------------------------- */
/** Exchange value with atomic. */
static inline void bs_atomic_int64_xchg(bs_atomic_int64_t *a_ptr,
                                        int64_t *v_ptr)
{
#if defined(__BS_ATOMIC_GCC_ASM_i386)
    // Exchange using CAS.
    int64_t curr_value;
    do {
        curr_value = bs_atomic_int64_get(a_ptr);
    } while (curr_value != bs_atomic_int64_cas(a_ptr, *v_ptr, curr_value));
    *v_ptr = curr_value;
#elif defined(__BS_ATOMIC_GCC_ASM_x86_64)
    __asm__ volatile (
        "lock; \n"
        "xchgq %0, %%rax; \n"
        : "+m" (a_ptr->v), "=a" (*v_ptr)
        : "m" (a_ptr->v), "a" (*v_ptr)
        );
#elif defined(__BS_ATOMIC_C11_STDATOMIC_INT64)

    *v_ptr = atomic_exchange(&a_ptr->v, *v_ptr);

#elif defined(__BS_ATOMIC_INT64_MUTEX)

    pthread_mutex_lock(&_bs_atomic_mutex);
    int64_t tmp = a_ptr->v;
    a_ptr->v = *v_ptr;
    *v_ptr = tmp;
    pthread_mutex_unlock(&_bs_atomic_mutex);

#else
#error "Unsupported compiler or architecture."
#endif
}

#endif  // defined(__cplusplus)

/** Unit tests. */
extern const bs_test_case_t   bs_atomic_test_cases[];

#endif /* __LIBBASE_ATOMIC_H__ */
/* == End of atomic.h ====================================================== */
