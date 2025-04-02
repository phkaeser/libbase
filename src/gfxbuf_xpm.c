/* ========================================================================= */
/**
 * @file gfxbuf_xpm.c
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

#include <libbase/gfxbuf_xpm.h>

#include <ctype.h>

#include <libbase/avltree.h>
#include <libbase/strutil.h>

/* == Declarations ========================================================= */

/** Node for storing & looking up colors from the XPM. */
typedef struct {
    /** Tree node. */
    bs_avltree_node_t         node;
    /** The characters that define this color. */
    char                      *pixel_chars_ptr;
    /** Number of caracters that make up the pixel. */
    unsigned                  chars_per_pixel;
    /** Corresponding color, in ARGB8888. */
    uint32_t                  color;
} bs_gfxbuf_xpm_color_node_t;

static bool _bs_gfxbuf_xpm_copy_data(
    bs_gfxbuf_t *gfxbuf_ptr,
    char **xpm_data_ptr,
    unsigned dest_x,
    unsigned dest_y);

static bool _bs_gfxbuf_xpm_parse_header_line(
    const char *header_line_ptr,
    unsigned *width_ptr,
    unsigned *height_ptr,
    unsigned *colors_ptr,
    unsigned *chars_per_pixel_ptr);
static bool _bs_gfxbuf_xpm_parse_color_into_node(
    bs_gfxbuf_xpm_color_node_t *node_ptr,
    unsigned chars_per_pixel,
    const char* color_line_ptr);

static bs_gfxbuf_xpm_color_node_t *_bs_gfxbuf_xpm_color_node_create(
    unsigned chars_per_pixel);
static void _bs_gfxbuf_xpm_color_node_destroy(
    bs_avltree_node_t *node_ptr);
