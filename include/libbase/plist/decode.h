/* ========================================================================= */
/**
 * @file decode.h
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
#ifndef __BSPL_DECODE_H__
#define __BSPL_DECODE_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Forward declaration: Descriptor. */
typedef struct _bspl_desc_t bspl_desc_t;

/** Enum descriptor. */
typedef struct {
    /** The string representation of the enum. */
    const char                *name_ptr;
    /** The corresponding numeric value. */
    int                       value;
} bspl_enum_desc_t;

/** Sentinel for an enum descriptor sequence. */
#define BSPL_ENUM_SENTINEL() { .name_ptr = NULL }

/** Helper to define an enum descriptor. */
#define BSPL_ENUM(_name, _value) { .name_ptr = (_name), .value = (_value) }

/** Supported types to decode from a plist dict. */
typedef enum {
    BSPL_TYPE_UINT64,
    BSPL_TYPE_INT64,
    BSPL_TYPE_DOUBLE,
    BSPL_TYPE_ARGB32,
    BSPL_TYPE_BOOL,
    BSPL_TYPE_ENUM,
    BSPL_TYPE_STRING,
    BSPL_TYPE_CHARBUF,
    BSPL_TYPE_DICT,
    BSPL_TYPE_CUSTOM,
    BSPL_TYPE_ARRAY,
} bspl_decode_type_t;

/** A signed 64-bit integer. */
typedef struct {
    /** The default value, if not in the dict. */
    int64_t                   default_value;
} bspl_desc_int64_t;

/** An unsigned 64-bit integer. */
typedef struct {
    /** The default value, if not in the dict. */
    uint64_t                   default_value;
} bspl_desc_uint64_t;

/** A floating point value. */
typedef struct {
    /** The default value, if not in the dict. */
    double                    default_value;
} bspl_desc_double_t;

/** A color, encoded as string in format 'argb32:aarrggbb'. */
typedef struct {
    /** The default value, if not in the dict. */
    uint32_t                   default_value;
} bspl_desc_argb32_t;

/** A boolean value. */
typedef struct {
    /** The default value, if not in the dict. */
    bool                      default_value;
} bspl_desc_bool_t;

/** An enum. */
typedef struct {
    /** The default value, if not in the dict. */
    int                        default_value;
    /** The enum descriptor. */
    const bspl_enum_desc_t     *desc_ptr;
} bspl_desc_enum_t;

/** A string. Will be (re)created and must be free'd. */
typedef struct {
    /** The default value, if not in the dict. */
    const char                *default_value_ptr;
} bspl_desc_string_t;

/** A char buffer. Fixed size, no need to create. */
typedef struct {
    /** Size of the char buffer at the specified offset. */
    size_t                    len;
    /** The default value, if not in the dict. */
    const char                *default_value_ptr;
} bspl_desc_charbuf_t;

/** An array, with a decoder for each item. */
typedef struct {
    /** Decoding method, for item at position `i`. */
    bool (*decode)(bspl_object_t *obj_ptr, size_t i, void *value_ptr);
    /** Initializer method: Allocate or prepare `value_ptr`. May be NULL. */
    bool (*init)(void *value_ptr);
    /** Cleanup method: Frees `value_ptr`. May be NULL.. */
    void (*fini)(void *value_ptr);
} bspl_desc_array_t;

/** A custom decoder. */
typedef struct {
    /** Initializer method: Allocate or prepare `value_ptr`. May be NULL. */
    bool (*init)(void *value_ptr);
    /** Cleanup method: Frees `value_ptr`. May be NULL.. */
    void (*fini)(void *value_ptr);
} bspl_desc_custom_t;

/** The value-specific aspects of the plist dict descriptor. */
union bspl_desc_value {
    /** Value descriptor for signed 64-bit integer. */
    bspl_desc_int64_t         v_int64;
    /** Value descriptor for unsigned 64-bit integer. */
    bspl_desc_uint64_t        v_uint64;
    /** Value descriptor for double. */
    bspl_desc_double_t        v_double;
    /** Value descriptor for an ARGB32 color value. */
    bspl_desc_argb32_t        v_argb32;
    /** Value descriptor for a bool. */
    bspl_desc_bool_t          v_bool;
    /** Value descriptor for an enum. */
    bspl_desc_enum_t          v_enum;
    /** Value descriptor for a string. */
    bspl_desc_string_t        v_string;
    /** Value descriptor for a char buf. */
    bspl_desc_charbuf_t       v_charbuf;
    /** Value descriptor for a dict. */
    const bspl_desc_t         *v_dict_desc_ptr;
    /** Value descriptor for a custom element. */
    bspl_desc_custom_t        v_custom;
    /** Value descriptor for an array. */
    bspl_desc_array_t         v_array;
};

