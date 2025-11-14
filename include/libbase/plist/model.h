/* ========================================================================= */
/**
 * @file model.h
 *
 * @copyright
 * Copyright 2024 Google LLC
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
#ifndef __BSPL_MODEL_H__
#define __BSPL_MODEL_H__

#include <libbase/libbase.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Forward declaration: Base object type. */
typedef struct _bspl_object_t bspl_object_t;
/** Forward declaration: A string. */
typedef struct _bspl_string_t bspl_string_t;
/** Forward declaration: A dict (key/value store for objects). */
typedef struct _bspl_dict_t bspl_dict_t;
/** Forward declaration: An array (sequential store for objects). */
typedef struct _bspl_array_t bspl_array_t;

/** Type of the object. */
typedef enum {
    BSPL_STRING,
    BSPL_DICT,
    BSPL_ARRAY
} bspl_type_t;

/**
 * Gets a reference to the object. Increases the reference count.
 *
 * The reference should be released by calling @ref bspl_object_unref.
 *
 * @param object_ptr
 *
 * @return Same as "object_ptr".
 */
bspl_object_t *bspl_object_ref(bspl_object_t *object_ptr);

/**
 * Removes reference to the object.
 *
 * Once no more references are pending, will call all into the virtual dtor of
 * the implementation.
 *
 * @param object_ptr
 */
void bspl_object_unref(bspl_object_t *object_ptr);

/**
 * Returns the type of `object_ptr`.
 *
 * @param object_ptr
 *
 * @return The type.
 */
bspl_type_t bspl_object_type(bspl_object_t *object_ptr);

/**
 * Writes the object into the buffer.
 *
 * @param object_ptr
 * @param dynbuf_ptr
 *
 * @return true on success.
 */
bool bspl_object_write(bspl_object_t *object_ptr,
                       bs_dynbuf_t *dynbuf_ptr);

/**
 * Writes the object into the buffer, with specified indentation & level.
 *
 * @param object_ptr
 * @param dynbuf_ptr
 * @param indent
 * @param level
 *
 * @return true on success.
 */
bool bspl_object_write_indented(
    bspl_object_t *object_ptr,
    bs_dynbuf_t *dynbuf_ptr,
    size_t indent,
    size_t level);

/**
 * Creates a string object.
 *
 * @param value_ptr
 *
 * @return The string object, or NULL on error or if `value_ptr` was NULL.
 */
bspl_string_t *bspl_string_create(const char *value_ptr);

/** Gets the superclass @ref bspl_object_t from the string. */
bspl_object_t *bspl_object_from_string(bspl_string_t *string_ptr);
/** Gets the @ref bspl_string_t for `object_ptr`. NULL if not a string. */
bspl_string_t *bspl_string_from_object(bspl_object_t *object_ptr);

/**
 * Returns value of the string object.
 *
 * Convenience wrapper for bspl_string_value(bspl_string_from_object(...)).
 *
 * @param object_ptr
 *
 * @return Pointer to the string object's value or NULL it not a string.
 */
const char *bspl_string_value_from_object(bspl_object_t *object_ptr);

/**
 * Returns the value of the string.
 *
 * @param string_ptr
 *
 * @return Pointer to the string's value.
 */
const char *bspl_string_value(const bspl_string_t *string_ptr);

/**
 * Creates a dict object.
 *
 * @return The dict object, or NULL on error.
 */
bspl_dict_t *bspl_dict_create(void);

/** @return the superclass @ref bspl_object_t of the dict. */
bspl_object_t *bspl_object_from_dict(bspl_dict_t *dict_ptr);
/** @return the @ref bspl_dict_t for `object_ptr`. NULL if not a dict. */
bspl_dict_t *bspl_dict_from_object(bspl_object_t *object_ptr);

/**
 * Adds an object to the dict.
 *
 * @param dict_ptr
 * @param key_ptr
 * @param object_ptr          The object to add. It will be duplicated by
 *                            calling @ref bspl_object_ref.
 *
 * @return true on success. Adding the object can fail if the key already
 *     exists, or if memory could not get allocated.
 */
bool bspl_dict_add(
    bspl_dict_t *dict_ptr,
    const char *key_ptr,
    bspl_object_t *object_ptr);

