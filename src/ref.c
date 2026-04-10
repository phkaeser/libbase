/* ========================================================================= */
/**
 * @file ref.c
 * Copyright (c) 2026 Philipp Kaeser
 */

#include <stdbool.h>
#include <libbase/libbase.h>

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
void bs_ref_init(
    bs_ref_t *ref_ptr,
    void (*destroy_fn)(void *value_ptr),
    const void *(*const_value_from_ref_fn)(bs_ref_t *ref_ptr))
{
    *ref_ptr = (bs_ref_t){
        .count = 1,
        .destroy_fn = destroy_fn,
        .const_value_from_ref_fn = const_value_from_ref_fn
    };
}

/* ------------------------------------------------------------------------- */
const void *bs_ref(bs_ref_t *ref_ptr)
{
    ++ref_ptr->count;
    return ref_ptr->const_value_from_ref_fn(ref_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_unref(bs_ref_t *ref_ptr)
{
    if (0 < --ref_ptr->count) return;
    BS_ASSERT(0 == ref_ptr->count);
    // FIXME: This is ugly.
    ref_ptr->destroy_fn((void*)ref_ptr->const_value_from_ref_fn(ref_ptr));
}

/* == Unit tests =========================================================== */

static void bs_ref_test(bs_test_t *test_ptr);

/** Test cases. */
static const bs_test_case_t   bs_ref_test_cases[] = {
    { true, "ref", bs_ref_test },
    BS_TEST_CASE_SENTINEL()
};

const bs_test_set_t           bs_ref_test_set = BS_TEST_SET(
    true, "ref", bs_ref_test_cases);

/* ------------------------------------------------------------------------- */

/** State used in tests. */
struct _bs_ref_test {
    /** Number of calls to dtor. */
    int dtor_calls;
    /** Reference state. */
    bs_ref_t ref;
};

/** Dtor used in tests. */
static void _bs_ref_test_dtor(void *p) {
    struct _bs_ref_test *t = p;
    ++t->dtor_calls;
}

/** Transforms the value from ref. */
static const void *_bs_ref_value(bs_ref_t *ref_ptr) {
    return BS_CONTAINER_OF(ref_ptr, struct _bs_ref_test, ref);
}

/** Simple test, for ref and unref. */
void bs_ref_test(bs_test_t *test_ptr)
{
    struct _bs_ref_test t = {};

    bs_ref_init(&t.ref, _bs_ref_test_dtor, _bs_ref_value);

    BS_TEST_VERIFY_EQ(test_ptr, 1, t.ref.count);
    bs_ref(&t.ref);
    BS_TEST_VERIFY_EQ(test_ptr, 2, t.ref.count);
    bs_unref(&t.ref);
    BS_TEST_VERIFY_EQ(test_ptr, 1, t.ref.count);
    BS_TEST_VERIFY_EQ(test_ptr, 0, t.dtor_calls);
    bs_unref(&t.ref);
    BS_TEST_VERIFY_EQ(test_ptr, 0, t.ref.count);
    BS_TEST_VERIFY_EQ(test_ptr, 1, t.dtor_calls);


}

/* == End of ref.c ========================================================= */
