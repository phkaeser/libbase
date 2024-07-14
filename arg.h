/* ========================================================================= */
/**
 * @file arg.h
 * Permits to declare commandline flags, define defaults and a constraints,
 * and provides parsing functions.
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
#ifndef __LIBBASE_ARG_H__
#define __LIBBASE_ARG_H__

#include "test.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Parsing mode, whether to permit extra args or not. */
typedef enum {
    /** Expects that the defined args consume all of argc/argv. */
    BS_ARG_MODE_NO_EXTRA,
    /** Permits extra values (but nothing with a "--" prefix). */
    BS_ARG_MODE_EXTRA_VALUES,
    /** Permits any leftovers. */
    BS_ARG_MODE_EXTRA_ARGS,
} bs_arg_mode_t;

/** Type to consider for the arg. */
typedef enum {
    /** Used for sentinel. */
    BS_ARG_TYPE_UNDEFINED = 0,
    /** A boolean. */
    BS_ARG_TYPE_BOOL,
    /** An enum, from a set of strings. */
    BS_ARG_TYPE_ENUM,
    /** A string value. */
    BS_ARG_TYPE_STRING,
    /** An unsigned 32-bit value. */
    BS_ARG_TYPE_UINT32,
} bs_arg_type_t;

/** Holds the specification for a boolean argument. */
typedef struct {
    /** Default value, if not specified on the commandline. */
    const bool                default_value;
    /** Points to the boolean that will hold the value. */
    bool                      *value_ptr;
} bs_arg_bool_t;

/** Lookup table for enum. */
typedef struct {
    /** The human-readable string of the enum. */
    const char                *name_ptr;
    /** Correspnding numeric value. */
    int                       value;
} bs_arg_enum_table_t;

/** Holds the specification for a string argument. */
typedef struct {
    /** Default value, if not specified on the commandline. */
    const char                *default_value;
    /** Points to the string that will hold the value. */
    char                      **value_ptr;
} bs_arg_string_t;

/** Holds the specification for an unsigned 32-bit argument. */
typedef struct {
    /** Default value, if not specified on the commandline. */
    const uint32_t            default_value;
    /** Minimum permitted value. Use 0 to permit all possible values. */
    const uint32_t            min_value;
    /** Maximum permitted value. Use UINT32_MAX to permit everything. */
    const uint32_t            max_value;
    /** Points to the uint32_t that will hold the value. */
    uint32_t                  *value_ptr;
} bs_arg_uint32_t;

/** Holds the specification for an enum. */
typedef struct {
    /** Default value, if not specified on the commandline. */
    const char                *default_name_ptr;
    /** Lookup table. */
    const bs_arg_enum_table_t *lookup_table;
    /** Points to the int that will hold the value. */
    int                        *value_ptr;
} bs_arg_enum_t;

/** Holds specification for one argument. */
typedef struct {
    /** Type of the argument. */
    bs_arg_type_t             type;
    /** Name of the argument. */
    const char                *name_ptr;
    /** Description, may be NULL. */
    const char                *description_ptr;

    /** Specification for the type. */
    union {
        void                  *v_sentinel;
        bs_arg_bool_t         v_bool;
        bs_arg_enum_t         v_enum;
        bs_arg_string_t       v_string;
        bs_arg_uint32_t       v_uint32;
    } v;
} bs_arg_t ;

/** Defines a boolean argument. */
#define BS_ARG_BOOL(_name, _desc, _default, _value_ptr) {               \
    .type = BS_ARG_TYPE_BOOL,                                           \
    .name_ptr = _name,                                                  \
    .description_ptr = _desc,                                           \
    .v = { .v_bool = {                                                  \
        .default_value = _default,                                      \
        .value_ptr = _value_ptr                                         \
    } }                                                                 \
}

/** Defines an enum argument. */
#define BS_ARG_ENUM(_name, _desc, _default, _lookup_table, _value_ptr) {\
    .type = BS_ARG_TYPE_ENUM,                                           \
    .name_ptr = _name,                                                  \
    .description_ptr = _desc,                                           \
    .v = { .v_enum = {                                                  \
        .default_name_ptr = _default,                                   \
        .lookup_table = _lookup_table,                                  \
        .value_ptr = _value_ptr                                         \
    } }                                                                 \
}

/** Defines a string argument. */
#define BS_ARG_STRING(_name, _desc, _default, _value_ptr) {             \
     .type = BS_ARG_TYPE_STRING,                                         \
    .name_ptr = _name,                                                  \
    .description_ptr = _desc,                                           \
    .v = { .v_string = {                                                \
        .default_value = _default,                                      \
        .value_ptr = _value_ptr                                         \
    } }                                                                 \
}

/** Defines an unsigned 32-bit integer argument. */
#define BS_ARG_UINT32(_name, _desc, _default, _min, _max, _value_ptr) { \
    .type = BS_ARG_TYPE_UINT32,                                         \
    .name_ptr = _name,                                                  \
    .description_ptr = _desc,                                           \
    .v = { .v_uint32 = {                                                \
        .default_value = _default,                                      \
        .min_value = _min,                                              \
        .max_value = _max,                                              \
        .value_ptr = _value_ptr                                         \
    } }                                                                 \
}

/** Defines a sentinel for the argument list. */
#define BS_ARG_SENTINEL() {                     \
    .type = BS_ARG_TYPE_UNDEFINED,              \
    .name_ptr = NULL,                           \
    .description_ptr = NULL,                    \
    .v = { .v_sentinel = NULL }                 \
}

/**
 * Parses the commandline.
 *
 * @param arg_ptr Specifies the array of arguments, ending in a sentinel.
 * @param mode How to treat extra arguments.
 * @param argc_ptr Pointer to the number of arguments.
 * @param argv_ptr Pointer to the list of argument values. It is assumed that
 *     argv_ptr[0] holds the program's name. It will be skipped for parsing.
 *
 * @return true if the parsing succeeded.
 */
bool bs_arg_parse(const bs_arg_t *arg_ptr, const bs_arg_mode_t mode,
                  int *argc_ptr, const char **argv_ptr);

/**
 * Cleanup any allocated resources during parsing by |bs_arg_parse|.
 *
 * @param arg_ptr Specifies the array of arguments, ending in a sentinel.
 */
void bs_arg_cleanup(const bs_arg_t *arg_ptr);

/**
 * Prints the arguments.
 *
 * @param stream_ptr
 * @param arg_ptr
 */
int bs_arg_print_usage(FILE *stream_ptr, const bs_arg_t *arg_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_arg_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_ARG_H__ */
/* == End of arg.h ========================================================= */
