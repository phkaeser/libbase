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
#ifdef HAVE_CAIRO
cairo_t *bs_gfxbuf_create_cairo(bs_gfxbuf_t *gfxbuf_ptr)
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
#endif  // HAVE_CAIRO

/* == Tests ================================================================ */

static void benchmark_clear(bs_test_t *test_ptr);
static void benchmark_clear_nonblack(bs_test_t *test_ptr);
static void benchmark_copy(bs_test_t *test_ptr);
static void test_copy_area(bs_test_t *test_ptr);
#ifdef HAVE_CAIRO
static void test_cairo(bs_test_t *test_ptr);
#endif  // HAVE_CAIRO

/* set benchmarks to last for 2.5s each */
static const uint64_t benchmark_duration = 2500000;

const bs_test_case_t          bs_gfxbuf_test_cases[] = {
    { 1, "benchmark-gfxbuf_clear-black", benchmark_clear },
    { 1, "benchmark-gfxbuf_clear-nonblack", benchmark_clear_nonblack },
    { 1, "benchmark-gfxbuf_copy", benchmark_copy },
    { 1, "copy_area", test_copy_area },
#ifdef HAVE_CAIRO
    { 1, "cairo", test_cairo },
#endif  // HAVE_CAIRO
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
}

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
#endif  // HAVE_CAIRO

/* == End of gfxbuf.c ====================================================== */
