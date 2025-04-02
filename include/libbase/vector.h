/* ========================================================================= */
/**
 * @file vector.h
 * Methods and definitions to work a bit more conveniently with vectors in C.
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
#ifndef __LIBBASE_VECTOR_H__
#define __LIBBASE_VECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Two-dimension vector with floating point dimensions. */
typedef struct {
    /** X dimension. */
    double                    x;
    /** Y dimension. */
    double                    y;
} bs_vector_2f_t;

/** Initializer for the 2-dimensional vector with floating points. */
#define BS_VECTOR_2F(_x, _y) ((bs_vector_2f_t){ .x = _x, .y = _y })

/**
 * Adds two vectors.
 *
 * @param v1
 * @param v2
 *
 * @return v1 + v2.
 */
static inline bs_vector_2f_t bs_vec_add_2f(const bs_vector_2f_t v1,
                                           const bs_vector_2f_t v2)
{
    return BS_VECTOR_2F(v1.x + v2.x, v1.y + v2.y);
}

/**
 * Scalar multiplication of a vector.
 *
 * @param scale
 * @param v
 *
 * @return scale * v.
 */
static inline bs_vector_2f_t bs_vec_mul_2f(const double scale,
                                           const bs_vector_2f_t v)
{
    return BS_VECTOR_2F(scale * v.x, scale * v.y);
}

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_VECTOR_H__ */
/* == End of vector.h ====================================================== */
