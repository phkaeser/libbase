/* ========================================================================= */
/**
 * @file plist.h
 *
 * Basic C library to parse Property List files, stored in text format.
 *
 * See https://en.wikipedia.org/wiki/Property_list for details. This library
 * supports basic types (String, Array, Dictionnary) only -- the Date and
 * Data types are (currently) missing.
 *
 * @copyright
 * Copyright 2025 Google LLC
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
#ifndef __LIBBASE_PLIST_H__
#define __LIBBASE_PLIST_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// IWYU pragma: begin_exports
#include "plist/decode.h"
#include "plist/model.h"
#include "plist/parse.h"
// IWYU pragma: end_exports

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_PLIST_H__ */
/* == End of plist.h ================================================== */
