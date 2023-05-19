/* ========================================================================= */
/**
 * @file gfxbuf.c
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

#include "gfxbuf.h"

#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>

#include "log_wrappers.h"
#include "test.h"
#include "time.h"

/* == Declarations ========================================================= */

/** Internal handle of a graphics buffer. */
typedef struct {
    /** Publicly accessible elements. */
    bs_gfxbuf_t               public;
    /** Whether `data_ptr` is owned by @ref bs_gfxbuf_t `public`. */
    bool                      managed;
} bs_gfxbuf_internal_t;

/** Returns the @ref bs_gfxbuf_internal_t for a @ref bs_gfxbuf_t. */
static inline bs_gfxbuf_internal_t *internal_from_gfxbuf(
    bs_gfxbuf_t *gfxbuf_ptr)
{
    return BS_CONTAINER_OF(gfxbuf_ptr, bs_gfxbuf_internal_t, public);
}

#ifdef HAVE_CAIRO
/** Image format used for @ref bs_gfxbuf_t, translated to Cairo terms. */
static const cairo_format_t   bs_gfx_cairo_image_format = CAIRO_FORMAT_ARGB32;
#endif  // HAVE_CAIRO

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bs_gfxbuf_t *bs_gfxbuf_create(unsigned width, unsigned height)
{
    uint32_t *data_ptr = (uint32_t*)logged_calloc(1, 4 * width * height);
    if (NULL == data_ptr) return NULL;

    bs_gfxbuf_t *gfxbuf_ptr = bs_gfxbuf_create_unmanaged(
        width, height, width, data_ptr);
    if (NULL == gfxbuf_ptr) {
        free(data_ptr);
    } else {
        bs_gfxbuf_internal_t *gfxbuf_internal_ptr =
            internal_from_gfxbuf(gfxbuf_ptr);
        gfxbuf_internal_ptr->managed = true;
    }
    return gfxbuf_ptr;
}

/* ------------------------------------------------------------------------- */
bs_gfxbuf_t *bs_gfxbuf_create_unmanaged(unsigned width, unsigned height,
                                        unsigned pixels_per_line,
                                        uint32_t* data_ptr)
{
    bs_gfxbuf_internal_t *buf_internal_ptr = logged_calloc(
        1, sizeof(bs_gfxbuf_internal_t));
    if (NULL == buf_internal_ptr) return NULL;

    buf_internal_ptr->public.width = width;
    buf_internal_ptr->public.height = height;
    buf_internal_ptr->public.pixels_per_line = pixels_per_line;
    buf_internal_ptr->public.data_ptr = data_ptr;
    buf_internal_ptr->managed = false;
    return &buf_internal_ptr->public;
}

