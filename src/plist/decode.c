/* ========================================================================= */
/**
 * @file decode.c
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

#include <inttypes.h>
#include <libbase/libbase.h>
#include <libbase/plist.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

/* == Declarations ========================================================= */

/** A pointer of type `value_type`, at `offset` behind `base_ptr`. */
#define BS_VALUE_AT(_value_type, _base_ptr, _offset)    \
    ((_value_type*)((uint8_t*)(_base_ptr) + (_offset)))

static bool _bspl_init_defaults(
    const bspl_desc_t *desc_ptr,
    void *dest_ptr);
static bool _bspl_decode_dict_without_init(
    bspl_dict_t *dict_ptr,
    const bspl_desc_t *desc_ptr,
    void *dest_ptr);

static bool _bspl_decode_uint64(
    bspl_object_t *obj_ptr,
    uint64_t *uint64_ptr);
static bool _bspl_decode_int64(
    bspl_object_t *obj_ptr,
    int64_t *int64_ptr);
static bool _bspl_decode_double(
    bspl_object_t *obj_ptr,
    double *double_ptr);
static bool _bspl_decode_argb32(
    bspl_object_t *obj_ptr,
    uint32_t *argb32_ptr);
static bool _bspl_decode_bool(
    bspl_object_t *obj_ptr,
    bool *bool_ptr);
static bool _bspl_decode_enum(
    bspl_object_t *obj_ptr,
    const bspl_enum_desc_t *enum_desc_ptr,
    int *enum_value_ptr);
static bool _bspl_decode_string(
    bspl_object_t *obj_ptr,
    char **str_ptr_ptr);
static bool _bspl_decode_charbuf(
    bspl_object_t *obj_ptr,
    char *str_ptr,
    size_t len);

static bool _bspl_encode_dict(
    const bspl_desc_t *desc_ptr,
    void *src_ptr,
    bspl_dict_t *dest_dict_ptr);
static bspl_object_t *_bspl_encode_uint64(uint64_t uint64);
static bspl_object_t *_bspl_encode_int64(int64_t int64);
static bspl_object_t *_bspl_encode_double(double d);
static bspl_object_t *_bspl_encode_argb32(uint32_t argb32);
static bspl_object_t *_bspl_encode_bool(bool b);
static bspl_object_t *_bspl_encode_enum(
    int value,
    const bspl_enum_desc_t *enum_desc_ptr);
static bspl_object_t *_bspl_encode_string(const char *str_ptr);

/** Enum descriptor for decoding bool. */
static const bspl_enum_desc_t _bspl_bool_desc[] = {
    BSPL_ENUM("True", true),
    BSPL_ENUM("False", false),
    BSPL_ENUM("Yes", true),
    BSPL_ENUM("No", false),
    BSPL_ENUM("Enabled", true),
    BSPL_ENUM("Disabled", false),
    BSPL_ENUM("On", true),
    BSPL_ENUM("Off", false),
    BSPL_ENUM_SENTINEL()
};
/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool bspl_decode_dict(
    bspl_dict_t *dict_ptr,
    const bspl_desc_t *desc_ptr,
    void *dest_ptr)
{
    if (!_bspl_init_defaults(desc_ptr, dest_ptr)) {
        bspl_decoded_destroy(desc_ptr, dest_ptr);
        return false;
    }

    return _bspl_decode_dict_without_init(dict_ptr, desc_ptr, dest_ptr);
}

/* ------------------------------------------------------------------------- */
bspl_dict_t *bspl_encode_dict(
    const bspl_desc_t *desc_ptr,
    void *src_ptr)
{
    bspl_dict_t *dict_ptr = bspl_dict_create();
    if (NULL == dict_ptr) return NULL;

    if (_bspl_encode_dict(desc_ptr, src_ptr, dict_ptr)) return dict_ptr;

    bspl_dict_unref(dict_ptr);
    return NULL;
}

/* ------------------------------------------------------------------------- */
void bspl_decoded_destroy(
    const bspl_desc_t *desc_ptr,
    void *dest_ptr)
{
    for (const bspl_desc_t *iter_desc_ptr = desc_ptr;
         iter_desc_ptr->key_ptr != NULL;
         ++iter_desc_ptr) {
        void *ptr = BS_VALUE_AT(void, dest_ptr, iter_desc_ptr->field_ofs);
        switch (iter_desc_ptr->type) {
        case BSPL_TYPE_STRING:
            if (NULL != ptr) {
                char **str_ptr_ptr = ptr;
                free(*str_ptr_ptr);
                *str_ptr_ptr = NULL;
            }
            break;

        case BSPL_TYPE_DICT:
            bspl_decoded_destroy(iter_desc_ptr->v.v_dict_desc_ptr, ptr);
            break;

        case BSPL_TYPE_CUSTOM:
            if (NULL != iter_desc_ptr->v.v_custom.fini) {
                iter_desc_ptr->v.v_custom.fini(ptr);
            }
            break;

        case BSPL_TYPE_ARRAY:
            if (NULL != iter_desc_ptr->v.v_array.fini) {
                iter_desc_ptr->v.v_array.fini(ptr);
            }
            break;

        default:
            // Nothing.
            break;
        }
    }
}