static int _bs_gfxbuf_xpm_color_node_cmp(
    const bs_avltree_node_t *node_ptr,
    const void *key_ptr);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bs_gfxbuf_t *bs_gfxbuf_xpm_create_from_data(char **xpm_data_ptr)
{
    unsigned width, height, colors, chars_per_pixel;
    if (!_bs_gfxbuf_xpm_parse_header_line(
            *xpm_data_ptr, &width, &height, &colors, &chars_per_pixel)) {
        return NULL;
    }

    bs_gfxbuf_t *gfxbuf_ptr = bs_gfxbuf_create(width, height);
    if (_bs_gfxbuf_xpm_copy_data(gfxbuf_ptr, xpm_data_ptr, 0, 0)) {
        return gfxbuf_ptr;
    }
    bs_gfxbuf_destroy(gfxbuf_ptr);
    return NULL;
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/**
 * Loads an XPM from included data at `xpm_data_ptr` to the position
 * `dest_x`, `dest_y` into `gfxbuf_ptr`. Will not overwrite pixels where the
 * XPM is transparent (color: None).
 *
 * @param gfxbuf_ptr
 * @param xpm_data_ptr
 * @param dest_x
 * @param dest_y
 *
 * @return true on success.
 */
bool _bs_gfxbuf_xpm_copy_data(
    bs_gfxbuf_t *gfxbuf_ptr,
    char **xpm_data_ptr,
    unsigned dest_x,
    unsigned dest_y)
{
    unsigned width, height, colors, chars_per_pixel;
    bs_gfxbuf_xpm_color_node_t *color_node_ptr;

    // Guard clause.
    if (NULL == gfxbuf_ptr) return false;

    if (!_bs_gfxbuf_xpm_parse_header_line(
            *xpm_data_ptr, &width, &height, &colors, &chars_per_pixel)) {
        return false;
    }

    // Prevent the XPM from spilling beyond the buffer.
    if (width + dest_x > gfxbuf_ptr->width) {
        width = gfxbuf_ptr->width - dest_x;
    }
    if (height + dest_y > gfxbuf_ptr->height) {
        height = gfxbuf_ptr->height - dest_y;
    }
    xpm_data_ptr++;

    // Parse the colors and store htem in the lookup tree.
    bs_avltree_t *tree_ptr = bs_avltree_create(
        _bs_gfxbuf_xpm_color_node_cmp, _bs_gfxbuf_xpm_color_node_destroy);
    if (NULL == tree_ptr) {
        bs_log(BS_ERROR, "Failed bs_avltree_create");
        return false;
    }
    for (unsigned color = 0; color < colors; ++color, ++xpm_data_ptr) {
        color_node_ptr = _bs_gfxbuf_xpm_color_node_create(chars_per_pixel);
        if (NULL != color_node_ptr) {
            if (_bs_gfxbuf_xpm_parse_color_into_node(
                    color_node_ptr, chars_per_pixel, *xpm_data_ptr)) {
                if (bs_avltree_insert(
                        tree_ptr,
                        color_node_ptr->pixel_chars_ptr,
                        &color_node_ptr->node,
                        false)) {
                    continue;
                }
                bs_log(BS_ERROR, "Color \"%s\" already exists", *xpm_data_ptr);
            }
            _bs_gfxbuf_xpm_color_node_destroy(&color_node_ptr->node);
        }
        bs_avltree_destroy(tree_ptr);
        return false;
    }

    // Now, parse the XPM pixels.
    bool outcome = true;
    for (unsigned y = 0; y < height; ++y, ++xpm_data_ptr) {
        if (width * chars_per_pixel > strlen(*xpm_data_ptr)) {
            bs_log(BS_ERROR, "Shorter than %u chars: \"%s\"",
                   width * chars_per_pixel, *xpm_data_ptr);
            outcome = false;
            break;
        }
        for (unsigned x = 0; x < width; ++x) {
            bs_gfxbuf_xpm_color_node_t *color_node_ptr =
                (bs_gfxbuf_xpm_color_node_t*)bs_avltree_lookup(
                    tree_ptr,
                    *xpm_data_ptr + x * chars_per_pixel);
            // Ignore transparent pixels.
            if (color_node_ptr->color != 0x00000000) {
                bs_gfxbuf_set_pixel(
                    gfxbuf_ptr,
                    dest_x + x, dest_y + y,
                    color_node_ptr->color);
            }
        }
    }

    bs_avltree_destroy(tree_ptr);
    return outcome;
}

/* ------------------------------------------------------------------------- */
/** Parses the XPM header line. */
bool _bs_gfxbuf_xpm_parse_header_line(
    const char *header_line_ptr,
    unsigned *width_ptr,
    unsigned *height_ptr,
    unsigned *colors_ptr,
    unsigned *chars_per_pixel_ptr)
{
    int                       scanned_values;

    scanned_values = sscanf(
        header_line_ptr, "%u %u %u %u",
        width_ptr, height_ptr, colors_ptr, chars_per_pixel_ptr);
    if (0 > scanned_values) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed sscanf(%s, \"%%u %%u %%u %%u\")",
               header_line_ptr);
        return false;
    }
    if (4 != scanned_values) {
        bs_log(BS_ERROR, "Failed to match & assign 4 input values for "
               "sscanf(%s, \"%%u %%u %%u %%u\")", header_line_ptr);
        return false;
    }

    return true;
}

/* ------------------------------------------------------------------------- */
/**
 * Parses an XPM color line into `color_node_ptr`.
 *
 * It has the format <characters> (<representation> <color>)+.
 */