/* ------------------------------------------------------------------------- */
void bs_gfxbuf_destroy(bs_gfxbuf_t *gfxbuf_ptr)
{
    bs_gfxbuf_internal_t *gfxbuf_internal_ptr =
        internal_from_gfxbuf(gfxbuf_ptr);
    if (gfxbuf_internal_ptr->managed &&
        NULL != gfxbuf_internal_ptr->public.data_ptr) {
        free(gfxbuf_internal_ptr->public.data_ptr);
        gfxbuf_ptr->data_ptr = NULL;
    }
    free(gfxbuf_internal_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_gfxbuf_clear(bs_gfxbuf_t *gfxbuf_ptr, const uint32_t color)
{
    uint32_t *pixel_ptr;

    if (color == 0) {
        for (unsigned y = 0; y < gfxbuf_ptr->height; ++y) {
            pixel_ptr = &gfxbuf_ptr->data_ptr[y * gfxbuf_ptr->pixels_per_line];
            // On amd64, using memset is ca 3x faster than filling self.
            memset(pixel_ptr, 0, sizeof(uint32_t) * gfxbuf_ptr->width);
        }
    } else {
        for (unsigned y = 0; y < gfxbuf_ptr->height; ++y) {
            pixel_ptr = &gfxbuf_ptr->data_ptr[y * gfxbuf_ptr->pixels_per_line];
            unsigned width = gfxbuf_ptr->width;
            while (width--) *pixel_ptr++ = color;
        }
    }
}

/* ------------------------------------------------------------------------- */
void bs_gfxbuf_copy(bs_gfxbuf_t *dest_gfxbuf_ptr,
                    const bs_gfxbuf_t *src_gfxbuf_ptr)
{
    BS_ASSERT(src_gfxbuf_ptr->width == dest_gfxbuf_ptr->width);
    BS_ASSERT(src_gfxbuf_ptr->height == dest_gfxbuf_ptr->height);

    uint32_t *src_pixel_ptr, *dest_pixel_ptr;
    for (unsigned y = 0; y < src_gfxbuf_ptr->height; ++y) {
        src_pixel_ptr = &src_gfxbuf_ptr->data_ptr[
            y * src_gfxbuf_ptr->pixels_per_line];
        dest_pixel_ptr = &dest_gfxbuf_ptr->data_ptr[
            y * dest_gfxbuf_ptr->pixels_per_line];

#if 1
        // On amd64, using memcpy is up to 3x faster.
        memcpy(dest_pixel_ptr, src_pixel_ptr,
               sizeof(uint32_t) * src_gfxbuf_ptr->width);
#else
        unsigned width = src_gfxbuf_ptr->width;
        while (width--) *dest_pixel_ptr++ = *src_pixel_ptr++;
#endif  // 1
    }
}

/* ------------------------------------------------------------------------- */
void bs_gfxbuf_copy_area(
    bs_gfxbuf_t *dest_gfxbuf_ptr,
    unsigned dest_x,
    unsigned dest_y,
    const bs_gfxbuf_t *src_gfxbuf_ptr,
    unsigned src_x,
    unsigned src_y,
    unsigned width,
    unsigned height)
{
    // Sanity check, don't copy from/in outside the valid buffers.
    if (src_gfxbuf_ptr->width <= src_x ||
        src_gfxbuf_ptr->height <= src_y ||
        dest_gfxbuf_ptr->width <= dest_x ||
        dest_gfxbuf_ptr->height <= dest_y) {
        return;
    }

    // Restrict area to buffer dimensions.
    width = BS_MIN(dest_gfxbuf_ptr->width - dest_x,
                   BS_MIN(src_gfxbuf_ptr->width - src_x, width));
    height = BS_MIN(dest_gfxbuf_ptr->height - dest_y,
                    BS_MIN(src_gfxbuf_ptr->height - src_y, height));

    uint32_t *src_data_ptr, *dest_data_ptr;
    for (unsigned y = 0; y < height; ++y) {
        dest_data_ptr = &dest_gfxbuf_ptr->data_ptr[
            (dest_y + y) * dest_gfxbuf_ptr->pixels_per_line + dest_x];
        src_data_ptr = &src_gfxbuf_ptr->data_ptr[
            (src_y + y) * src_gfxbuf_ptr->pixels_per_line + src_x];
        memcpy(dest_data_ptr, src_data_ptr, sizeof(uint32_t) * width);
    }
}

/* ------------------------------------------------------------------------- */
void bs_gfxbuf_argb8888_to_floats(
    const uint32_t argb8888,
    float *red_ptr,
    float *green_ptr,
    float *blue_ptr,
    float *alpha_ptr)
{
    *red_ptr = BS_MIN(1.0, ((argb8888 & 0xff0000) >> 0x10) / 255.0);
    *green_ptr = BS_MIN(1.0, ((argb8888 & 0x00ff00) >> 0x8) / 255.0);
    *blue_ptr = BS_MIN(1.0, ((argb8888 & 0x0000ff)) / 255.0);
    if (NULL != alpha_ptr) {
        *alpha_ptr = BS_MIN(1.0, ((argb8888 & 0xff000000) >> 0x18) / 255.0);
    }
}

/* ------------------------------------------------------------------------- */
#ifdef HAVE_CAIRO
cairo_t *bs_gfxbuf_create_cairo(const bs_gfxbuf_t *gfxbuf_ptr)
{
    cairo_surface_t *cairo_surface_ptr = cairo_image_surface_create_for_data(
        (unsigned char*)gfxbuf_ptr->data_ptr, bs_gfx_cairo_image_format,
        gfxbuf_ptr->width, gfxbuf_ptr->height,
        gfxbuf_ptr->pixels_per_line * sizeof(uint32_t));
    if (NULL == cairo_surface_ptr) {
        bs_log(
            BS_ERROR,
            "Failed cairo_image_surface_create_for_data(%p, %d, %u, %u, %zu)",
            gfxbuf_ptr->data_ptr, bs_gfx_cairo_image_format,
            gfxbuf_ptr->width, gfxbuf_ptr->height,
            gfxbuf_ptr->pixels_per_line * sizeof(uint32_t));
        return NULL;
    }
    cairo_t *cairo_ptr = cairo_create(cairo_surface_ptr);
    cairo_surface_destroy(cairo_surface_ptr);
    if (NULL == cairo_ptr) {
        bs_log(BS_ERROR, "Failed cairo_create(%p)", cairo_surface_ptr);
    }
    return cairo_ptr;
}

/* ------------------------------------------------------------------------- */
void bs_gfxbuf_cairo_set_source_argb8888(
    cairo_t *cairo_ptr,
    uint32_t argb8888)
{
    float r, g, b, alpha;
    bs_gfxbuf_argb8888_to_floats(argb8888, &r, &g, &b, &alpha);
    cairo_set_source_rgba(cairo_ptr, r, g, b, alpha);

}

/* ------------------------------------------------------------------------- */
/** TODO(kaeser@gubbe.ch): Change this to use libpng, and clean the code. */
void bs_test_gfxbuf_equals_png_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    int line,
    const bs_gfxbuf_t *gfxbuf_ptr,
    const char *png_fname_ptr)
{
    if (NULL == png_fname_ptr) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line, "PNG file name is NULL");
        return;
    }

    cairo_surface_t *png_surface_ptr =
        cairo_image_surface_create_from_png(png_fname_ptr);
    if (NULL == png_surface_ptr) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "Failed cairo_image_surface_create_from_png(\"%s\")",
            png_fname_ptr);
        return;
    }

    if (CAIRO_STATUS_SUCCESS != cairo_surface_status(png_surface_ptr)) {
        bs_test_fail_at(test_ptr, fname_ptr, line,
                        "Failed to load PNG surface from \"%s\"",
                        png_fname_ptr);
    }

    if ((unsigned)cairo_image_surface_get_width(png_surface_ptr) !=
        gfxbuf_ptr->width) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "gfxbuf width %u != PNG width %d",
            gfxbuf_ptr->width,
            cairo_image_surface_get_width(png_surface_ptr));
    }
    if ((unsigned)cairo_image_surface_get_height(png_surface_ptr) !=
        gfxbuf_ptr->height) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "gfxbuf height %u != PNG height %d",
            gfxbuf_ptr->height,
            cairo_image_surface_get_height(png_surface_ptr));
    }
    if ((unsigned)cairo_image_surface_get_stride(png_surface_ptr) <
        gfxbuf_ptr->width * sizeof(uint32_t)) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "PNG bytes per line (%d) lower than gfxbuf width.",
            cairo_image_surface_get_stride(png_surface_ptr));
    }

    if (!bs_test_failed(test_ptr)) {
        for (unsigned l = 0; l < gfxbuf_ptr->height; ++l) {
            if (0 != memcmp(
                    bs_gfxbuf_pixel_at(gfxbuf_ptr, 0, l),
                    cairo_image_surface_get_data(png_surface_ptr) + (
                        l * cairo_image_surface_get_stride(png_surface_ptr)),
                    gfxbuf_ptr->width * sizeof(uint32_t))) {
                bs_test_fail_at(
                    test_ptr, fname_ptr, line,
                    "gfxbuf content at line %u different from PNG.", line);
            }
        }
    }

    cairo_surface_destroy(png_surface_ptr);

    if (!bs_test_failed(test_ptr)) return;

    cairo_t *cairo_ptr = bs_gfxbuf_create_cairo(gfxbuf_ptr);
    if (NULL == cairo_ptr) {
        bs_log(BS_ERROR, "Failed bs_gfxbuf_create_cairo");
        return;
    }
    cairo_surface_t *surface_ptr = cairo_get_target(cairo_ptr);
    if (NULL == cairo_ptr) {
        bs_log(BS_ERROR, "Failed cairo_get_target");
        cairo_destroy(cairo_ptr);
        return;
    }

    cairo_status_t status = cairo_surface_write_to_png(surface_ptr, "/tmp/out.png");
    if (CAIRO_STATUS_SUCCESS == status) {
        bs_log(BS_ERROR, "PNG did not match. gfxbuf written to \"/tmp/out.png\"");
    } else {
        bs_log(BS_ERROR, "Failed cairo_surface_write_to_png(%p, \"/tmp/out.png\")",
               surface_ptr);
    }

    cairo_destroy(cairo_ptr);
}

