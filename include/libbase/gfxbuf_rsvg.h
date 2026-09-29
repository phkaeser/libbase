/* ========================================================================= */
/**
 * @file gfxbuf_rsvg.h
 * Implements a wrapper to librsvg 2.0 for loading SVG into a @ref bs_gfxbuf_t.
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
#ifndef __GFXBUF_RSVG_H__
#define __GFXBUF_RSVG_H__

#include <libbase/libbase.h>
#include <librsvg/rsvg.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Renders a SVG from a librsvg 2.0 handle into a @ref bs_gfxbuf_t.
 *
 * @param gfxbuf_ptr
 * @param rsvg_handle_ptr
 *
 * @return true on success.
 */
bool bs_gfxbuf_render_rsvg(
    bs_gfxbuf_t *gfxbuf_ptr,
    RsvgHandle *rsvg_handle_ptr);

/** Unite test set. */
extern const bs_test_set_t bs_gfxbuf_rsvg_test_set;

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // __GFXBUF_RSVG_H__
/* == End of gfxbuf_rsvg.h ================================================= */
