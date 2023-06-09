/* ========================================================================= */
/**
 * @file c2x_compat.h
 * Compatibility layer for upcoming convenient C2x methods.
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
#ifndef __LIBBASE_C2X_COMPAT_H__
#define __LIBBASE_C2X_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Returns a pointer to a new string which is a duplicate of `s`.
 *
 * @param s
 *
 * @return A pointer to the duplicated string, to be free'd with free(3).
 */
char *strdup(const char*s);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_C2X_COMPAT_H__ */
/* == End of c2x_compat.h ================================================== */
