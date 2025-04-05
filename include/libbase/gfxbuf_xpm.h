/* ========================================================================= */
/**
 * @file gfxbuf_xpm.h
 * Implements a simple XPM loader for compiled-in XPM images.
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
#ifndef __GFXBUF_XPM_H__
#define __GFXBUF_XPM_H__

#include "libbase/gfxbuf.h"
#include "libbase/test.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Creates a @ref bs_gfxbuf_t from the XPM data at `xpm_data_ptr`.
 *
 * @param xpm_data_ptr
 *
 * @return A new @ref bs_gfxbuf_t, or NULL on error. Must be destroyed by
 *     calling @ref bs_gfxbuf_destroy.
 */
bs_gfxbuf_t *bs_gfxbuf_xpm_create_from_data(char **xpm_data_ptr);

/** Unit test cases. */
extern const bs_test_case_t bs_gfxbuf_xpm_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __GFXBUF_XPM_H__ */
/* == End of gfxbuf_xpm.h ================================================== */