/** @return the given object from the dict. */
bspl_object_t *bspl_dict_get(
    bspl_dict_t *dict_ptr,
    const char *key_ptr);

/**
 * Executes `fn` for each key and object of the dict.
 *
 * @param dict_ptr
 * @param fn
 * @param userdata_ptr
 *
 * @return true if all calls to `fn` returned true. The iteration will be
 *     aborted on the first failed call to `fn`.
 */
bool bspl_dict_foreach(
    bspl_dict_t *dict_ptr,
    bool (*fn)(const char *key_ptr,
               bspl_object_t *object_ptr,
               void *userdata_ptr),
    void *userdata_ptr);

/**
 * Creates an array object.
 *
 * @return The array object, or NULL on error.
 */
bspl_array_t *bspl_array_create(void);

/** @return the superclass @ref bspl_object_t of the array. */
bspl_object_t *bspl_object_from_array(bspl_array_t *array_ptr);
/** @return the @ref bspl_array_t for `object_ptr`. NULL if not an array. */
bspl_array_t *bspl_array_from_object(bspl_object_t *object_ptr);

/**
 * Adds an object to the end of the array.
 *
 * @param array_ptr
 * @param object_ptr
 *
 * @return true on success. Adding the object can fail if the array does not
 *     have space and fails to grow.
 */
bool bspl_array_push_back(
    bspl_array_t *array_ptr,
    bspl_object_t *object_ptr);

/** @return Size of the array, ie. the number of contained objects. */
size_t bspl_array_size(bspl_array_t *array_ptr);

/**
 * Returns the object at the position `index` of the array.
 *
 * @param array_ptr
 * @param index
 *
 * @return Pointer to the object at the specified position in the array.
 *     Returns NULL if index is out of bounds.
 */
bspl_object_t *bspl_array_at(
    bspl_array_t *array_ptr,
    size_t index);

/* -- Static & inlined methods: Convenience wrappers ----------------------- */

/** Unreferences the string. Wraps to @ref bspl_object_unref. */
static inline void bspl_string_unref(bspl_string_t *string_ptr)
{
    bspl_object_unref(bspl_object_from_string(string_ptr));
}

/** Gets a reference to `dict_ptr`. */
static inline bspl_dict_t *bspl_dict_ref(bspl_dict_t *dict_ptr) {
    return bspl_dict_from_object(
        bspl_object_ref(bspl_object_from_dict(dict_ptr)));
}
/** Unreferences the dict. Wraps to @ref bspl_object_unref. */
static inline void bspl_dict_unref(bspl_dict_t *dict_ptr) {
    bspl_object_unref(bspl_object_from_dict(dict_ptr));
}

/** @return the dict value of the specified object, or NULL on error. */
static inline bspl_dict_t *bspl_dict_get_dict(
    bspl_dict_t *dict_ptr,
    const char *key_ptr) {
    return bspl_dict_from_object(bspl_dict_get(dict_ptr, key_ptr));
}

/** @return the array value of the specified object, or NULL on error. */
static inline bspl_array_t *bspl_dict_get_array(
    bspl_dict_t *dict_ptr,
    const char *key_ptr) {
    return bspl_array_from_object(bspl_dict_get(dict_ptr, key_ptr));
}
/** @return the string value of the specified object, or NULL on error. */
static inline const char *bspl_dict_get_string_value(
    bspl_dict_t *dict_ptr,
    const char *key_ptr) {
    return bspl_string_value(
        bspl_string_from_object(bspl_dict_get(dict_ptr, key_ptr)));
}

/** Unreferences the array. Wraps to @ref bspl_object_unref. */
static inline void bspl_array_unref(bspl_array_t *array_ptr) {
    bspl_object_unref(bspl_object_from_array(array_ptr));
}

/** @return the value of the string object at `idx`, or NULL on error. */
static inline const char *bspl_array_string_value_at(
    bspl_array_t *array_ptr, size_t idx) {
    return bspl_string_value(
        bspl_string_from_object(bspl_array_at(array_ptr, idx)));
}

/** Unit test set for the config data model. */
extern const bs_test_set_t bspl_model_test_set;

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __BSPL_MODEL_H__ */
/* == End of model.h ================================================== */