#endif  // HAVE_CAIRO

/* == Tests ================================================================ */

static void test_copy_area(bs_test_t *test_ptr);
static void test_argb8888_to_floats(bs_test_t *test_ptr);
#ifdef HAVE_CAIRO
static void test_cairo(bs_test_t *test_ptr);
static void test_equals_png(bs_test_t *test_ptr);
#endif  // HAVE_CAIRO

const bs_test_case_t          bs_gfxbuf_test_cases[] = {
    { 1, "copy_area", test_copy_area },
    { 1, "argb8888_fo_floats", test_argb8888_to_floats },
#ifdef HAVE_CAIRO
    { 1, "cairo", test_cairo },
    { 1, "equals_png", test_equals_png },
#endif  // HAVE_CAIRO
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
void test_copy_area(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf1 = bs_gfxbuf_create(3, 3);
    bs_gfxbuf_clear(buf1, 0x10203040);
    bs_gfxbuf_t *buf2 = bs_gfxbuf_create(4, 4);

    BS_TEST_VERIFY_EQ(test_ptr, 0, *bs_gfxbuf_pixel_at(buf2, 1, 1));

    bs_gfxbuf_copy_area(buf2, 1, 1, buf1, 1, 1, 3, 3);
    BS_TEST_VERIFY_EQ(test_ptr, 0, *bs_gfxbuf_pixel_at(buf2, 0, 0));
    BS_TEST_VERIFY_EQ(test_ptr, 0X10203040, *bs_gfxbuf_pixel_at(buf2, 1, 1));
    BS_TEST_VERIFY_EQ(test_ptr, 0X10203040, *bs_gfxbuf_pixel_at(buf2, 2, 2));
    BS_TEST_VERIFY_EQ(test_ptr, 0, *bs_gfxbuf_pixel_at(buf2, 3, 3));

    bs_gfxbuf_destroy(buf2);
    bs_gfxbuf_destroy(buf1);
}

/* ------------------------------------------------------------------------- */
/** Verifies that cairo_util_argb8888_to_floats behaves properly */
void test_argb8888_to_floats(bs_test_t *test_ptr) {
    float r, g, b, alpha;
    bs_gfxbuf_argb8888_to_floats(0, &r, &g, &b, &alpha);
    BS_TEST_VERIFY_EQ(test_ptr, 0, r);
    BS_TEST_VERIFY_EQ(test_ptr, 0, g);
    BS_TEST_VERIFY_EQ(test_ptr, 0, b);
    BS_TEST_VERIFY_EQ(test_ptr, 0, alpha);

    bs_gfxbuf_argb8888_to_floats(0xffffffff, &r, &g, &b, &alpha);
    BS_TEST_VERIFY_EQ(test_ptr, 1.0, r);
    BS_TEST_VERIFY_EQ(test_ptr, 1.0, g);
    BS_TEST_VERIFY_EQ(test_ptr, 1.0, b);
    BS_TEST_VERIFY_EQ(test_ptr, 1.0, alpha);

    // Floating point - we're fine with a near-enough value.
    bs_gfxbuf_argb8888_to_floats(0xffc08040, &r, &g, &b, NULL);
    BS_TEST_VERIFY_TRUE(test_ptr, 1e-3 > fabs(r - 0.7529));
    BS_TEST_VERIFY_TRUE(test_ptr, 1e-3 > fabs(g - 0.5020));
    BS_TEST_VERIFY_TRUE(test_ptr, 1e-3 > fabs(b - 0.2510));
}

#ifdef HAVE_CAIRO
/* ------------------------------------------------------------------------- */
void test_cairo(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf = bs_gfxbuf_create(1, 1);
    cairo_t *cairo_ptr = bs_gfxbuf_create_cairo(buf);

    BS_TEST_VERIFY_NEQ(test_ptr, NULL, cairo_ptr);
    // Color is 0 after initialization.
    BS_TEST_VERIFY_EQ(test_ptr, 0, *bs_gfxbuf_pixel_at(buf, 0, 0));

    cairo_set_source_rgba(cairo_ptr, 1.0, 1.0, 1.0, 1.0);
    cairo_rectangle(cairo_ptr, 0, 0, 1, 1);
    cairo_fill(cairo_ptr);

    // Cairo should have filled it with white.
    BS_TEST_VERIFY_EQ(test_ptr, 0xffffffff, *bs_gfxbuf_pixel_at(buf, 0, 0));

    cairo_destroy(cairo_ptr);
    bs_gfxbuf_destroy(buf);
}

/* ------------------------------------------------------------------------- */
void test_equals_png(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf = bs_gfxbuf_create(1, 1);
    bs_gfxbuf_clear(buf, 0xff804020);
    bs_test_gfxbuf_equals_png_at(
        test_ptr, __FILE__, __LINE__, buf,
        bs_test_resolve_path("testdata/gfxbuf_equals.png"));
    bs_gfxbuf_destroy(buf);
}
#endif  // HAVE_CAIRO

/* == Benchmarks =========================================================== */

static void benchmark_clear(bs_test_t *test_ptr);
static void benchmark_clear_nonblack(bs_test_t *test_ptr);
static void benchmark_copy(bs_test_t *test_ptr);

/* set benchmarks to last for 2.5s each */
static const uint64_t benchmark_duration = 2500000;

const bs_test_case_t          bs_gfxbuf_benchmarks[] = {
    { 1, "benchmark-gfxbuf_clear-black", benchmark_clear },
    { 1, "benchmark-gfxbuf_clear-nonblack", benchmark_clear_nonblack },
    { 1, "benchmark-gfxbuf_copy", benchmark_copy },
    { 0, NULL, NULL }
};

/* ------------------------------------------------------------------------- */
static void benchmark_clear(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf_ptr = bs_gfxbuf_create(1024, 768);
    if (NULL == buf_ptr) {
        BS_TEST_FAIL(test_ptr, "Failed bs_gfxbuf_create(1024, 768)");
        return;
    }

    uint64_t usec = bs_usec();
    unsigned iterations = 0;
    while (usec + benchmark_duration >= bs_usec()) {
        bs_gfxbuf_clear(buf_ptr, 0);
        iterations++;
    }
    usec = bs_usec() - usec;

    bs_test_succeed(test_ptr, "bs_gfxbuf_clear: %.3e pix/sec - %"PRIu64"us",
                    (double)iterations * 1024 * 768 / (usec * 1e-6), usec);
    bs_gfxbuf_destroy(buf_ptr);
}

/* ------------------------------------------------------------------------- */
static void benchmark_clear_nonblack(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf_ptr = bs_gfxbuf_create(1024, 768);
    if (NULL == buf_ptr) {
        BS_TEST_FAIL(test_ptr, "Failed bs_gfxbuf_create(1024, 768)");
        return;
    }

    uint64_t usec = bs_usec();
    unsigned iterations = 0;
    while (usec + benchmark_duration >= bs_usec()) {
        bs_gfxbuf_clear(buf_ptr, 0x204080ff);
        iterations++;
    }
    usec = bs_usec() - usec;

    bs_test_succeed(test_ptr, "bs_gfxbuf_clear: %.3e pix/sec - %"PRIu64"us",
                    (double)iterations * 1024 * 768 / (usec * 1e-6), usec);
    bs_gfxbuf_destroy(buf_ptr);
}

/* ------------------------------------------------------------------------- */
static void benchmark_copy(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf_1_ptr = bs_gfxbuf_create(1024, 768);
    if (NULL == buf_1_ptr) {
        BS_TEST_FAIL(test_ptr, "Failed bs_gfxbuf_create(1024, 768)");
        return;
    }
    bs_gfxbuf_t *buf_2_ptr = bs_gfxbuf_create(1024, 768);
    if (NULL == buf_2_ptr) {
        BS_TEST_FAIL(test_ptr, "Failed bs_gfxbuf_create(1024, 768)");
        bs_gfxbuf_destroy(buf_1_ptr);
        return;
    }

    uint64_t usec = bs_usec();
    unsigned iterations = 0;
    while (usec + benchmark_duration >= bs_usec()) {
        bs_gfxbuf_copy(buf_2_ptr, buf_1_ptr);
        iterations++;
    }
    usec = bs_usec() - usec;

    bs_test_succeed(test_ptr, "bs_gfxbuf_copy: %.3e pix/sec",
                    (double)iterations * 1024 * 768 / (usec * 1e-6));
    bs_gfxbuf_destroy(buf_1_ptr);
    bs_gfxbuf_destroy(buf_2_ptr);
}

/* == End of gfxbuf.c ====================================================== */
