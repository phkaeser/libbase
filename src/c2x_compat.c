/* ========================================================================= */
/**
 * @file c2x_compat.c
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

#include <libbase/c2x_compat.h>

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
char *strdup(const char *str) {
    size_t len = strlen(str);
    char *new_ptr = malloc(len + 1);
    if (NULL == new_ptr) return NULL;
    new_ptr[len] = '\0';
    memcpy(new_ptr, str, len);
    return new_ptr;
}

/* == End of c2x_compat.c ================================================== */