bool _bs_gfxbuf_xpm_parse_color_into_node(
    bs_gfxbuf_xpm_color_node_t *color_node_ptr,
    unsigned chars_per_pixel,
    const char* color_line_ptr)
{
    // Sanity check: String must at least hold the color.
    if (chars_per_pixel > strlen(color_line_ptr)) {
        bs_log(BS_ERROR, "shorter than %u chars: \"%s\"",
               chars_per_pixel, color_line_ptr);
        return false;
    }

    // Copy the <character> component.
    BS_ASSERT(color_node_ptr->chars_per_pixel == chars_per_pixel);
    memcpy(color_node_ptr->pixel_chars_ptr, color_line_ptr, chars_per_pixel);

    // Then, skip over it and over succeeding whitespace.
    color_line_ptr += chars_per_pixel;
    if (!isspace(*color_line_ptr)) {
        bs_log(BS_ERROR, "Whitespace missing after <characters>");
        return false;
    }
    while (*color_line_ptr && isspace(*color_line_ptr)) color_line_ptr++;

    // Check <representation>. We only support 'c' for "color".
    if ('c' != *color_line_ptr) {
        bs_log(BS_ERROR, "Unsupported color representation: \"%s\"",
               color_line_ptr);
        return false;
    }

    // Again, skip over it and over succeeding whitespace.
    color_line_ptr++;
    while (*color_line_ptr && isspace(*color_line_ptr)) color_line_ptr++;

    // The <color>. If it's "None", it means the pixel is transparent.
    if (0 == strncmp(color_line_ptr, "None", 4)) {
        color_node_ptr->color = 0;
        return true;
    }

    // If the <color> start with '#', it is a RGB representation.
    if (*color_line_ptr == '#') {
        size_t alnum_count = 0;
        while (isalnum(color_line_ptr[1 + alnum_count])) ++alnum_count;
        if (6 != alnum_count) {
            bs_log(BS_ERROR, "Not a #RRGGBB representation: %s",
                   color_line_ptr + 1);
            return false;
        }

        uint64_t tmp_value;
        if (!bs_strconvert_uint64(color_line_ptr + 1, &tmp_value, 16)) {
            return false;
        }
        if (tmp_value > 0xffffff) {
            bs_log(BS_ERROR, "Color value out of range: 0x%"PRIx64,
                   tmp_value);
            return false;
        }
        color_node_ptr->color = tmp_value | 0xff000000;

        return true;
    }

    bs_log(BS_ERROR, "Unsupported color encoding: %s", color_line_ptr + 1);
    return false;
}

/* ------------------------------------------------------------------------- */
bs_gfxbuf_xpm_color_node_t *_bs_gfxbuf_xpm_color_node_create(
    unsigned chars_per_pixel)
{
    bs_gfxbuf_xpm_color_node_t *color_node_ptr = calloc(
        1, sizeof(bs_gfxbuf_xpm_color_node_t));
    if (NULL == color_node_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed calloc(1, %zu)",
               sizeof(bs_gfxbuf_xpm_color_node_t));
        return NULL;
    }

    color_node_ptr->pixel_chars_ptr = calloc(1, chars_per_pixel);
    if (NULL == color_node_ptr->pixel_chars_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed calloc(1, %u)", chars_per_pixel);
        _bs_gfxbuf_xpm_color_node_destroy(&color_node_ptr->node);
        return NULL;
    }
    color_node_ptr->chars_per_pixel = chars_per_pixel;
    return color_node_ptr;
}

/* ------------------------------------------------------------------------- */
void _bs_gfxbuf_xpm_color_node_destroy(bs_avltree_node_t *node_ptr)
{
    bs_gfxbuf_xpm_color_node_t *color_node_ptr =
        (bs_gfxbuf_xpm_color_node_t*)node_ptr;
    if (NULL != color_node_ptr->pixel_chars_ptr) {
        free(color_node_ptr->pixel_chars_ptr);
        color_node_ptr->pixel_chars_ptr = NULL;
    }
    free(color_node_ptr);
}

/* ------------------------------------------------------------------------- */
int _bs_gfxbuf_xpm_color_node_cmp(const bs_avltree_node_t *node_ptr,
                                  const void *key_ptr) {
    bs_gfxbuf_xpm_color_node_t *color_node_ptr =
        (bs_gfxbuf_xpm_color_node_t*)node_ptr;
    return memcmp(color_node_ptr->pixel_chars_ptr,
                  (const char*)key_ptr,
                  color_node_ptr->chars_per_pixel);
}

/* == Unit tests =========================================================== */

static void test_parse_color(bs_test_t *test_ptr);
static void test_parse_xpm(bs_test_t *test_ptr);
static void test_create_xpm(bs_test_t *test_ptr);

const bs_test_case_t          bs_gfxbuf_xpm_test_cases[] = {
    { 1, "parse_color", test_parse_color },
    { 1, "parse_xpm", test_parse_xpm },
    { 1, "create_xpm", test_create_xpm },
    { 0, NULL, NULL }
};

static char *test_xpm_data[] = {
    "2 2 3 1",
    "  c None",
    ". c #0000ff",
    "+ c #000000",
    ".+",
    "+ "};

