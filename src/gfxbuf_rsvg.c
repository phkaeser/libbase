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

/* ------------------------------------------------------------------------- */
bool bs_gfxbuf_render_rsvg_data(
    bs_gfxbuf_t *gfxbuf_ptr,
    const void *data_ptr,
    size_t data_size)
{
    GError *error_ptr = NULL;
    RsvgHandle *svg_handle_ptr = rsvg_handle_new_from_data(
        data_ptr, data_size, &error_ptr);
    if (NULL == svg_handle_ptr) {
        bs_log(BS_ERROR, "Failed rsvg_handle_new_from_data(%p, %zu, %p)%s%s",
               data_ptr, data_size, &error_ptr,
               error_ptr ? ": " : "",
               error_ptr ? error_ptr->message : "");
        if (NULL != error_ptr) free(error_ptr);
        return false;
    }

    bool rv = bs_gfxbuf_render_rsvg(gfxbuf_ptr, svg_handle_ptr);
    g_object_unref(svg_handle_ptr);
    return rv;
}

/* ------------------------------------------------------------------------- */
bool bs_gfxbuf_render_rsvg_file(
    bs_gfxbuf_t *gfxbuf_ptr,
    const char *fname_ptr)
{
    GError *error_ptr = NULL;
    RsvgHandle *svg_handle_ptr = rsvg_handle_new_from_file(
        fname_ptr, &error_ptr);
    if (NULL == svg_handle_ptr) {
        bs_log(BS_ERROR, "Failed rsvg_handle_new_from_data(\"%s\", %p)%s%s",
               fname_ptr,
               &error_ptr,
               error_ptr ? ": " : "",
               error_ptr ? error_ptr->message : "");
        if (NULL != error_ptr) free(error_ptr);
        return false;
    }

    bool rv = bs_gfxbuf_render_rsvg(gfxbuf_ptr, svg_handle_ptr);
    g_object_unref(svg_handle_ptr);
    return rv;
}

/* == Unit tests =========================================================== */
/** @cond TEST */

static void _bs_gfxbuf_rsvg_test_embedded(bs_test_t *test_ptr);
static void _bs_gfxbuf_rsvg_test_file(bs_test_t *test_ptr);

static const bs_test_case_t _bs_gfxbuf_rsvg_test_cases[] = {
    { true, "embedded", _bs_gfxbuf_rsvg_test_embedded },
    { true, "file", _bs_gfxbuf_rsvg_test_file },
    { false, NULL, NULL },
};

const bs_test_set_t bs_gfxbuf_rsvg_test_set = BS_TEST_SET(
    true, "gfxbuf_rsvg", _bs_gfxbuf_rsvg_test_cases);

/* ------------------------------------------------------------------------- */
void _bs_gfxbuf_rsvg_test_embedded(bs_test_t *test_ptr)
{
    static const char *s = (
        "<svg width=\"1\" height=\"1\" viewBox=\"0 0 1 1\">"
        "<rect width=\"1\" height=\"1\" fill=\"#4080c0\"/>"
        "</svg>");

    bs_gfxbuf_t *g = bs_gfxbuf_create(1, 1);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, g);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_gfxbuf_render_rsvg_data(g, s, strlen(s)));

    BS_TEST_VERIFY_EQ(test_ptr, 0xff4080c0, *bs_gfxbuf_pixel_at(g, 0, 0));
    bs_gfxbuf_destroy(g);
}

/* ------------------------------------------------------------------------- */
void _bs_gfxbuf_rsvg_test_file(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *g = bs_gfxbuf_create(1, 1);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, g);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_gfxbuf_render_rsvg_file(
            g,
            bs_test_data_path(test_ptr, "data/example.svg")));

    BS_TEST_VERIFY_EQ(test_ptr, 0xffc0b0a0, *bs_gfxbuf_pixel_at(g, 0, 0));
    bs_gfxbuf_destroy(g);
}

/** @endcond */
/* == End of gfxbuf_rsvg.c ================================================= */