/** Descriptor to decode a plist dict. */
struct _bspl_desc_t {
    /** Type of the value. */
    bspl_decode_type_t        type;
    /** The key used for the described value in the plist dict. */
    const char                *key_ptr;
    /** Whether the field is required. */
    bool                      required;
    /** Offset of the field where to store the value. */
    size_t                    field_ofs;
    /**
     * Offset of the field (boolean) where to store whether this field had been
     * set. If @ref bspl_desc_t::presence_ofs == @ref bspl_desc_t::field_ofs,
     * presence will not be recorded.
     */
    size_t                    presence_ofs;

    /** Method to decode `object_ptr` into `value_ptr`. */
    bool (*decode)(bspl_object_t *object_ptr,
                   const union bspl_desc_value *desc_value_ptr,
                   void *value_ptr);
    /** Method to encode `value_ptr` and return as a plist object. */
    bspl_object_t *(*encode)(const union bspl_desc_value *desc_value_ptr,
                             const void *value_ptr);

    /** And the descriptor of the value. */
    union bspl_desc_value     v;
};

/** Decodes an unsigned number, using uint64_t as carry-all. */
bool bspl_decode_uint64(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);
/** Decodes a signed number, using int64_t as carry-all. */
bool bspl_decode_int64(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);
/** Decodes a floating point number. */
bool bspl_decode_double(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);
/** Decodes an ARGB32 color value. */
bool bspl_decode_argb32(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);
/** Decodes a boolean. */
bool bspl_decode_bool(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);
/** Decodes an enum. */
bool bspl_decode_enum(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);
/** Decodes a string. */
bool bspl_decode_string(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);
/** Decodes a charbuf. */
bool bspl_decode_charbuf(
    bspl_object_t *obj_ptr,
    const union bspl_desc_value *desc_value_ptr,
    void *value_ptr);

/** Encodes an unsigned 64-bit value into a plist object (a string). */
bspl_object_t *bspl_encode_uint64(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes a signed 64-bit value into a plist object (a string). */
bspl_object_t *bspl_encode_int64(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes a double into a plist object (a string). */
bspl_object_t *bspl_encode_double(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes an ARGB32 color value into a plist object (a string). */
bspl_object_t *bspl_encode_argb32(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes a bool value into a plist object (a string). */
bspl_object_t *bspl_encode_bool(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes an enum value into a plist object (a string). */
bspl_object_t *bspl_encode_enum(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes a string into a plist object. */
bspl_object_t *bspl_encode_string(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes a character buffer into a plist object. */
bspl_object_t *bspl_encode_charbuf(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);
/** Encodes into a dictionnary. */
bspl_object_t *bspl_encode_dict_as_object(
    const union bspl_desc_value *desc_value_ptr,
    const void *value_ptr);

/** Descriptor sentinel. Put at end of a @ref bspl_desc_t sequence. */
#define BSPL_DESC_SENTINEL() { .key_ptr = NULL }

/** Descriptor for an unsigned int64. */
#define BSPL_DESC_UINT64(_key, _required, _base, _field, _presence, _default) { \
        .type = BSPL_TYPE_UINT64,                                       \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_uint64,                               \
            .encode = bspl_encode_uint64,                               \
            .v.v_uint64.default_value = _default                        \
            }

/** Descriptor for an signed int64. */
#define BSPL_DESC_INT64(_key, _required, _base, _field, _presence, _default) { \
        .type = BSPL_TYPE_INT64,                                        \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_int64,                                \
            .encode = bspl_encode_int64,                                \
            .v.v_int64.default_value = _default                         \
            }

/** Descriptor for a floating point value. */
#define BSPL_DESC_DOUBLE(_key, _required, _base, _field, _presence, _default) { \
        .type = BSPL_TYPE_DOUBLE,                                       \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_double,                               \
            .encode = bspl_encode_double,                               \
            .v.v_double.default_value = _default                        \
            }

/** Descriptor for an ARGB32 value. */
#define BSPL_DESC_ARGB32(_key, _required, _base, _field, _presence, _default) { \
        .type = BSPL_TYPE_ARGB32,                                       \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_argb32,                               \
            .encode = bspl_encode_argb32,                               \
            .v.v_argb32.default_value = _default                        \
            }

/** Descriptor for a bool value. */
#define BSPL_DESC_BOOL(_key, _required, _base, _field, _presence, _default) { \
        .type = BSPL_TYPE_BOOL,                                         \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_bool,                                 \
            .encode = bspl_encode_bool,                                 \
            .v.v_bool.default_value = _default                          \
            }

/** Descriptor for an enum value. */
#define BSPL_DESC_ENUM(_key, _required, _base, _field, _presence, _default, _desc_ptr) \
    {                                                                   \
        .type = BSPL_TYPE_ENUM,                                         \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_enum,                                 \
            .encode = bspl_encode_enum,                                 \
            .v.v_enum.default_value = _default,                         \
            .v.v_enum.desc_ptr = _desc_ptr                              \
            }

/** Descriptor for a string value value. */
#define BSPL_DESC_STRING(_key, _required, _base, _field, _presence, _default) { \
        .type = BSPL_TYPE_STRING,                                       \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_string,                               \
            .encode = bspl_encode_string,                               \
            .v.v_string.default_value_ptr = _default,                   \
            }

/** Descriptor for a char buffer. */
#define BSPL_DESC_CHARBUF(_key, _required, _base, _field, _presence, _len, _default) { \
        .type = BSPL_TYPE_CHARBUF,                                      \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = bspl_decode_charbuf,                              \
            .encode = bspl_encode_charbuf,                              \
            .v.v_charbuf.len = _len,                                    \
            .v.v_charbuf.default_value_ptr = _default,                  \
            }

