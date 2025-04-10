/* ========================================================================= */
/**
 * @file gfxbuf.h
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
#ifndef __LIBBASE_GFXBUF_H__
#define __LIBBASE_GFXBUF_H__

#include <inttypes.h>
#include <libbase/assert.h>
#include <libbase/test.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif  // HAVE_CAIRO

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** A graphics buffer. */
typedef struct {
    /** Width, in pixels. */
    unsigned                  width;
    /** Height, in pixels. */
    unsigned                  height;
    /** Pixels per line. */
    unsigned                  pixels_per_line;
    /** The pixels buffer. Has `height` * `pixels_per_line` elements. */
    uint32_t                  *data_ptr;
} bs_gfxbuf_t;

/**
 * Creates a graphics buffer @ref bs_gfxbuf_t.
 *
 * @param width
 * @param height
 *
 * @return A pointer to @ref bs_gfxbuf_t. Must be free'd by
 *     @ref bs_gfxbuf_destroy().
 */
bs_gfxbuf_t *bs_gfxbuf_create(unsigned width, unsigned height);

/**
 * Creates a graphics buffer @ref bs_gfxbuf_t.
 *
 * Will use the existing pixels buffer at `data_ptr`, and expects it to remain
 * available throughout the lifetime of the created @ref bs_gfxbuf_t.
 *
 * @param width
 * @param height
 * @param pixels_per_line
 * @param data_ptr
 */
bs_gfxbuf_t *bs_gfxbuf_create_unmanaged(unsigned width, unsigned height,
                                        unsigned pixels_per_line,
                                        uint32_t* data_ptr);

/**
 * Destroys the graphics buffer.
 *
 * @param gfxbuf_ptr
 */
void bs_gfxbuf_destroy(bs_gfxbuf_t *gfxbuf_ptr);

/**
 * Clears the graphics buffer with the specified `color`.
 *
 * @param gfxbuf_ptr
 * @param color               The fill color, as an ARGB 8888.
 */
void bs_gfxbuf_clear(bs_gfxbuf_t *gfxbuf_ptr, const uint32_t color);

/**
 * Copies the contents of `src_gfxbuf_ptr` to `dest_gfxbuf_ptr`.
 *
 * Expects width and height of `src_gfxbuf_ptr` and `dest_gfxbuf_ptr` the same.
 *
 * @param src_gfxbuf_ptr
 * @param dest_gfxbuf_ptr
 */
void bs_gfxbuf_copy(bs_gfxbuf_t *dest_gfxbuf_ptr,
                    const bs_gfxbuf_t *src_gfxbuf_ptr);

/**
 * Copies a rectangular area between graphics buffers.
 *
 * The width and height of the rectangular area will be truncated to fit both
 * source and destination buffers.
 *
 * @param dest_gfxbuf_ptr     Destination graphics buffer.
 * @param dest_x              Destination coordinate.
 * @param dest_y              Destination coordinate.
 * @param src_gfxbuf_ptr      Source graphics buffere.
 * @param src_x               Source coordinate.
 * @param src_y               Source coordinate.
 * @param width               Width of the rectangle to copy.
 * @param height              Height of the rectangle to copy.
 */
void bs_gfxbuf_copy_area(
    bs_gfxbuf_t *dest_gfxbuf_ptr,
    unsigned dest_x,
    unsigned dest_y,
    const bs_gfxbuf_t *src_gfxbuf_ptr,
    unsigned src_x,
    unsigned src_y,
    unsigned width,
    unsigned height);

/**
 * Returns a pointer to the pixel at the given coordinates.
 *
 * @param gfxbuf_ptr
 * @param x
 * @param y
 *
 * @return A pointer to the pixel at the specified position.
 */
static inline uint32_t *bs_gfxbuf_pixel_at(
    const bs_gfxbuf_t *gfxbuf_ptr,
    unsigned x,
    unsigned y) {
    BS_ASSERT(x < gfxbuf_ptr->width);
    BS_ASSERT(y < gfxbuf_ptr->height);
    return &gfxbuf_ptr->data_ptr[y * gfxbuf_ptr->pixels_per_line + x];
}

/**
 * Colors the pixel at the given coordinates.
 *
 * @param gfxbuf_ptr
 * @param x
 * @param y
 * @param color               Color, in ARGB8888 format.
 */
static inline void bs_gfxbuf_set_pixel(
    bs_gfxbuf_t *gfxbuf_ptr,
    unsigned x,
    unsigned y,
    uint32_t color) {
    *bs_gfxbuf_pixel_at(gfxbuf_ptr, x, y) = color;
}

/**
 * Computes the red, green, blue and alpha components as floating points from
 * the given `argb8888` value.
 *
 * @param argb8888            Color, in ARGB 8888 format.
 * @param red_ptr             Output value, will be clamped to [0, 1].
 * @param blue_ptr            Output value, will be clamped to [0, 1].
 * @param green_ptr           Output value, will be clamped to [0, 1].
 * @param alpha_ptr           Output value, will be clamped to [0, 1].
 */
void bs_gfxbuf_argb8888_to_floats(
    const uint32_t argb8888,
    float *red_ptr,
    float *green_ptr,
    float *blue_ptr,
    float *alpha_ptr);

#ifdef HAVE_CAIRO
/**
 * Creates a Cairo drawing context for the @ref bs_gfxbuf_t.
 *
 * This is a convenience function that permits drawing into the graphics buffer
 * using the Cairo 2D graphics library (https://cairographics.org/).
 *
 * @param gfxbuf_ptr          Graphics buffer. Must outlive the returned
 *                            `cairo_t`.
 *
 * @return A cairo drawing context, or NULL on error. The returned context must
 *     be destroyed by calling cairo_destroy().
 */
cairo_t *cairo_create_from_bs_gfxbuf(const bs_gfxbuf_t *gfxbuf_ptr);

/**
 * Sets the source color for the cairo at `cairo_ptr`.
 *
 * @param cairo_ptr
 * @param argb8888            Color, in ARGB 8888 format.
 */
void cairo_set_source_argb8888(
    cairo_t *cairo_ptr,
    uint32_t argb8888);

/**
 * Tests whether the graphics is equal to the contents of the PNG file.
 *
 * @param test_ptr
 * @paran fname_ptr
 * @paran line
 * @param gfxbuf_ptr
 * @param png_fname_ptr       Name of the PNG file. The test will report as
 *                            failed if `png_fname_ptr` is NULL.
 */
void bs_test_gfxbuf_equals_png_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    int line,
    const bs_gfxbuf_t *gfxbuf_ptr,
    const char *png_fname_ptr);

/**
 * Tests whether @ref bs_gfxbuf_t equals the PNG file.
 *
 * @param _test               The @ref bs_test_t of the current test case.
 * @param _gfxbuf_ptr         A @ref bs_gfxbuf_t, the graphics buffer to test.
 * @param _png_name           Path to the PNG file name. Relative paths will be
 *                            resolved using @ref bs_test_resolve_path.
 */
#define BS_TEST_VERIFY_GFXBUF_EQUALS_PNG(_test, _gfxbuf_ptr, _png_name) { \
        bs_test_gfxbuf_equals_png_at(                                   \
            (_test), __FILE__, __LINE__, (_gfxbuf_ptr),                 \
            bs_test_resolve_path(_png_name));                           \
    }

#endif  // HAVE_CAIRO

/** Unit tests. */
extern const bs_test_case_t   bs_gfxbuf_test_cases[];

/** Benchmarks, in the form of an unit test. */
extern const bs_test_case_t   bs_gfxbuf_benchmarks[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_GFXBUF_H__ */
/* == End of gfxbuf.h ================================================== */
