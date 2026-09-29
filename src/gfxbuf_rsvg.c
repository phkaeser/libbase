/* ========================================================================= */
/**
 * @file gfxbuf_rsvg.c
 *
 * @copyright
 * Copyright 2026 Google LLC
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

#include <cairo.h>
#include <glib-object.h>
#include <glib.h>
#include <libbase/libbase.h>
#include <librsvg/rsvg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "libbase/gfxbuf_rsvg.h"

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool bs_gfxbuf_render_rsvg(
    bs_gfxbuf_t *gfxbuf_ptr,
    RsvgHandle *rsvg_handle_ptr)
{
    // Create corresponding cairo. Errors are logged there.
    cairo_t *cairo_ptr = cairo_create_from_bs_gfxbuf(gfxbuf_ptr);
    if (NULL == cairo_ptr) return false;

    RsvgRectangle viewport = {
        .width = gfxbuf_ptr->width,
        .height = gfxbuf_ptr->height
    };
    GError *error_ptr = NULL;
    bool rv = rsvg_handle_render_document(
        rsvg_handle_ptr, cairo_ptr, &viewport, &error_ptr);
    cairo_destroy(cairo_ptr);
    if (!rv) {
        bs_log(BS_ERROR, "Failed rsvg_handle_render_document()%s%s",
               error_ptr ? ": " : "",
               error_ptr ? error_ptr->message : "");
    }
    if (NULL != error_ptr) free(error_ptr);

    return rv;
}

/* == Unit tests =========================================================== */
/** @cond TEST */

static void _bs_gfxbuf_rsvg_test_embedded(bs_test_t *test_ptr);

static const bs_test_case_t _bs_gfxbuf_rsvg_test_cases[] = {
    { true, "embedded", _bs_gfxbuf_rsvg_test_embedded },
    { false, NULL, NULL },
};

const bs_test_set_t bs_gfxbuf_rsvg_test_set = BS_TEST_SET(
    true, "gfxbuf_rsvg", _bs_gfxbuf_rsvg_test_cases);

/* ------------------------------------------------------------------------- */
void _bs_gfxbuf_rsvg_test_embedded(bs_test_t *test_ptr)
{
    static const char *svg_ptr = (
        "<svg width=\"1\" height=\"1\" viewBox=\"0 0 1 1\">"
        "<rect width=\"1\" height=\"1\" fill=\"#4080c0\"/>"
        "</svg>");

    bs_gfxbuf_t *g = bs_gfxbuf_create(1, 1);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, g);

    GError *e = NULL;
    RsvgHandle *h = rsvg_handle_new_from_data((const void*)svg_ptr, strlen(svg_ptr), &e);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, h);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_gfxbuf_render_rsvg(g, h));
    g_object_unref(h);

    BS_TEST_VERIFY_EQ(test_ptr, 0xff4080c0, *bs_gfxbuf_pixel_at(g, 0, 0));
    bs_gfxbuf_destroy(g);
}

/** @endcond */
/* == End of gfxbuf_rsvg.c ================================================= */
