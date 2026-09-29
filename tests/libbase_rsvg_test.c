/* ========================================================================= */
/**
 * @file libbase_rsvg_test.c
 * Copyright (c) 2026 Philipp Kaeser
 */

#include <libbase/gfxbuf_rsvg.h>
#include <libbase/libbase.h>
#include <stddef.h>

/* ========================================================================= */
/** Main program, runs all unit tests. */
int main(int argc, const char **argv)
{
    const bs_test_param_t params = {
        .test_data_dir_ptr   = BS_TEST_DATA_DIR
    };

    const bs_test_set_t *sets[] = {
        &bs_gfxbuf_rsvg_test_set,
        NULL
    };

    return bs_test_sets(sets, argc, argv, &params);
}

/* == End of libbase_rsvg_test.c =========================================== */
