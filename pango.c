/* ========================================================================= */
/**
 * @file pango.c
 * Copyright (c) 2025 by Philipp Kaeser <kaeser@gubbe.ch>
 */

#include "libbase.h"

#include <pango/pangocairo.h>

void draw_text(bs_gfxbuf_t *gfxbuf_ptr, cairo_t *cairo_ptr)
{
    PangoLayout *layout_ptr;

    layout_ptr = pango_cairo_create_layout(cairo_ptr);

    pango_layout_set_text(layout_ptr, "Example Text gg", -1);
    PangoFontDescription *fd_ptr = pango_font_description_from_string("Classic Console 30");  // Sans Bold 30");
    pango_layout_set_font_description(layout_ptr, fd_ptr);
    pango_font_description_free(fd_ptr);

    pango_layout_set_ellipsize(layout_ptr, PANGO_ELLIPSIZE_END);
    // Huh, that's enough for "Ex..." -- -> pixels in scale
    pango_layout_set_width(layout_ptr, 245 * PANGO_SCALE);
    pango_layout_set_height(layout_ptr, -1);   // 1 line.

    // 30 * 1024.0 ?
    bs_log(BS_WARNING, "fd Absolute? %d", pango_font_description_get_size_is_absolute(fd_ptr));
    bs_log(BS_WARNING, "fd Size: %d", pango_font_description_get_size(fd_ptr));

    bs_log(BS_WARNING, "layout width %d", pango_layout_get_width(layout_ptr));
    bs_log(BS_WARNING, "layout height %d", pango_layout_get_height(layout_ptr));

    PangoRectangle ink, logical;
    pango_layout_get_pixel_extents(layout_ptr, &ink, &logical);

    bs_log(BS_WARNING, "layout ink: %d, %d (%d x %d)",
           ink.x, ink.y, ink.width, ink.height);
    bs_log(BS_WARNING, "layout logical: %d, %d (%d x %d)",
           logical.x, logical.y, logical.width, logical.height);

    // 'logical' is the box we can use for scaling/positioning.

    // 'ink' is dynamic and boxes the effectively-drawn area.
    // eg. includes super- or underscript elements
    for (int x = ink.x; x < ink.x + ink.width; ++x) {
        for (int y = ink.y; y < ink.y + ink.height; ++y) {
            bs_gfxbuf_set_pixel(gfxbuf_ptr, x, y, 0xff102040);
        }
    }



    cairo_set_source_rgb(cairo_ptr, 1.0, 1.0, 1.0);
    cairo_move_to(cairo_ptr, logical.x, logical.y);
    pango_cairo_show_layout(cairo_ptr, layout_ptr);


    g_object_unref(layout_ptr);
}

int main(void)
{
    bs_gfxbuf_t *gfxbuf_ptr = bs_gfxbuf_create(1024, 768);
    if (NULL == gfxbuf_ptr) return EXIT_FAILURE;
    bs_gfxbuf_clear(gfxbuf_ptr, 0xff0000c0);


    // FreeType? PangoFontMap *fm_ptr = pango_ft2_font_map_new();
//     PangoFontFamily *f_ptr;
//    int n;

    PangoFontMap *fm_cairo_ptr = pango_cairo_font_map_new();

    PangoFontFamily **family_ptr = NULL;
    int n = 0;
    pango_font_map_list_families(fm_cairo_ptr, &family_ptr, &n);
    bs_log(BS_WARNING, "pango_font_map_list_families gave %d", n);
    for (int i = 0; i < n; ++i) {
        bs_log(BS_WARNING, "Family %d: %s",
               i, pango_font_family_get_name(family_ptr[i]));
    }

    g_free(family_ptr);


    g_object_unref(fm_cairo_ptr);

    cairo_t *cairo_ptr = cairo_create_from_bs_gfxbuf(gfxbuf_ptr);
    if (NULL == cairo_ptr) return EXIT_FAILURE;

    draw_text(gfxbuf_ptr, cairo_ptr);

    cairo_surface_t *surface_ptr = cairo_get_target(cairo_ptr);
    cairo_status_t status = cairo_surface_write_to_png(surface_ptr, "/tmp/out.png");
    if (CAIRO_STATUS_SUCCESS != status) {
        bs_log(BS_ERROR, "Failed cairo_surface_write_to_png(%p, \"/tmp/out.png\")",
               surface_ptr);
        return EXIT_FAILURE;
    }
    cairo_destroy(cairo_ptr);
    bs_gfxbuf_destroy(gfxbuf_ptr);

    return EXIT_SUCCESS;
}

/* == End of pango.c ======================================================= */