/* ------------------------------------------------------------------------- */
bool bspl_enum_name_to_value(
    const bspl_enum_desc_t *enum_desc_ptr,
    const char *name_ptr,
    int *value_ptr)
{
    if (NULL == name_ptr) return false;
    for (; NULL != enum_desc_ptr->name_ptr; ++enum_desc_ptr) {
        if (0 == strcmp(enum_desc_ptr->name_ptr, name_ptr)) {
            *value_ptr = enum_desc_ptr->value;
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------------------- */
bool bspl_enum_value_to_name(
    const bspl_enum_desc_t *enum_desc_ptr,
    int value,
    const char **name_ptr_ptr)
{
    for (; NULL != enum_desc_ptr->name_ptr; ++enum_desc_ptr) {
        if (value == enum_desc_ptr->value) {
            *name_ptr_ptr = enum_desc_ptr->name_ptr;
            return true;
        }
    }
    return false;
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/**
 * Initializes default values at the destination, as described.
 *
 * @param desc_ptr
 * @param dest_ptr
 */
bool _bspl_init_defaults(const bspl_desc_t *desc_ptr,
                         void *dest_ptr)
{
    char **str_ptr_ptr;
    char *str_ptr;

    for (const bspl_desc_t *iter_desc_ptr = desc_ptr;
         iter_desc_ptr->key_ptr != NULL;
         ++iter_desc_ptr) {

        if (iter_desc_ptr->presence_ofs != iter_desc_ptr->field_ofs) {
            *BS_VALUE_AT(bool, dest_ptr, iter_desc_ptr->presence_ofs) = false;
        }

        switch (iter_desc_ptr->type) {
        case BSPL_TYPE_UINT64:
            *BS_VALUE_AT(uint64_t, dest_ptr, iter_desc_ptr->field_ofs) =
                iter_desc_ptr->v.v_uint64.default_value;
            break;

        case BSPL_TYPE_INT64:
            *BS_VALUE_AT(int64_t, dest_ptr, iter_desc_ptr->field_ofs) =
                iter_desc_ptr->v.v_int64.default_value;
            break;

        case BSPL_TYPE_DOUBLE:
            *BS_VALUE_AT(double, dest_ptr, iter_desc_ptr->field_ofs) =
                iter_desc_ptr->v.v_double.default_value;
            break;

        case BSPL_TYPE_ARGB32:
            *BS_VALUE_AT(uint32_t, dest_ptr, iter_desc_ptr->field_ofs) =
                iter_desc_ptr->v.v_argb32.default_value;
            break;

        case BSPL_TYPE_BOOL:
            *BS_VALUE_AT(bool, dest_ptr, iter_desc_ptr->field_ofs) =
                iter_desc_ptr->v.v_bool.default_value;
            break;

        case BSPL_TYPE_ENUM:
            *BS_VALUE_AT(int, dest_ptr, iter_desc_ptr->field_ofs) =
                iter_desc_ptr->v.v_enum.default_value;
            break;

        case BSPL_TYPE_STRING:
            str_ptr_ptr = BS_VALUE_AT(
                char*, dest_ptr, iter_desc_ptr->field_ofs);
            *str_ptr_ptr = logged_strdup(
                iter_desc_ptr->v.v_string.default_value_ptr);
            if (NULL == *str_ptr_ptr) return false;
            break;

        case BSPL_TYPE_CHARBUF:
            str_ptr = BS_VALUE_AT(
                char, dest_ptr, iter_desc_ptr->field_ofs);
            if (NULL == iter_desc_ptr->v.v_charbuf.default_value_ptr) break;

            if (iter_desc_ptr->v.v_charbuf.len <
                strlen(iter_desc_ptr->v.v_charbuf.default_value_ptr) + 1) {
                bs_log(BS_ERROR,
                       "Buffer size %zu <  %zu + 1, default charbuf (\"%s\")",
                       iter_desc_ptr->v.v_charbuf.len,
                       strlen(iter_desc_ptr->v.v_charbuf.default_value_ptr),
                       iter_desc_ptr->v.v_charbuf.default_value_ptr);
                return false;
            }
            strcpy(str_ptr, iter_desc_ptr->v.v_charbuf.default_value_ptr);
            break;

        case BSPL_TYPE_DICT:
            if (!_bspl_init_defaults(
                    iter_desc_ptr->v.v_dict_desc_ptr,
                    BS_VALUE_AT(void*, dest_ptr,
                                iter_desc_ptr->field_ofs))) {
                return false;
            }
            break;

        case BSPL_TYPE_CUSTOM:
            if (NULL != iter_desc_ptr->v.v_custom.init &&
                !iter_desc_ptr->v.v_custom.init(
                    BS_VALUE_AT(void*, dest_ptr, iter_desc_ptr->field_ofs))) {
                return false;
            }
            break;

        case BSPL_TYPE_ARRAY:
            if (NULL != iter_desc_ptr->v.v_array.init &&
                !iter_desc_ptr->v.v_array.init(
                    BS_VALUE_AT(void*, dest_ptr, iter_desc_ptr->field_ofs))) {
                return false;
            }
            break;

        default:
            bs_log(BS_ERROR, "Unsupported type %d.", iter_desc_ptr->type);
            return false;
        }
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/** Decodes the dict. Will not recursively initialize dicts. */
bool _bspl_decode_dict_without_init(
    bspl_dict_t *dict_ptr,
    const bspl_desc_t *desc_ptr,
    void *dest_ptr)
{
    for (const bspl_desc_t *iter_desc_ptr = desc_ptr;
         iter_desc_ptr->key_ptr != NULL;
         ++iter_desc_ptr) {

        bspl_object_t *obj_ptr = bspl_dict_get(
            dict_ptr, iter_desc_ptr->key_ptr);
        if (NULL == obj_ptr) {
            if (iter_desc_ptr->required) {
                bs_log(BS_ERROR, "Key \"%s\" not found in dict %p.",
                       iter_desc_ptr->key_ptr, dict_ptr);
                bspl_decoded_destroy(desc_ptr, dest_ptr);
                return false;
            }
            continue;
        }

        bool rv = false;
        switch (iter_desc_ptr->type) {
        case BSPL_TYPE_UINT64:
            rv = _bspl_decode_uint64(
                obj_ptr,
                BS_VALUE_AT(uint64_t, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_INT64:
            rv = _bspl_decode_int64(
                obj_ptr,
                BS_VALUE_AT(int64_t, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_DOUBLE:
            rv = _bspl_decode_double(
                obj_ptr,
                BS_VALUE_AT(double, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_ARGB32:
            rv = _bspl_decode_argb32(
                obj_ptr,
                BS_VALUE_AT(uint32_t, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_BOOL:
            rv = _bspl_decode_bool(
                obj_ptr,
                BS_VALUE_AT(bool, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_ENUM:
            rv = _bspl_decode_enum(
                obj_ptr,
                iter_desc_ptr->v.v_enum.desc_ptr,
                BS_VALUE_AT(int, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_STRING:
            rv = _bspl_decode_string(
                obj_ptr,
                BS_VALUE_AT(char*, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_CHARBUF:
            rv = _bspl_decode_charbuf(
                obj_ptr,
                BS_VALUE_AT(char, dest_ptr, iter_desc_ptr->field_ofs),
                iter_desc_ptr->v.v_charbuf.len);
            break;
        case BSPL_TYPE_DICT:
            rv = _bspl_decode_dict_without_init(
                bspl_dict_from_object(obj_ptr),
                iter_desc_ptr->v.v_dict_desc_ptr,
                BS_VALUE_AT(void*, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_CUSTOM:
            rv = iter_desc_ptr->v.v_custom.decode(
                obj_ptr,
                BS_VALUE_AT(void*, dest_ptr, iter_desc_ptr->field_ofs));
            break;
        case BSPL_TYPE_ARRAY:
            if (BSPL_ARRAY == bspl_object_type(obj_ptr)) {
                bspl_array_t *array_ptr = bspl_array_from_object(obj_ptr);
                rv = true;
                for (size_t i = 0; i < bspl_array_size(array_ptr); ++i) {
                    if (!iter_desc_ptr->v.v_array.decode(
                            bspl_array_at(array_ptr, i),
                            i,
                            BS_VALUE_AT(
                                void *,
                                dest_ptr,
                                iter_desc_ptr->field_ofs))) {
                        rv = false;
                    }
                }
            }
            break;
        default:
            bs_log(BS_ERROR, "Unsupported type %d.", iter_desc_ptr->type);
            rv = false;
            break;
        }

        if (iter_desc_ptr->presence_ofs != iter_desc_ptr->field_ofs) {
            *BS_VALUE_AT(bool, dest_ptr, iter_desc_ptr->presence_ofs) = rv;
        }
        if (!rv) {
            bs_log(BS_ERROR, "Failed to decode key \"%s\"",
                   iter_desc_ptr->key_ptr);
            bspl_decoded_destroy(desc_ptr, dest_ptr);
            return false;
        }
    }
    return true;
}

/* ------------------------------------------------------------------------- */
bool _bspl_encode_dict(const bspl_desc_t *desc_ptr,
                       void *src_ptr,
                       bspl_dict_t *dest_dict_ptr)
{
    bspl_object_t *object_ptr;
    bspl_dict_t *sub_dict_ptr;

    for (const bspl_desc_t *iter_desc_ptr = desc_ptr;
         iter_desc_ptr->key_ptr != NULL;
         ++iter_desc_ptr) {

        // Check presence field, but only if given.
        if (iter_desc_ptr->presence_ofs != iter_desc_ptr->field_ofs &&
            !*BS_VALUE_AT(bool, src_ptr, iter_desc_ptr->presence_ofs)) {
            continue;
        }

        switch (iter_desc_ptr->type) {

        case BSPL_TYPE_UINT64:
            object_ptr = _bspl_encode_uint64(
                *BS_VALUE_AT(uint64_t, src_ptr, iter_desc_ptr->field_ofs));
            break;

        case BSPL_TYPE_INT64:
            object_ptr = _bspl_encode_int64(
                *BS_VALUE_AT(int64_t, src_ptr, iter_desc_ptr->field_ofs));
            break;

        case BSPL_TYPE_DOUBLE:
            object_ptr = _bspl_encode_double(
                *BS_VALUE_AT(double, src_ptr, iter_desc_ptr->field_ofs));
            break;

        case BSPL_TYPE_ARGB32:
            object_ptr = _bspl_encode_argb32(
                *BS_VALUE_AT(uint32_t, src_ptr, iter_desc_ptr->field_ofs));
            break;

        case BSPL_TYPE_BOOL:
            object_ptr = _bspl_encode_bool(
                *BS_VALUE_AT(bool, src_ptr, iter_desc_ptr->field_ofs));
            break;

        case BSPL_TYPE_ENUM:
            object_ptr = _bspl_encode_enum(
                *BS_VALUE_AT(int, src_ptr, iter_desc_ptr->field_ofs),
                iter_desc_ptr->v.v_enum.desc_ptr);
            break;

        case BSPL_TYPE_STRING:
            object_ptr = _bspl_encode_string(
                *BS_VALUE_AT(char*, src_ptr, iter_desc_ptr->field_ofs));
            break;

        case BSPL_TYPE_CHARBUF:
            object_ptr = _bspl_encode_string(
                BS_VALUE_AT(char, src_ptr, iter_desc_ptr->field_ofs));
            break;

        case BSPL_TYPE_DICT:
            sub_dict_ptr = bspl_dict_create();
            if (NULL == sub_dict_ptr) return false;
            if (!_bspl_encode_dict(
                    iter_desc_ptr->v.v_dict_desc_ptr,
                    BS_VALUE_AT(void*, src_ptr, iter_desc_ptr->field_ofs),
                    sub_dict_ptr)) {
                bspl_dict_unref(sub_dict_ptr);
                return false;
            }
            object_ptr = bspl_object_from_dict(sub_dict_ptr);
            break;

        default:
            return false;
        }

        if (NULL == object_ptr) return false;
        if (!bspl_dict_add(
                dest_dict_ptr,
                iter_desc_ptr->key_ptr,
                object_ptr)) {
            bs_log(BS_WARNING, "Failed bspl-dict_add(%p, \"%s\", %p)",
                   dest_dict_ptr,
                   iter_desc_ptr->key_ptr,
                   object_ptr);
            return false;
        }
        bspl_object_unref(object_ptr);
    }

    return true;
}

/* ------------------------------------------------------------------------- */
/** Decodes an unsigned number, using uint64_t as carry-all. */
bool _bspl_decode_uint64(bspl_object_t *obj_ptr, uint64_t *uint64_ptr)
{
    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) return false;
    const char *value_ptr = bspl_string_value(string_ptr);
    if (NULL == value_ptr) return false;
    return bs_strconvert_uint64(value_ptr, uint64_ptr, 10);
}

/* ------------------------------------------------------------------------- */
/** Decodes a signed number, using int64_t as carry-all. */
bool _bspl_decode_int64(bspl_object_t *obj_ptr, int64_t *int64_ptr)
{
    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) return false;
    const char *value_ptr = bspl_string_value(string_ptr);
    if (NULL == value_ptr) return false;
    return bs_strconvert_int64(value_ptr, int64_ptr, 10);
}

/* ------------------------------------------------------------------------- */
/** Decodes a floating point number. */
bool _bspl_decode_double(bspl_object_t *obj_ptr, double *double_ptr)
{
    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) return false;
    const char *value_ptr = bspl_string_value(string_ptr);
    if (NULL == value_ptr) return false;
    return bs_strconvert_double(value_ptr, double_ptr);
}

/* ------------------------------------------------------------------------- */
/** Deocdes an ARGB32 value from the config object. */
bool _bspl_decode_argb32(bspl_object_t *obj_ptr, uint32_t *argb32_ptr)
{
    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) return false;

    const char *value_ptr = bspl_string_value(string_ptr);
    if (NULL == value_ptr) return false;
    int rv = sscanf(value_ptr, "argb32:%"PRIx32, argb32_ptr);
    if (1 != rv) {
        bs_log(BS_ERROR | BS_ERRNO,
               "Failed sscanf(\"%s\", \"argb32:%%"PRIx32", %p)",
               value_ptr, argb32_ptr);
        return false;
    }

    return true;
}

/* ------------------------------------------------------------------------- */
/** Translates a bool value from the string. */
bool _bspl_decode_bool(
    bspl_object_t *obj_ptr,
    bool *bool_ptr)
{
    int bool_value;
    bool rv = _bspl_decode_enum(obj_ptr, _bspl_bool_desc, &bool_value);
    if (rv) *bool_ptr = bool_value;
    return rv;
}

/* ------------------------------------------------------------------------- */
/** Translates a enum value from the string, using the provided descriptor. */
bool _bspl_decode_enum(
    bspl_object_t *obj_ptr,
    const bspl_enum_desc_t *enum_desc_ptr,
    int *enum_value_ptr)
{
    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) return false;
    const char *value_ptr = bspl_string_value(string_ptr);
    if (NULL == value_ptr) return false;

    for (; NULL != enum_desc_ptr->name_ptr; ++enum_desc_ptr) {
        if (0 == strcmp(enum_desc_ptr->name_ptr, value_ptr)) {
            *enum_value_ptr = enum_desc_ptr->value;
            return true;
        }
    }

    bs_log(BS_WARNING, "Failed to decode enum value '%s'.", value_ptr);
    return false;
}

/* ------------------------------------------------------------------------- */
/** Translates (ie. duplicates) a string value from the plist string. */
bool _bspl_decode_string(
    bspl_object_t *obj_ptr,
    char **str_ptr_ptr)
{
    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) return false;
    const char *value_ptr = bspl_string_value(string_ptr);
    if (NULL == value_ptr) return false;

    if (NULL != *str_ptr_ptr) free(*str_ptr_ptr);
    *str_ptr_ptr = logged_strdup(value_ptr);
    return (NULL != *str_ptr_ptr);
}

/* ------------------------------------------------------------------------- */
/** Translates (ie. duplicates) a char buf from the plist string. */
bool _bspl_decode_charbuf(
    bspl_object_t *obj_ptr,
    char *str_ptr,
    size_t len)
{
    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) return false;
    const char *value_ptr = bspl_string_value(string_ptr);
    if (NULL == value_ptr) return false;

    if (len < strlen(value_ptr) + 1) {
        bs_log(BS_WARNING, "Charbuf size %zu < %zu + 1 for \"%s\"",
               len, strlen(value_ptr), value_ptr);
        return false;
    }
    strcpy(str_ptr, value_ptr);
    return true;
}

/* ------------------------------------------------------------------------- */
/** Encodes an unsigned 64-bit value into a plist object (a string). */
bspl_object_t *_bspl_encode_uint64(uint64_t uint64)
{
    char buf[21];  // Length of the UINT64_MAX value plus 1 for NUL.

    int rv = snprintf(buf, sizeof(buf), "%"PRIu64, uint64);
    if (0 > rv || (size_t)rv >= sizeof(buf)) return NULL;
    return bspl_object_from_string(bspl_string_create(buf));
}

/* ------------------------------------------------------------------------- */
/** Encodes a signed 64-bit value into a plist object (a string). */
bspl_object_t *_bspl_encode_int64(int64_t int64)
{
    char buf[22];  // Length of INT64_MIN value plus sign, plus 1 for NUL.

    int rv = snprintf(buf, sizeof(buf), "%"PRId64, int64);
    if (0 > rv || (size_t)rv >= sizeof(buf)) return NULL;
    return bspl_object_from_string(bspl_string_create(buf));
}

/* ------------------------------------------------------------------------- */
/** Encodes a floating point value into a plist object (a string). */
bspl_object_t *_bspl_encode_double(double d)
{
    char buf[32];  // Surely enough for a double.

    int rv = snprintf(buf, sizeof(buf), "%e", d);
    if (0 > rv || (size_t)rv >= sizeof(buf)) return NULL;
    return bspl_object_from_string(bspl_string_create(buf));
}

/* ------------------------------------------------------------------------- */
/** Encodes the 32-bit value as a ARGB32 value. */
bspl_object_t *_bspl_encode_argb32(uint32_t argb32)
{
    char buf[16];   // Enough for "argb32:aarrggbb".

    int rv = snprintf(buf, sizeof(buf), "argb32:%"PRIx32, argb32);
    if (0 > rv || (size_t)rv >= sizeof(buf)) return NULL;
    return bspl_object_from_string(bspl_string_create(buf));
}

/* ------------------------------------------------------------------------- */
/** Encodes the boolean value. Falls back to @ref _bspl_encode_enum. */
bspl_object_t *_bspl_encode_bool(bool b)
{
    return _bspl_encode_enum(b != 0, _bspl_bool_desc);
}

/* ------------------------------------------------------------------------- */
/** Encodes the integer value as an enum. */
bspl_object_t *_bspl_encode_enum(
    int value,
    const bspl_enum_desc_t *enum_desc_ptr)
{
    const char *p;
    if (!bspl_enum_value_to_name(enum_desc_ptr, value, &p)) return NULL;
    return bspl_object_from_string(bspl_string_create(p));
}

/* ------------------------------------------------------------------------- */
bspl_object_t *_bspl_encode_string(const char *str_ptr)
{
    return bspl_object_from_string(bspl_string_create(str_ptr));
}

/* == Unit tests =========================================================== */

static void test_init_defaults(bs_test_t *test_ptr);
static void test_enum_translate(bs_test_t *test_ptr);
static void test_decode_dict(bs_test_t *test_ptr);
static void test_decode_number(bs_test_t *test_ptr);
static void test_decode_argb32(bs_test_t *test_ptr);
static void test_decode_bool(bs_test_t *test_ptr);
static void test_decode_enum(bs_test_t *test_ptr);
static void test_decode_string(bs_test_t *test_ptr);
static void test_decode_charbuf(bs_test_t *test_ptr);

static void test_encode_dict(bs_test_t *test_ptr);
static void test_encode_number(bs_test_t *test_ptr);
static void test_encode_argb32(bs_test_t *test_ptr);
static void test_encode_bool(bs_test_t *test_ptr);
static void test_encode_enum(bs_test_t *test_ptr);
static void test_encode_string(bs_test_t *test_ptr);

const bs_test_case_t bspl_decode_test_cases[] = {
    { 1, "init_defaults", test_init_defaults },
    { 1, "enum_translate", test_enum_translate },
    { 1, "dict", test_decode_dict },
    { 1, "number", test_decode_number },
    { 1, "argb32", test_decode_argb32 },
    { 1, "bool", test_decode_bool },
    { 1, "enum", test_decode_enum },
    { 1, "string", test_decode_string },
    { 1, "charbuf", test_decode_charbuf },
    { 1, "encode_dict", test_encode_dict },
    { 1, "encode_number", test_encode_number },
    { 1, "encode_argb32", test_encode_argb32 },
    { 1, "encode_bool", test_encode_bool },
    { 1, "encode_enum", test_encode_enum },
    { 1, "encode_string", test_encode_string },
    { 0, NULL, NULL },
};

static bool _bspl_test_custom_decode(bspl_object_t *o_ptr, void *dst_ptr);
static void _bspl_test_array_item_destroy(
    bs_dllist_node_t *dlnode_ptr,
    void *ud_ptr);
static bool _bspl_test_custom_init(void *dst_ptr);
static void _bspl_test_custom_fini(void *dst_ptr);
static bool _bspl_test_array_decode(
    bspl_object_t *obj_ptr, size_t i, void *dst_ptr);
static bool _bspl_test_array_init(void *dst_ptr);
static void _bspl_test_array_fini(void *dst_ptr);

/** Structure with test values. */
typedef struct {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    char                      *value;
#endif  // DOXYGEN_SHOULD_SKIP_THIS
} _test_subdict_value_t;

/** Structure with test values. */
typedef struct {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    uint64_t                  v_uint64;
    bool                      has_uint64;
    int64_t                   v_int64;
    bool                      has_int64;
    double                    v_double;
    bool                      has_double;
    uint32_t                  v_argb32;
    bool                      has_argb32;
    bool                      v_bool;
    bool                      has_bool;
    int                       v_enum;
    bool                      has_enum;
    char                      *v_string;
    bool                      has_string;
    char                      v_charbuf[10];
    bool                      has_charbuf;
    _test_subdict_value_t     subdict;
    bool                      has_subdict;
    void                      *v_custom_ptr;
    bool                      has_custom;
    bs_dllist_t               *dllist_ptr;
    bool                      has_array;
#endif  // DOXYGEN_SHOULD_SKIP_THIS
} _test_value_t;

/** A decoded array item. */
typedef struct {
    /** Node of _test_value_t::dllist_ptr. */
    bs_dllist_node_t          dlnode;
    /** the character of the item. */
    char                      c;
} _test_array_item_t;

/** An enum descriptor. */
static const bspl_enum_desc_t _test_enum_desc[] = {
    BSPL_ENUM("enum1", 1),
    BSPL_ENUM("enum2", 2),
    BSPL_ENUM_SENTINEL()
};

/** Descriptor of a contained dict. */
static const bspl_desc_t _bspl_decode_test_subdesc[] = {
    BSPL_DESC_STRING("string", true, _test_subdict_value_t, value, value,
                     "Other String"),
    BSPL_DESC_SENTINEL(),
};

/** Test descriptor. */
static const bspl_desc_t _bspl_decode_test_desc[] = {
    BSPL_DESC_UINT64("u64", true, _test_value_t, v_uint64, has_uint64, 1234),
    BSPL_DESC_INT64("i64", true, _test_value_t, v_int64, has_int64, -1234),
    BSPL_DESC_DOUBLE("d", true, _test_value_t, v_double, has_double, 3.14),
    BSPL_DESC_ARGB32("argb32", true, _test_value_t, v_argb32, has_argb32,
                     0x01020304),
    BSPL_DESC_BOOL("bool", true, _test_value_t, v_bool, has_bool, true),
    BSPL_DESC_ENUM("enum", true, _test_value_t, v_enum, has_enum,
                   3, _test_enum_desc),
    BSPL_DESC_STRING("string", true, _test_value_t, v_string, has_string,
                     "The String"),
    BSPL_DESC_CHARBUF("charbuf", true, _test_value_t, v_charbuf, has_charbuf,
                      10, "CharBuf"),
    BSPL_DESC_DICT("subdict", true, _test_value_t, subdict, has_subdict,
                   _bspl_decode_test_subdesc),
    BSPL_DESC_CUSTOM("custom", true, _test_value_t, v_custom_ptr, has_custom,
                     _bspl_test_custom_decode,
                     _bspl_test_custom_init,
                     _bspl_test_custom_fini),
    BSPL_DESC_ARRAY("array", true, _test_value_t, dllist_ptr, has_array,
                    _bspl_test_array_decode,
                    _bspl_test_array_init,
                    _bspl_test_array_fini),
    BSPL_DESC_SENTINEL(),
};


/* ------------------------------------------------------------------------- */
/** A custom decoding function. Here: just decode a string. */
bool _bspl_test_custom_decode(bspl_object_t *o_ptr,
                              void *dst_ptr)
{
    char** str_ptr_ptr = dst_ptr;
    _bspl_test_custom_fini(dst_ptr);

    bspl_string_t *string_ptr = bspl_string_from_object(o_ptr);
    if (NULL == string_ptr) return false;
    *str_ptr_ptr = logged_strdup(bspl_string_value(string_ptr));
    return *str_ptr_ptr != NULL;
}

/* ------------------------------------------------------------------------- */
/** A custom decoding initializer. Here: Just create a string. */
bool _bspl_test_custom_init(void *dst_ptr)
{
    char** str_ptr_ptr = dst_ptr;
    _bspl_test_custom_fini(dst_ptr);
    *str_ptr_ptr = logged_strdup("Custom Init");
    return *str_ptr_ptr != NULL;
}

/* ------------------------------------------------------------------------- */
/** A custom decoding cleanup method. Frees the string. */
void _bspl_test_custom_fini(void *dst_ptr)
{
    char** str_ptr_ptr = dst_ptr;
    if (NULL != *str_ptr_ptr) {
        free(*str_ptr_ptr);
        *str_ptr_ptr = NULL;
    }
}

/* ------------------------------------------------------------------------- */
/** Deocde method for array item. */
bool _bspl_test_array_decode(
    bspl_object_t *obj_ptr,
    __UNUSED__ size_t i,
    void *dst_ptr)
{
    bs_dllist_t **dllist_ptr_ptr = dst_ptr;

    _test_array_item_t *item_ptr = logged_calloc(1, sizeof(_test_array_item_t));
    if (NULL == item_ptr) return false;

    bspl_string_t *string_ptr = bspl_string_from_object(obj_ptr);
    if (NULL == string_ptr) {
        free(item_ptr);
        return false;
    }

    item_ptr->c = bspl_string_value(string_ptr)[0];
    bs_dllist_push_back(*dllist_ptr_ptr, &item_ptr->dlnode);
    return true;
}

/* ------------------------------------------------------------------------- */
void _bspl_test_array_item_destroy(
    bs_dllist_node_t *dlnode_ptr,
    __UNUSED__ void *ud_ptr)
{
    _test_array_item_t *item_ptr = BS_CONTAINER_OF(
        dlnode_ptr, _test_array_item_t, dlnode);
    free(item_ptr);
}

/* ------------------------------------------------------------------------- */
bool _bspl_test_array_init(void *dst_ptr)
{
    bs_dllist_t **dllist_ptr_ptr = dst_ptr;
    *dllist_ptr_ptr = logged_calloc(1, sizeof(bs_dllist_t));
    if (NULL == *dllist_ptr_ptr) return false;
    return true;
}

/* ------------------------------------------------------------------------- */
void _bspl_test_array_fini(void *dst_ptr)
{
    bs_dllist_t **dllist_ptr_ptr = dst_ptr;
    bs_dllist_for_each(*dllist_ptr_ptr, _bspl_test_array_item_destroy, NULL);
    if (NULL != *dllist_ptr_ptr) {
        free(*dllist_ptr_ptr);
        *dllist_ptr_ptr = NULL;
    }
}

/* ------------------------------------------------------------------------- */
/** Tests initialization of default values. */
void test_init_defaults(bs_test_t *test_ptr)
{
    _test_value_t val;
    memset(&val, 1, sizeof(_test_value_t));
    val.v_custom_ptr = NULL;
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bspl_init_defaults(_bspl_decode_test_desc, &val));
    BS_TEST_VERIFY_EQ(test_ptr, 1234, val.v_uint64);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_uint64);
    BS_TEST_VERIFY_EQ(test_ptr, -1234, val.v_int64);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_int64);
    BS_TEST_VERIFY_EQ(test_ptr, 0x01020304, val.v_argb32);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_argb32);
    BS_TEST_VERIFY_EQ(test_ptr, true, val.v_bool);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_bool);
    BS_TEST_VERIFY_EQ(test_ptr, 3, val.v_enum);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_enum);
    BS_TEST_VERIFY_STREQ(test_ptr, "The String", val.v_string);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_string);
    BS_TEST_VERIFY_STREQ(test_ptr, "CharBuf", val.v_charbuf);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_charbuf);
    BS_TEST_VERIFY_STREQ(test_ptr, "Other String", val.subdict.value);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_subdict);
    BS_TEST_VERIFY_STREQ(test_ptr, "Custom Init", val.v_custom_ptr);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_custom);
    BS_TEST_VERIFY_FALSE(test_ptr, val.has_array);
    bspl_decoded_destroy(_bspl_decode_test_desc, &val);
}

/* ------------------------------------------------------------------------- */
/** Tests @ref bspl_enum_name_to_value and @ref bspl_enum_value_to_name. */
void test_enum_translate(bs_test_t *test_ptr)
{
    const bspl_enum_desc_t *d = _bspl_bool_desc;
    int v;

    BS_TEST_VERIFY_TRUE(test_ptr, bspl_enum_name_to_value(d, "True", &v));
    BS_TEST_VERIFY_EQ(test_ptr, 1, v);
    BS_TEST_VERIFY_TRUE(test_ptr, bspl_enum_name_to_value(d, "On", &v));
    BS_TEST_VERIFY_EQ(test_ptr, 1, v);
    BS_TEST_VERIFY_TRUE(test_ptr, bspl_enum_name_to_value(d, "Off", &v));
    BS_TEST_VERIFY_EQ(test_ptr, 0, v);
    BS_TEST_VERIFY_FALSE(test_ptr, bspl_enum_name_to_value(d, "Bad", &v));
    BS_TEST_VERIFY_FALSE(test_ptr, bspl_enum_name_to_value(d, NULL, &v));

    const char *n_ptr;
    BS_TEST_VERIFY_TRUE(test_ptr, bspl_enum_value_to_name(d, true, &n_ptr));
    BS_TEST_VERIFY_STREQ(test_ptr, "True", n_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bspl_enum_value_to_name(d, false, &n_ptr));
    BS_TEST_VERIFY_STREQ(test_ptr, "False", n_ptr);
    BS_TEST_VERIFY_FALSE(test_ptr, bspl_enum_value_to_name(d, 42, &n_ptr));
}

/* ------------------------------------------------------------------------- */
/** Tests dict decoding. */
void test_decode_dict(bs_test_t *test_ptr)
{
    _test_value_t val = {};
    const char *plist_string_ptr = ("{"
                                    "u64 = \"100\";"
                                    "i64 = \"-101\";"
                                    "d = \"-1.414\";"
                                    "argb32 = \"argb32:0204080c\";"
                                    "bool = Disabled;"
                                    "enum = enum1;"
                                    "string = TestString;"
                                    "charbuf = TestBuf;"
                                    "subdict = { string = OtherTestString };"
                                    "array = (a, b);"
                                    "custom = CustomThing"
                                    "}");
    bspl_dict_t *dict_ptr;

    dict_ptr = bspl_dict_from_object(
        bspl_create_object_from_plist_string(plist_string_ptr));
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, dict_ptr);
    BS_TEST_VERIFY_TRUE_OR_RETURN(
        test_ptr,
        bspl_decode_dict(dict_ptr, _bspl_decode_test_desc, &val));
    BS_TEST_VERIFY_EQ(test_ptr, 100, val.v_uint64);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_uint64);
    BS_TEST_VERIFY_EQ(test_ptr, -101, val.v_int64);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_int64);
    BS_TEST_VERIFY_EQ(test_ptr, -1.414, val.v_double);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_double);
    BS_TEST_VERIFY_EQ(test_ptr, 0x0204080c, val.v_argb32);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_argb32);
    BS_TEST_VERIFY_EQ(test_ptr, false, val.v_bool);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_bool);
    BS_TEST_VERIFY_EQ(test_ptr, 1, val.v_enum);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_enum);
    BS_TEST_VERIFY_STREQ(test_ptr, "TestString", val.v_string);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_string);
    BS_TEST_VERIFY_STREQ(test_ptr, "TestBuf", val.v_charbuf);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_charbuf);
    BS_TEST_VERIFY_STREQ(test_ptr, "OtherTestString", val.subdict.value);
    // No presence flag there.
    BS_TEST_VERIFY_STREQ(test_ptr, "CustomThing", val.v_custom_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_custom);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, val.dllist_ptr);
    BS_TEST_VERIFY_EQ_OR_RETURN(test_ptr, 2, bs_dllist_size(val.dllist_ptr));
    BS_TEST_VERIFY_TRUE(test_ptr, val.has_array);
    _test_array_item_t *item_ptr = BS_CONTAINER_OF(
        val.dllist_ptr->head_ptr, _test_array_item_t, dlnode);
    BS_TEST_VERIFY_EQ(test_ptr, 'a', item_ptr->c);
    item_ptr = BS_CONTAINER_OF(
        item_ptr->dlnode.next_ptr, _test_array_item_t, dlnode);
    BS_TEST_VERIFY_EQ(test_ptr, 'b', item_ptr->c);
    bspl_dict_unref(dict_ptr);
    bspl_decoded_destroy(_bspl_decode_test_desc, &val);

    dict_ptr = bspl_dict_from_object(
        bspl_create_object_from_plist_string("{anything=value}"));
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bspl_decode_dict(dict_ptr, _bspl_decode_test_desc, &val));
    bspl_dict_unref(dict_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests number decoding. */
void test_decode_number(bs_test_t *test_ptr)
{
    bspl_object_t *obj_ptr;
    int64_t i64;
    uint64_t u64;

    obj_ptr = bspl_create_object_from_plist_string("42");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_uint64(obj_ptr, &u64));
    BS_TEST_VERIFY_EQ(test_ptr, 42, u64);
    bspl_object_unref(obj_ptr);

    obj_ptr = bspl_create_object_from_plist_string("\"-1234\"");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_FALSE(test_ptr, _bspl_decode_uint64(obj_ptr, &u64));
    bspl_object_unref(obj_ptr);

    obj_ptr = bspl_create_object_from_plist_string("42");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_int64(obj_ptr, &i64));
    BS_TEST_VERIFY_EQ(test_ptr, 42, i64);
    bspl_object_unref(obj_ptr);

    obj_ptr = bspl_create_object_from_plist_string("\"-1234\"");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_int64(obj_ptr, &i64));
    BS_TEST_VERIFY_EQ(test_ptr, -1234, i64);
    bspl_object_unref(obj_ptr);

    double d;
    obj_ptr = bspl_create_object_from_plist_string("\"3.14\"");
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_double(obj_ptr, &d));
    BS_TEST_VERIFY_EQ(test_ptr, 3.14, d);
    bspl_object_unref(obj_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests argb32 decoding. */
void test_decode_argb32(bs_test_t *test_ptr)
{
    bspl_object_t *obj_ptr = bspl_create_object_from_plist_string(
        "\"argb32:01020304\"");
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, obj_ptr);

    uint32_t argb32;
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_argb32(obj_ptr, &argb32));
    BS_TEST_VERIFY_EQ(test_ptr, 0x01020304, argb32);
    bspl_object_unref(obj_ptr);

    obj_ptr = bspl_create_object_from_plist_string(
        "{c=\"argb32:ffa0b0c0\"}");
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, obj_ptr);
    _test_value_t v;
    bspl_desc_t desc[] = {
        BSPL_DESC_ARGB32("c", false, _test_value_t , v_argb32, v_argb32,
                         0x01020304),
        BSPL_DESC_SENTINEL()
    };
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bspl_decode_dict(bspl_dict_from_object(obj_ptr), desc, &v));
    BS_TEST_VERIFY_EQ(test_ptr, 0xffa0b0c0, v.v_argb32);
    bspl_object_unref(obj_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests bool decoding. */
void test_decode_bool(bs_test_t *test_ptr)
{
    bool value;
    bspl_object_t *obj_ptr;

    obj_ptr = bspl_create_object_from_plist_string("Yes");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_bool(obj_ptr, &value));
    BS_TEST_VERIFY_TRUE(test_ptr, value);
    bspl_object_unref(obj_ptr);

    obj_ptr = bspl_create_object_from_plist_string("Disabled");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_bool(obj_ptr, &value));
    BS_TEST_VERIFY_FALSE(test_ptr, value);
    bspl_object_unref(obj_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests enum decoding. */
void test_decode_enum(bs_test_t *test_ptr)
{
    int value;
    bspl_object_t *obj_ptr;

    obj_ptr = bspl_create_object_from_plist_string("enum2");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bspl_decode_enum(obj_ptr, _test_enum_desc, &value));
    BS_TEST_VERIFY_EQ(test_ptr, 2, value);
    bspl_object_unref(obj_ptr);

    obj_ptr = bspl_create_object_from_plist_string("\"enum2\"");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _bspl_decode_enum(obj_ptr, _test_enum_desc, &value));
    BS_TEST_VERIFY_EQ(test_ptr, 2, value);
    bspl_object_unref(obj_ptr);

    obj_ptr = bspl_create_object_from_plist_string("INVALID");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        _bspl_decode_enum(obj_ptr, _test_enum_desc, &value));
    bspl_object_unref(obj_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests string decoding. */
void test_decode_string(bs_test_t *test_ptr)
{
    char *v_ptr = NULL;
    bspl_object_t *obj_ptr;

    obj_ptr = bspl_create_object_from_plist_string("TheString");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_string(obj_ptr, &v_ptr));
    BS_TEST_VERIFY_STREQ(test_ptr, "TheString", v_ptr);
    bspl_object_unref(obj_ptr);
    free(v_ptr);
    v_ptr = NULL;

    obj_ptr = bspl_create_object_from_plist_string("1234");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_string(obj_ptr, &v_ptr));
    BS_TEST_VERIFY_STREQ(test_ptr, "1234", v_ptr);
    bspl_object_unref(obj_ptr);
    // Not free-ing v_ptr => the next 'decode' call has to do that.

    obj_ptr = bspl_create_object_from_plist_string("\"quoted string\"");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, obj_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_string(obj_ptr, &v_ptr));
    BS_TEST_VERIFY_STREQ(test_ptr, "quoted string", v_ptr);
    bspl_object_unref(obj_ptr);
    free(v_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests string decoding into a char buf. */
void test_decode_charbuf(bs_test_t *test_ptr)
{
    char b[10];
    bspl_object_t *o;

    o = bspl_create_object_from_plist_string("123456789");
    BS_TEST_VERIFY_TRUE(test_ptr, _bspl_decode_charbuf(o, b, sizeof(b)));
    BS_TEST_VERIFY_STREQ(test_ptr, b, "123456789");
    bspl_object_unref(o);

    o = bspl_create_object_from_plist_string("1234567890");
    BS_TEST_VERIFY_FALSE(test_ptr, _bspl_decode_charbuf(o, b, sizeof(b)));
    bspl_object_unref(o);
}

/* ------------------------------------------------------------------------- */
/** Tests encoding a dict. */
void test_encode_dict(bs_test_t *t)
{
    _test_value_t val = {
        .v_uint64 = 64,
        .has_uint64 = true,
        .v_int64 = -2,
        .has_int64 = true,
        .v_double = 3.14,
        .has_double = true,
        .v_argb32 = 0x10203040,
        .has_argb32 = true,
        .v_bool = true,
        .has_bool = true,
        .v_enum = 1,
        .has_enum = true,
        .v_string = "TheStr",
        .has_string = true,
        .v_charbuf = { 'a', 'b' },
        .has_charbuf = true,
        .subdict.value = "SubVal",
        .has_subdict = true,
    };

    bspl_dict_t *d = bspl_encode_dict(_bspl_decode_test_desc, &val);
    BS_TEST_VERIFY_NEQ_OR_RETURN(t, NULL, d);

    BS_TEST_VERIFY_STREQ(t, "64", bspl_dict_get_string_value(d, "u64"));
    BS_TEST_VERIFY_STREQ(t, "-2", bspl_dict_get_string_value(d, "i64"));
    BS_TEST_VERIFY_STREQ(
        t,
        "3.140000e+00",
        bspl_dict_get_string_value(d, "d"));
    BS_TEST_VERIFY_STREQ(
        t,
        "argb32:10203040",
        bspl_dict_get_string_value(d, "argb32"));
    BS_TEST_VERIFY_STREQ(t, "True", bspl_dict_get_string_value(d, "bool"));
    BS_TEST_VERIFY_STREQ(t, "enum1", bspl_dict_get_string_value(d, "enum"));
    BS_TEST_VERIFY_STREQ(t, "TheStr", bspl_dict_get_string_value(d, "string"));
    BS_TEST_VERIFY_STREQ(t, "ab", bspl_dict_get_string_value(d, "charbuf"));

    bspl_dict_t *sd = bspl_dict_get_dict(d, "subdict");
    BS_TEST_VERIFY_NEQ_OR_RETURN(t, NULL, sd);
    BS_TEST_VERIFY_STREQ(t, "SubVal", bspl_dict_get_string_value(sd, "string"));

    bspl_dict_unref(d);
}

/* ------------------------------------------------------------------------- */
/** Tests number encoding. */
void test_encode_number(bs_test_t *t)
{
    bspl_string_t *s;

    s = bspl_string_from_object(_bspl_encode_uint64(UINT64_MAX));
    BS_TEST_VERIFY_STREQ(t, "18446744073709551615", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_int64(INT64_MAX));
    BS_TEST_VERIFY_STREQ(t, "9223372036854775807", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_int64(INT64_MIN));
    BS_TEST_VERIFY_STREQ(t, "-9223372036854775808", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_double(0.2));
    BS_TEST_VERIFY_STREQ(t, "2.000000e-01", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_double(DBL_MIN));
    BS_TEST_VERIFY_STREQ(t, "2.225074e-308", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_double(DBL_MAX));
    BS_TEST_VERIFY_STREQ(t, "1.797693e+308", bspl_string_value(s));
    bspl_string_unref(s);
}

/* ------------------------------------------------------------------------- */
/** Tests argb32 encoding. */
void test_encode_argb32(bs_test_t *test_ptr)
{
    bspl_string_t *s;

    s = bspl_string_from_object(_bspl_encode_argb32(0x10203040));
    BS_TEST_VERIFY_STREQ(test_ptr, "argb32:10203040", bspl_string_value(s));
    bspl_string_unref(s);
}

/* ------------------------------------------------------------------------- */
/** Tests bool encoding. */
void test_encode_bool(bs_test_t *test_ptr)
{
    bspl_string_t *s;
    s = bspl_string_from_object(_bspl_encode_bool(0));
    BS_TEST_VERIFY_STREQ(test_ptr, "False", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_bool(1));
    BS_TEST_VERIFY_STREQ(test_ptr, "True", bspl_string_value(s));
    bspl_string_unref(s);

    // Does not correspond to the defined enums, but must translate to True.
    s = bspl_string_from_object(_bspl_encode_bool(42));
    BS_TEST_VERIFY_STREQ(test_ptr, "True", bspl_string_value(s));
    bspl_string_unref(s);
}
/* ------------------------------------------------------------------------- */
/** Tests enum encoding. */
void test_encode_enum(bs_test_t *test_ptr)
{
    bspl_string_t *s;

    s = bspl_string_from_object(_bspl_encode_enum(1, _bspl_bool_desc));
    BS_TEST_VERIFY_STREQ(test_ptr, "True", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_enum(0, _bspl_bool_desc));
    BS_TEST_VERIFY_STREQ(test_ptr, "False", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_enum(42, _bspl_bool_desc));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, s);
}

/* ------------------------------------------------------------------------- */
/** Tests string encoding. */
void test_encode_string(bs_test_t *test_ptr)
{
    bspl_string_t *s = bspl_string_from_object(_bspl_encode_string("test"));
    BS_TEST_VERIFY_STREQ(test_ptr, "test", bspl_string_value(s));
    bspl_string_unref(s);

    s = bspl_string_from_object(_bspl_encode_string(NULL));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, s);
}

/* == End of decode.c ====================================================== */
