/* ========================================================================= */
/**
 * @file ref.h
 * Copyright (c) 2026 Philipp Kaeser
 */
#ifndef __REF_H__
#define __REF_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


/* == Declarations ========================================================= */

/** Forward declaration. */
typedef struct _bs_ref_t bs_ref_t;

/** State for reference counting. */
struct _bs_ref_t {
    /** Number of references. */
    int                       count;
    /** dtor. */
    void                      (*destroy_fn)(bs_ref_t *ref_ptr);
};

/* == Exported methods ===================================================== */

/**
 * Initializes the reference counter state.
 *
 * @param ref_ptr
 * @param destroy_fn
 */
void bs_ref_init(
    bs_ref_t *ref_ptr,
    void (*destroy_fn)(bs_ref_t *ref_ptr));

/**
 * Retain a reference to `ref_ptr`.
 *
 * @param ref_ptr
 */
void bs_ref_retain(bs_ref_t *ref_ptr);

/**
 * Releases a reference to `ref_ptr`.
 *
 * Calls @ref bs_ref_t::destroy_fn, if there are no other references.
 *
 * @param ref_ptr
 */
void bs_ref_release(bs_ref_t *ref_ptr);

/** Unit tests. */
extern const bs_test_set_t bs_ref_test_set;

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // __REF_H__
/* == End of ref.h ========================================================= */