/* ------------------------------------------------------------------------- */
void test_parse_color(bs_test_t *test_ptr)
{
    bs_gfxbuf_xpm_color_node_t cnode;
    char pixel_chars[2];

    cnode.pixel_chars_ptr = &pixel_chars[0];
    cnode.chars_per_pixel = 2;

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "xy c #123456"));
    BS_TEST_VERIFY_EQ(test_ptr, 0xff123456, cnode.color);
    BS_TEST_VERIFY_EQ(test_ptr, 'x', cnode.pixel_chars_ptr[0]);
    BS_TEST_VERIFY_EQ(test_ptr, 'y', cnode.pixel_chars_ptr[1]);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "xy c #ffffff"));
    BS_TEST_VERIFY_EQ(test_ptr, 0xffffffff, cnode.color);
    BS_TEST_VERIFY_EQ(test_ptr, 'x', cnode.pixel_chars_ptr[0]);
    BS_TEST_VERIFY_EQ(test_ptr, 'y', cnode.pixel_chars_ptr[1]);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "ab c None"));
    BS_TEST_VERIFY_EQ(test_ptr, cnode.color, 0x00000000);
    BS_TEST_VERIFY_EQ(test_ptr, 'a', cnode.pixel_chars_ptr[0]);
    BS_TEST_VERIFY_EQ(test_ptr, 'b', cnode.pixel_chars_ptr[1]);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "a  c None"));
    BS_TEST_VERIFY_EQ(test_ptr, cnode.color, 0x00000000);
    BS_TEST_VERIFY_EQ(test_ptr, 'a', cnode.pixel_chars_ptr[0]);
    BS_TEST_VERIFY_EQ(test_ptr, ' ', cnode.pixel_chars_ptr[1]);

    BS_TEST_VERIFY_FALSE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "t c None"));
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "abc c None"));
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "ab c #12345"));
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "ab c #1234567"));
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        _bs_gfxbuf_xpm_parse_color_into_node(&cnode, 2, "ab c #12xx56"));
}

/* ------------------------------------------------------------------------- */
void test_parse_xpm(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf_ptr = bs_gfxbuf_create(3, 3);
    bs_gfxbuf_clear(buf_ptr, 42);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bs_gfxbuf_xpm_copy_data(buf_ptr, test_xpm_data, 1, 1));

    BS_TEST_VERIFY_EQ(test_ptr, 42, *bs_gfxbuf_pixel_at(buf_ptr, 0, 0));
    BS_TEST_VERIFY_EQ(test_ptr, 42, *bs_gfxbuf_pixel_at(buf_ptr, 1, 0));
    BS_TEST_VERIFY_EQ(test_ptr, 42, *bs_gfxbuf_pixel_at(buf_ptr, 2, 0));

    BS_TEST_VERIFY_EQ(test_ptr, 42, *bs_gfxbuf_pixel_at(buf_ptr, 0, 1));
    BS_TEST_VERIFY_EQ(test_ptr, 0xff0000ff, *bs_gfxbuf_pixel_at(buf_ptr, 1, 1));
    BS_TEST_VERIFY_EQ(test_ptr, 0xff000000, *bs_gfxbuf_pixel_at(buf_ptr, 2, 1));

    BS_TEST_VERIFY_EQ(test_ptr, 42, *bs_gfxbuf_pixel_at(buf_ptr, 0, 2));
    BS_TEST_VERIFY_EQ(test_ptr, 0xff000000, *bs_gfxbuf_pixel_at(buf_ptr, 1, 2));
    BS_TEST_VERIFY_EQ(test_ptr, 42, *bs_gfxbuf_pixel_at(buf_ptr, 2, 2));

    bs_gfxbuf_destroy(buf_ptr);
}

/* ------------------------------------------------------------------------- */
void test_create_xpm(bs_test_t *test_ptr)
{
    bs_gfxbuf_t *buf_ptr;

    buf_ptr = bs_gfxbuf_xpm_create_from_data(test_xpm_data);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, buf_ptr);
    if (buf_ptr != NULL) {
        BS_TEST_VERIFY_EQ(test_ptr, 2, buf_ptr->width);
        BS_TEST_VERIFY_EQ(test_ptr, 2, buf_ptr->height);
        bs_gfxbuf_destroy(buf_ptr);
    }
}

/* == End of gfxbuf_xpm.c ================================================== */