/** Descriptor for a dict sub-value. */
#define BSPL_DESC_DICT(_key, _required, _base, _field, _presence, _desc) { \
        .type = BSPL_TYPE_DICT,                                         \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .encode = bspl_encode_dict_as_object,                       \
            .v.v_dict_desc_ptr = _desc                                  \
            }

/** Descriptor for a custom object decoder. */
#define BSPL_DESC_CUSTOM(_key, _required, _base, _field, _presence, _d ,_e, _i, _f) { \
        .type = BSPL_TYPE_CUSTOM,                                       \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .decode = _d,                                               \
            .encode = _e,                                               \
            .v.v_custom.init = _i,                                      \
            .v.v_custom.fini = _f,                                      \
            }

/** Descriptor for an array, with an item-decoder. */
#define BSPL_DESC_ARRAY(_key, _required, _base, _field, _presence, _d, _e, _i, _f) { \
        .type = BSPL_TYPE_ARRAY,                                        \
            .key_ptr = (_key),                                          \
            .required = _required,                                      \
            .field_ofs = offsetof(_base, _field),                       \
            .presence_ofs = offsetof(_base, _presence),                 \
            .encode = _e,                                               \
            .v.v_array.decode = _d,                                     \
            .v.v_array.init = _i,                                       \
            .v.v_array.fini = _f,                                       \
            }

/**
 * Decodes the plist `dict_ptr` into `value_ptr` as described.
 *
 * @param dict_ptr
 * @param desc_ptr
 * @param value_ptr
 *
 * @return true on success.
 */
bool bspl_decode_dict(
    bspl_dict_t *dict_ptr,
    const bspl_desc_t *desc_ptr,
    void *value_ptr);

/**
 * Encodes the data at `src_ptr` according to description into a plist object.
 *
 * @param desc_ptr
 * @param src_ptr
 *
 * @return A new plist object with the encoded data, or NULL on error.
 */
bspl_dict_t *bspl_encode_dict(
    const bspl_desc_t *desc_ptr,
    void *src_ptr);

/**
 * Destroys resources that were allocated during @ref bspl_decode_dict.
 *
 * @param desc_ptr
 * @param value_ptr
 */
void bspl_decoded_destroy(
    const bspl_desc_t *desc_ptr,
    void *value_ptr);

/**
 * Translates from the given name of an enum to it's value.
 *
 * @param enum_desc_ptr
 * @param name_ptr
 * @param value_ptr
 *
 * @return true if `name_ptr` was a valid enum name.
 */
bool bspl_enum_name_to_value(
    const bspl_enum_desc_t *enum_desc_ptr,
    const char *name_ptr,
    int *value_ptr);

/**
 * Translates from the given value of an enum to it's name.
 *
 * @param enum_desc_ptr
 * @param value
 * @param name_ptr_ptr
 *
 * @return true if `name_ptr` was a valid enum name.
 */
bool bspl_enum_value_to_name(
    const bspl_enum_desc_t *enum_desc_ptr,
    int value,
    const char **name_ptr_ptr);

/** Unit tests. */
extern const bs_test_case_t bspl_decode_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __BSPL_DECODE_H__ */
/* == End of decode.h ====================================================== */
