 /* ========================================================================= */
/**
 * @file arg.c
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

#include "arg.h"
#include "assert.h"
#include "avltree.h"
#include "c2x_compat.h"
#include "log.h"
#include "strconvert.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* == Declarations ========================================================= */

/** How the arg matches a bs_arg_t. */
typedef enum {
    /** No match at all. */
    _BS_ARG_NO_MATCH = 0,
    /** The value is "name=value", as one string. */
    _BS_ARG_MATCH_WITH_EQUAL_SIGN = 1,
    /** The value is "name", so the next arg is the value. */
    _BS_ARG_MATCH_WITH_TWO_ARGS = 2,
    /** It's a bool, an exact match. Doesn't take a value. */
    _BS_ARG_MATCH_BOOL = 3,
    /** It's a bool, with a 'no' prefix. Doesn't take a value. */
    _BS_ARG_MATCH_BOOL_OVERRIDE_WITH_NO = 4
} bs_arg_match_t;

static bs_arg_match_t get_match_type(const bs_arg_t *arg_ptr,
                                     const char *argv, const char *next_argv,
                                     const char **arg_value_ptr);
static bool find_matching_arg(const bs_arg_t *arg_ptr,
                              const char *argv, const char *next_argv,
                              const bs_arg_t **matching_arg_ptr,
                              const char **arg_value_ptr);
static bool parse_arg(const bs_arg_t *arg_ptr, const char *value_ptr);
static bool parse_bool(const bs_arg_bool_t *arg_bool_ptr,
                       const char *value_ptr);
static bool parse_enum(const bs_arg_enum_t *arg_enum_ptr,
                       const char *value_ptr);
static bool parse_string(const bs_arg_string_t *arg_string_ptr,
                         const char *value_ptr);
static bool parse_uint32(const bs_arg_uint32_t *arg_uint32_ptr,
                         const char *value_ptr);

static void set_all_defaults(const bs_arg_t *arg_ptr);
static bool check_arg(const bs_arg_t *arg_ptr);

/**  Store an arg name in a tree, to identify duplicates. */
typedef struct {
    /** Tree node. */
    bs_avltree_node_t         node;
    /** Argument name. */
    char                      *name_ptr;
} arg_name_t;

static int node_cmp(const bs_avltree_node_t *node_ptr, const void *key_ptr);
static arg_name_t *node_create(const char *prefix_ptr, const char *name_ptr);
static void node_destroy(bs_avltree_node_t *node_ptr);
static bool is_name_valid(const char *name_ptr);

static bool lookup_enum(const bs_arg_enum_table_t *lookup_table,
                        const char *name_ptr,
                        uint32_t *value_ptr);

/* == Data ================================================================= */

static const char             *bs_arg_bool_value_true = "true";
static const char             *bs_arg_bool_value_false = "false";

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool bs_arg_parse(const bs_arg_t *arg_ptr, const bs_arg_mode_t mode,
                  int *argc_ptr, const char **argv_ptr)
{
    if (!check_arg(arg_ptr)) {
        return false;
    }

    int                       not_consumed = 1;
    const bs_arg_t            *matching_arg_ptr;
    const char                *next_argv_ptr, *arg_value_ptr;

    set_all_defaults(arg_ptr);

    // Start at 1 -- argv_ptr[0] is the program's name.
    for (int i = 1; i < *argc_ptr; ++i) {
        next_argv_ptr = i + 1 >= *argc_ptr ? NULL : argv_ptr[i+1];

        if (!find_matching_arg(arg_ptr, argv_ptr[i], next_argv_ptr,
                               &matching_arg_ptr, &arg_value_ptr)) {
            bs_arg_cleanup(arg_ptr);
            return false;
        }
        if (NULL == matching_arg_ptr) {
            argv_ptr[not_consumed++] = argv_ptr[i];
            continue;
        }

        if (!parse_arg(matching_arg_ptr, arg_value_ptr)) {
            bs_arg_cleanup(arg_ptr);
            return false;
        }

        // Move by two args, if the next arg was actually used.
        if (arg_value_ptr == next_argv_ptr) ++i;
    }

    // Cleanup rest of argv.
    for (int i = not_consumed; i < *argc_ptr; ++i) {
        argv_ptr[i] = NULL;
    }
    *argc_ptr = not_consumed;

    bool retval = true;
    switch (mode) {
    case BS_ARG_MODE_NO_EXTRA:
        /* No extra args expected. */
        for (int i = 1; i < not_consumed; ++i) {
            bs_log(BS_WARNING, "Unexpected extra argv: %s", argv_ptr[i]);
            retval = false;
        }
        break;

    case BS_ARG_MODE_EXTRA_VALUES:
        /* Values are permitted, but report any extra "--" prefix. */
        for (int i = 1; i < not_consumed; ++i) {
            if (0 == strncmp(argv_ptr[i], "--", 2)) {
                bs_log(BS_WARNING, "Unexpected extra arg: %s", argv_ptr[i]);
                retval = false;
            }
        }
        break;

    case BS_ARG_MODE_EXTRA_ARGS:
        /* We're good. Anything is allowed. */
        break;

    default:
        bs_log(BS_FATAL, "Unexpected mode %d.", mode);
        retval = false;
        BS_ABORT();
    }

    if (!retval) bs_arg_cleanup(arg_ptr);
    return retval;
}

/* ------------------------------------------------------------------------- */
void bs_arg_cleanup(const bs_arg_t *arg_ptr)
{
    for (; NULL != arg_ptr->name_ptr; ++arg_ptr) {
        switch (arg_ptr->type) {
        case BS_ARG_TYPE_STRING:
            if (NULL != *arg_ptr->v.v_string.value_ptr) {
                free(*arg_ptr->v.v_string.value_ptr);
                *arg_ptr->v.v_string.value_ptr = NULL;
            }
            break;

        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------------- */
int bs_arg_print_usage(FILE *stream_ptr, const bs_arg_t *arg_ptr)
{
    const bs_arg_enum_table_t *lookup_table;
    int written_bytes = 0;
    for (; NULL != arg_ptr->name_ptr; ++arg_ptr) {
        written_bytes += fprintf(stream_ptr, "--%s : %s\n",
                                 arg_ptr->name_ptr,
                                 arg_ptr->description_ptr);

        switch (arg_ptr->type) {
        case BS_ARG_TYPE_ENUM:
            lookup_table = arg_ptr->v.v_enum.lookup_table;
            written_bytes += fprintf(stderr, "    Enum values:\n");
            for (; NULL != lookup_table->name_ptr; ++lookup_table) {
                written_bytes += fprintf(
                    stream_ptr, "      %s (%"PRIu32")\n",
                    lookup_table->name_ptr, lookup_table->value);
            }
            break;

        default:
            break;
        }
    }
    return written_bytes;
}

/* == Local methods ======================================================== */

/* ------------------------------------------------------------------------- */
/**
 * Returns the match type (if any) for |*arg_ptr| on |argv|. Updates
 * |*arg_value_ptr| to point to the value -- either NULL (no value expected),
 * |next_argv| (on _TWO_ARGS match) or to the part after '=' of |argv|.
 *
 * @param arg_ptr
 * @param argv
 * @param next_argv
 * @param arg_value_ptr
 *
 * @returns Match type.
 */
bs_arg_match_t get_match_type(const bs_arg_t *arg_ptr,
                              const char *argv, const char *next_argv,
                              const char **arg_value_ptr)
{
    size_t name_len;

    *arg_value_ptr = NULL;
    if (0 != strncmp(argv, "--", 2)) {
        return _BS_ARG_NO_MATCH;
    }
    argv +=2;

    // BOOL type: A full match, a matching with a "no" prefix. No value!
    if (BS_ARG_TYPE_BOOL == arg_ptr->type) {
        if (0 == strcmp(argv, arg_ptr->name_ptr)) {
            return _BS_ARG_MATCH_BOOL;
        }
        if (0 == strncmp(argv, "no", 2) &&
            0 == strcmp(argv + 2, arg_ptr->name_ptr)) {
            return _BS_ARG_MATCH_BOOL_OVERRIDE_WITH_NO;
        }
        return _BS_ARG_NO_MATCH;
    }

    // Other types: Matches must start with the arg name...
    name_len = strlen(arg_ptr->name_ptr);
    if (0 == strncmp(argv, arg_ptr->name_ptr, name_len)) {
        // ... and continue with a '=' and value, or space.
        if ('=' == argv[name_len]) {
            *arg_value_ptr = argv + name_len + 1;
            return _BS_ARG_MATCH_WITH_EQUAL_SIGN;
        }
        if ('\0' == argv[name_len]) {
            *arg_value_ptr = next_argv;
            return _BS_ARG_MATCH_WITH_TWO_ARGS;
        }
    }

    // Fall-through: It's not a match.
    return _BS_ARG_NO_MATCH;
}

/* ------------------------------------------------------------------------- */
/** Finds the arg matching |argv|. Updates arg_value_ptr to the value. */
bool find_matching_arg(const bs_arg_t *arg_ptr,
                       const char *argv, const char *next_argv,
                       const bs_arg_t **matching_arg_ptr,
                       const char **arg_value_ptr)
{
    bs_arg_match_t            match_type;

    *matching_arg_ptr = NULL;
    *arg_value_ptr = NULL;
    for (; BS_ARG_TYPE_UNDEFINED != arg_ptr->type; ++arg_ptr) {

        match_type = get_match_type(arg_ptr, argv, next_argv, arg_value_ptr);
        switch (match_type) {
        case _BS_ARG_MATCH_WITH_EQUAL_SIGN:
            BS_ASSERT(NULL != *arg_value_ptr);
            *matching_arg_ptr = arg_ptr;
            return true;

        case _BS_ARG_MATCH_WITH_TWO_ARGS:
            if (NULL == *arg_value_ptr) {
                bs_log(BS_WARNING, "Missing value for arg '%s'",
                       arg_ptr->name_ptr);
                return false;
            }
            *matching_arg_ptr = arg_ptr;
            return true;

        case _BS_ARG_MATCH_BOOL:
            *arg_value_ptr = bs_arg_bool_value_true;
            *matching_arg_ptr = arg_ptr;
            return true;

        case _BS_ARG_MATCH_BOOL_OVERRIDE_WITH_NO:
            *arg_value_ptr = bs_arg_bool_value_false;
            *matching_arg_ptr = arg_ptr;
            return true;

        case _BS_ARG_NO_MATCH:
        default:
            break;
        }

    }
    /* no match found, but not an error */
    return true;
}

/* ------------------------------------------------------------------------- */
/** Parses |value_ptr| for |arg_ptr|. */
bool parse_arg(const bs_arg_t *arg_ptr, const char *value_ptr)
{
    bool                      retval = false;

    switch(arg_ptr->type) {
    case BS_ARG_TYPE_BOOL:
        retval = parse_bool(&arg_ptr->v.v_bool, value_ptr);
        break;

    case BS_ARG_TYPE_ENUM:
        retval = parse_enum(&arg_ptr->v.v_enum, value_ptr);
        break;

    case BS_ARG_TYPE_STRING:
        retval = parse_string(&arg_ptr->v.v_string, value_ptr);
        break;

    case BS_ARG_TYPE_UINT32:
        retval = parse_uint32(&arg_ptr->v.v_uint32, value_ptr);
        break;

    case BS_ARG_TYPE_UNDEFINED:
    default:
        bs_log(BS_FATAL, "Unhandled arg type %d for %s", arg_ptr->type,
               arg_ptr->name_ptr);
        BS_ABORT();
    }

    if (!retval) {
        bs_log(BS_ERROR, "Failed to parse --%s for \"%s\"",
               arg_ptr->name_ptr, value_ptr);
    }
    return retval;
}

/* ------------------------------------------------------------------------- */
/** Parses a boolean value at |value_ptr|. */
bool parse_bool(const bs_arg_bool_t *arg_bool_ptr,
                const char *value_ptr)
{
    if (0 == strcmp(bs_arg_bool_value_true, value_ptr)) {
        *(arg_bool_ptr->value_ptr) = true;
        return true;
    }

    if (0 == strcmp(bs_arg_bool_value_false, value_ptr)) {
        *(arg_bool_ptr->value_ptr) = false;
        return true;
    }

    bs_log(BS_ERROR, "Unrecognized bool value \"%s\"", value_ptr);
    return false;
}

/* ------------------------------------------------------------------------- */
/** Parses an enum from |value_ptr|. */
bool parse_enum(const bs_arg_enum_t *arg_enum_ptr,
                const char *value_ptr)
{
    if (!lookup_enum(arg_enum_ptr->lookup_table,
                     value_ptr,
                     arg_enum_ptr->value_ptr)) {
        bs_log(BS_ERROR, "Unknown value \"%s\" for enum.", value_ptr);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/** Parses a string value at |value_ptr|. */
bool parse_string(const bs_arg_string_t *arg_string_ptr,
                  const char *value_ptr)
{
    if (NULL != *arg_string_ptr->value_ptr) {
        free(*arg_string_ptr->value_ptr);
    }
    *arg_string_ptr->value_ptr = strdup(value_ptr);
    if (NULL == *arg_string_ptr->value_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed strdup(%s)", value_ptr);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/** Parses a 32-bit unsigned integer value at |value_ptr|. */
bool parse_uint32(const bs_arg_uint32_t *arg_uint32_ptr,
                  const char *value_ptr)
{
    uint64_t                  value;

    if (!bs_strconvert_uint64(value_ptr, &value, 10)) {
        return false;
    }

    if (value < arg_uint32_ptr->min_value) {
        bs_log(BS_ERROR, "Out of range: \"%s\" (%"PRIu64" < %"PRIu32")",
               value_ptr, value, arg_uint32_ptr->min_value);
        return false;
    }

    if (value > arg_uint32_ptr->max_value) {
        bs_log(BS_ERROR, "Out of range: \"%s\" (%"PRIu64" > %"PRIu32")",
               value_ptr, value, arg_uint32_ptr->max_value);
        return false;
    }

    BS_ASSERT(value <= UINT32_MAX);
    *arg_uint32_ptr->value_ptr = (uint32_t)value;
    return true;
}

/* ------------------------------------------------------------------------- */
void set_all_defaults(const bs_arg_t *arg_ptr)
{
    for (;
         NULL != arg_ptr->name_ptr && BS_ARG_TYPE_UNDEFINED != arg_ptr->type;
         ++arg_ptr) {

        switch(arg_ptr->type) {
        case BS_ARG_TYPE_BOOL:
            *arg_ptr->v.v_bool.value_ptr = arg_ptr->v.v_bool.default_value;
            break;

        case BS_ARG_TYPE_ENUM:
            if (!lookup_enum(arg_ptr->v.v_enum.lookup_table,
                             arg_ptr->v.v_enum.default_name_ptr,
                             arg_ptr->v.v_enum.value_ptr)) {
                bs_log(BS_FATAL, "Failed to lookup default \"%s\" for enum %s",
                       arg_ptr->v.v_enum.default_name_ptr, arg_ptr->name_ptr);
                BS_ABORT();
            }
            break;

        case BS_ARG_TYPE_STRING:
            if (NULL == arg_ptr->v.v_string.default_value) {
                *arg_ptr->v.v_string.value_ptr = NULL;
            } else {
                *arg_ptr->v.v_string.value_ptr = strdup(
                    arg_ptr->v.v_string.default_value);
                if (NULL == *arg_ptr->v.v_string.value_ptr) {
                    bs_log(BS_FATAL | BS_ERRNO, "Failed strdup(%s)",
                           arg_ptr->v.v_string.default_value);
                    BS_ABORT();
                }
            }
            break;

        case BS_ARG_TYPE_UINT32:
            *arg_ptr->v.v_uint32.value_ptr = arg_ptr->v.v_uint32.default_value;
            break;

        case BS_ARG_TYPE_UNDEFINED:
        default:
            bs_log(BS_FATAL, "Unhandled arg type %d for %s", arg_ptr->type,
                   arg_ptr->name_ptr);
            BS_ABORT();
        }
    }
}

/* ------------------------------------------------------------------------- */
bool check_arg(const bs_arg_t *arg_ptr)
{
    bool                      retval = true;
    bs_avltree_t              *tree_ptr;
    arg_name_t                *arg_name_ptr;

    tree_ptr = bs_avltree_create(node_cmp, node_destroy);

    for (; BS_ARG_TYPE_UNDEFINED != arg_ptr->type; ++arg_ptr) {

        if (NULL == arg_ptr->name_ptr) {
            bs_log(BS_ERROR, "Name not given for arg_ptr at %p", arg_ptr);
            retval = false;
            continue;
        }
        if (!is_name_valid(arg_ptr->name_ptr)) {
            // Already logged.
            retval = false;
            continue;
        }

        arg_name_ptr = node_create("", arg_ptr->name_ptr);
        if (!bs_avltree_insert(tree_ptr, arg_name_ptr->name_ptr,
                               &arg_name_ptr->node, false)) {
            bs_log(BS_ERROR, "Duplicate argument name \"%s\"",
                   arg_name_ptr->name_ptr);
            node_destroy(&arg_name_ptr->node);
            retval = false;
            continue;
        }


        switch(arg_ptr->type) {
        case BS_ARG_TYPE_BOOL:
            if (NULL == arg_ptr->v.v_bool.value_ptr) {
                bs_log(BS_ERROR, "Pointer to value not provided for --%s",
                       arg_ptr->name_ptr);
                retval = false;
            }

            arg_name_ptr = node_create("no", arg_ptr->name_ptr);
            if (!bs_avltree_insert(tree_ptr, arg_name_ptr->name_ptr,
                                   &arg_name_ptr->node, false)) {
                bs_log(BS_ERROR, "Duplicate argument name \"%s\"",
                       arg_name_ptr->name_ptr);
                node_destroy(&arg_name_ptr->node);
                retval = false;
                continue;
            }

            break;

        case BS_ARG_TYPE_ENUM:
            if (NULL == arg_ptr->v.v_enum.value_ptr) {
                bs_log(BS_ERROR, "Pointer to value not provided for --%s",
                       arg_ptr->name_ptr);
                retval = false;
            }

            if (NULL == arg_ptr->v.v_enum.lookup_table ||
                NULL == arg_ptr->v.v_enum.lookup_table->name_ptr) {
                bs_log(BS_ERROR, "Lookup table missing or empty for enum --%s",
                       arg_ptr->name_ptr);
                retval = false;
            }
            break;

        case BS_ARG_TYPE_STRING:
            if (NULL == arg_ptr->v.v_string.value_ptr) {
                bs_log(BS_ERROR, "Pointer to value not provided for --%s",
                       arg_ptr->name_ptr);
                retval = false;
            }
            break;

        case BS_ARG_TYPE_UINT32:
            if (NULL == arg_ptr->v.v_uint32.value_ptr) {
                bs_log(BS_ERROR, "Pointer to value not provided for --%s",
                       arg_ptr->name_ptr);
                retval = false;
            }
            break;

        default:
            bs_log(BS_ERROR, "Unhandled type for --%s", arg_ptr->name_ptr);
            retval = false;
            break;
        }

    }

    bs_avltree_destroy(tree_ptr);
    return retval;
}

/* == Helpers for storing the names in a tree ============================== */

/* ------------------------------------------------------------------------- */
int node_cmp(const bs_avltree_node_t *node_ptr, const void *key_ptr)
{
    arg_name_t *arg_name_ptr = BS_CONTAINER_OF(node_ptr, arg_name_t, node);
    return strcmp(arg_name_ptr->name_ptr, (const char*)key_ptr);
}

/* ------------------------------------------------------------------------- */
arg_name_t *node_create(const char *prefix_ptr, const char *name_ptr)
{
    size_t                    len;
    arg_name_t                *arg_name_ptr;

    arg_name_ptr = (arg_name_t*)calloc(1, sizeof(arg_name_t));
    if (NULL == arg_name_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed calloc(1, %zu)",
               sizeof(arg_name_t));
        return NULL;
    }

    len = strlen(prefix_ptr) + strlen(name_ptr) + 1;
    arg_name_ptr->name_ptr = malloc(len);
    if (NULL == arg_name_ptr->name_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed malloc(%zu)", len);
        node_destroy(&arg_name_ptr->node);
    }
    strcpy(arg_name_ptr->name_ptr, prefix_ptr);
    strcpy(arg_name_ptr->name_ptr + strlen(prefix_ptr), name_ptr);
    return arg_name_ptr;
}

/* ------------------------------------------------------------------------- */
void node_destroy(bs_avltree_node_t *node_ptr)
{
    arg_name_t *arg_name_ptr = BS_CONTAINER_OF(node_ptr, arg_name_t, node);
    if (NULL != arg_name_ptr->name_ptr) {
        free(arg_name_ptr->name_ptr);
        arg_name_ptr->name_ptr = NULL;
    }

    free(arg_name_ptr);
}

/* ------------------------------------------------------------------------- */
bool is_name_valid(const char *name_ptr)
{
    if (!isalpha(*name_ptr) && !(*name_ptr & 0x80)) {
        bs_log(BS_ERROR, "Argument name must start with [a-zA-Z]: %s",
               name_ptr);
        return false;
    }

    while (*++name_ptr) {
        if (!(*name_ptr & 0x80) &&  // same as: isascii()
            (isalnum(*name_ptr) || '-' == *name_ptr || '_' == *name_ptr)) {
            continue;
        }
        bs_log(BS_ERROR, "Argument name must only contain [-_a-zA-Z0-9]: %s",
               name_ptr);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
bool lookup_enum(const bs_arg_enum_table_t *lookup_table,
                 const char *name_ptr,
                 uint32_t *value_ptr)
{
    for (;
         NULL != lookup_table->name_ptr;
         ++lookup_table) {
        if (0 == strcmp(lookup_table->name_ptr, name_ptr)) {
            *value_ptr = lookup_table->value;
            return true;
        }
    }
    return false;
}

/** == Unit tests ========================================================= */

static void bs_arg_test_get_match_type_bool(bs_test_t *test_ptr);
static void bs_arg_test_get_match_type_nonbool(bs_test_t *test_ptr);
static void bs_arg_test_find_matching_arg(bs_test_t *test_ptr);
static void bs_arg_test_parse_arg_for_bool(bs_test_t *test_ptr);
static void bs_arg_test_parse_arg_for_uint32(bs_test_t *test_ptr);
static void bs_arg_test_parse_arg_for_enum(bs_test_t *test_ptr);
static void bs_arg_test_set_defaults(bs_test_t *test_ptr);
static void bs_arg_test_parse(bs_test_t *test_ptr);
static void bs_arg_test_check_arg(bs_test_t *test_ptr);

const bs_test_case_t          bs_arg_test_cases[] = {
    { 1, "get_match_type for bool values", bs_arg_test_get_match_type_bool },
    { 1, "get_match_type for non-bool values", bs_arg_test_get_match_type_nonbool },
    { 1, "find_matching_args", bs_arg_test_find_matching_arg },
    { 1, "parse_arg_for_bool", bs_arg_test_parse_arg_for_bool },
    { 1, "parse_arg_for_uint32", bs_arg_test_parse_arg_for_uint32 },
    { 1, "parse_arg_for_enum", bs_arg_test_parse_arg_for_enum },
    { 1, "set_defaults", bs_arg_test_set_defaults },
    { 1, "parse", bs_arg_test_parse },
    { 1, "check_arg", bs_arg_test_check_arg },
    { 0, NULL, NULL }
};

static const bs_arg_enum_table_t enum_test_table[] = {
    { "alpha", 1 }, { "bravo", 42 }, { "charlie", 7 }, { NULL, 0 } };

/* -- Verifies get_match_type() with bool args ----------------------------- */
void bs_arg_test_get_match_type_bool(bs_test_t *test_ptr) {
    static const bs_arg_t     bool_arg = BS_ARG_BOOL(
        "novalue", "description", true, NULL );
    const char                *next_arg_ptr = "something";
    const char                *arg_value;

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_NO_MATCH,
        get_match_type(&bool_arg, "--value", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, arg_value);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_MATCH_BOOL,
        get_match_type(&bool_arg, "--novalue", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, arg_value);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_MATCH_BOOL_OVERRIDE_WITH_NO,
        get_match_type(&bool_arg, "--nonovalue", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, arg_value);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_NO_MATCH,
        get_match_type(&bool_arg, "--nononovalue", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, arg_value);
}

/* -- Verifies get_match_type() with non-bool args ------------------------- */
void bs_arg_test_get_match_type_nonbool(bs_test_t *test_ptr) {
    static const bs_arg_t     uint32_arg = BS_ARG_UINT32(
        "value", "description", 42, 0, UINT32_MAX, NULL );
    const char                *next_arg_ptr = "1234";
    const char                *arg_value;

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_NO_MATCH,
        get_match_type(&uint32_arg, "--other", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, arg_value);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_MATCH_WITH_EQUAL_SIGN,
        get_match_type(&uint32_arg, "--value=4321", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, 0, strcmp(arg_value, "4321"));

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_MATCH_WITH_EQUAL_SIGN,
        get_match_type(&uint32_arg, "--value=", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, '\0', *arg_value);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_MATCH_WITH_TWO_ARGS,
        get_match_type(&uint32_arg, "--value", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, next_arg_ptr, arg_value);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        _BS_ARG_NO_MATCH,
        get_match_type(&uint32_arg, "--novalue", next_arg_ptr, &arg_value));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, arg_value);

}

/* ------------------------------------------------------------------------- */
void bs_arg_test_find_matching_arg(bs_test_t *test_ptr) {
    static const bs_arg_t     args[] = {
        BS_ARG_BOOL("b", "d", true, NULL),
        BS_ARG_UINT32("u32", "d", 42, 0, UINT32_MAX, NULL),
        BS_ARG_SENTINEL(),
    };
    const bs_arg_t            *matching_arg;
    const char                *av_ptr;

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        find_matching_arg(args, "--b", NULL, &matching_arg, &av_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, &args[0], matching_arg);
    BS_TEST_VERIFY_STREQ(test_ptr, "true", av_ptr);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        find_matching_arg(args, "--nob", NULL, &matching_arg, &av_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, &args[0], matching_arg);
    BS_TEST_VERIFY_STREQ(test_ptr, "false", av_ptr);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        find_matching_arg(args, "--u32=123", NULL, &matching_arg, &av_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, &args[1], matching_arg);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        find_matching_arg(args, "--u32", "456", &matching_arg, &av_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, &args[1], matching_arg);
    BS_TEST_VERIFY_STREQ(test_ptr, "456", av_ptr);

    BS_TEST_VERIFY_FALSE(
        test_ptr,
        find_matching_arg(args, "--u32", NULL, &matching_arg, &av_ptr));

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        find_matching_arg(args, "--unknown", NULL, &matching_arg, &av_ptr));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, matching_arg);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, av_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_arg_test_parse_arg_for_bool(bs_test_t *test_ptr)
{
    static bool               value;
    static const bs_arg_t     arg = BS_ARG_BOOL("b", "d", true, &value );

    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg, "true"));
    BS_TEST_VERIFY_EQ(test_ptr, true, value);

    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg, "false"));
    BS_TEST_VERIFY_EQ(test_ptr, false, value);

    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "meh"));
    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "truea"));
    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "falsea"));
}

/* ------------------------------------------------------------------------- */
void bs_arg_test_parse_arg_for_uint32(bs_test_t *test_ptr)
{
    static uint32_t           value;
    static const bs_arg_t     arg = BS_ARG_UINT32(
        "u32", "d", 42, 0, UINT32_MAX, &value);
    static const bs_arg_t     arg_limited = BS_ARG_UINT32(
        "u32", "d", 42, 10, 100, &value);


    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg, "0"));
    BS_TEST_VERIFY_EQ(test_ptr, 0, value);
    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg, "4294967295"));
    BS_TEST_VERIFY_EQ(test_ptr, UINT32_MAX, value);

    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "999999999999999999999"));
    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "4294967296"));
    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "12a"));
    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "a"));

    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg_limited, "10"));
    BS_TEST_VERIFY_EQ(test_ptr, 10, value);
    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg_limited, "100"));
    BS_TEST_VERIFY_EQ(test_ptr, 100, value);

    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg_limited, "9"));
    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg_limited, "101"));
}

/* ------------------------------------------------------------------------- */
void bs_arg_test_parse_arg_for_enum(bs_test_t *test_ptr)
{
    static uint32_t           value;
    static const bs_arg_t     arg = BS_ARG_ENUM(
        "e", "d", "alpha", enum_test_table, &value);

    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg, "alpha"));
    BS_TEST_VERIFY_EQ(test_ptr, 1, value);
    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg, "bravo"));
    BS_TEST_VERIFY_EQ(test_ptr, 42, value);
    BS_TEST_VERIFY_TRUE(test_ptr, parse_arg(&arg, "charlie"));
    BS_TEST_VERIFY_EQ(test_ptr, 7, value);

    BS_TEST_VERIFY_FALSE(test_ptr, parse_arg(&arg, "delta"));
}

/* ------------------------------------------------------------------------- */
void bs_arg_test_set_defaults(bs_test_t *test_ptr)
{
    static bool               value_bool;
    static uint32_t           value_uint32;
    static uint32_t           value_enum;
    static const bs_arg_t     args[] = {
        BS_ARG_BOOL("b", "d", true, &value_bool),
        BS_ARG_UINT32("u32", "d", 42, 0, UINT32_MAX, &value_uint32),
        BS_ARG_ENUM("e", "d", "alpha", enum_test_table, &value_enum),
        BS_ARG_SENTINEL(),
    };

    value_bool = false;
    value_uint32 = 0;

    set_all_defaults(args);

    BS_TEST_VERIFY_EQ(test_ptr, true, value_bool);
    BS_TEST_VERIFY_EQ(test_ptr, 42, value_uint32);
    BS_TEST_VERIFY_EQ(test_ptr, 1, value_enum);
}

/* ------------------------------------------------------------------------- */
void bs_arg_test_parse(bs_test_t *test_ptr)
{
    static bool               value_bool;
    static uint32_t           value_uint32;
    static uint32_t           value_enum;
    static char               *value_string = NULL;
    static const bs_arg_t     args[] = {
        BS_ARG_BOOL("b", "d", true, &value_bool),
        BS_ARG_ENUM("e", "d", "alpha", enum_test_table, &value_enum),
        BS_ARG_UINT32("u32", "d", 42, 0, UINT32_MAX, &value_uint32),
        BS_ARG_STRING("str", "d", "bravo", &value_string),
        BS_ARG_SENTINEL(),
    };
    const char *argv[] = { "program" };
    int argc = 1;

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_arg_parse(args, BS_ARG_MODE_NO_EXTRA, &argc, argv));
    BS_TEST_VERIFY_EQ(test_ptr, true, value_bool);
    BS_TEST_VERIFY_EQ(test_ptr, 42, value_uint32);
    BS_TEST_VERIFY_EQ(test_ptr, 1, argc);
    BS_TEST_VERIFY_STREQ(test_ptr, "bravo", value_string);
    bs_arg_cleanup(args);

    const char *argv1[] = {
        "program", "x", "--nob", "--u32", "1234", "y", "--e", "bravo",
        "--str", "charlie" };
    argc = sizeof(argv1) / sizeof(const char*);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_arg_parse(args, BS_ARG_MODE_EXTRA_VALUES, &argc, argv1));
    BS_TEST_VERIFY_EQ(test_ptr, false, value_bool);
    BS_TEST_VERIFY_EQ(test_ptr, 1234, value_uint32);
    BS_TEST_VERIFY_EQ(test_ptr, 3, argc);
    BS_TEST_VERIFY_STREQ(test_ptr, "x", argv1[1]);
    BS_TEST_VERIFY_STREQ(test_ptr, "y", argv1[2]);
    BS_TEST_VERIFY_EQ(test_ptr, 42, value_enum);
    BS_TEST_VERIFY_STREQ(test_ptr, "charlie", value_string);
    bs_arg_cleanup(args);

    const char *argv2[] = { "program", "--u32=4321", "1234" };
    argc = sizeof(argv2) / sizeof(const char*);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_arg_parse(args, BS_ARG_MODE_EXTRA_ARGS, &argc, argv2));
    BS_TEST_VERIFY_EQ(test_ptr, 4321, value_uint32);
    BS_TEST_VERIFY_EQ(test_ptr, 2, argc);
    BS_TEST_VERIFY_STREQ(test_ptr, "1234", argv2[1]);
    bs_arg_cleanup(args);

    const char *argv3[] = { "program", "--u32" };
    argc = sizeof(argv3) / sizeof(const char*);
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bs_arg_parse(args, BS_ARG_MODE_EXTRA_ARGS, &argc, argv3));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, value_string);

    const char *argv4[] = { "program", "--unknown=123" };
    argc = sizeof(argv4) / sizeof(const char*);
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bs_arg_parse(args, BS_ARG_MODE_NO_EXTRA, &argc, argv4));
    BS_TEST_VERIFY_EQ(test_ptr, 2, argc);
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bs_arg_parse(args, BS_ARG_MODE_EXTRA_VALUES, &argc, argv4));
    BS_TEST_VERIFY_EQ(test_ptr, 2, argc);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_arg_parse(args, BS_ARG_MODE_EXTRA_ARGS, &argc, argv4));
    BS_TEST_VERIFY_EQ(test_ptr, 2, argc);
    bs_arg_cleanup(args);
}

/* ------------------------------------------------------------------------- */
static void bs_arg_test_check_arg(bs_test_t *test_ptr)
{
    static bool               value_bool;
    static uint32_t           value_uint32;
    static uint32_t           value_enum;
    static char               *value_string;

    // A NULL name.
    static const bs_arg_t     args1[] = {
        BS_ARG_BOOL(NULL, "d", true, &value_bool),
        BS_ARG_SENTINEL(),
    };
    BS_TEST_VERIFY_FALSE(test_ptr, check_arg(args1));

    // Not starting with a-zA-Z.
    static const bs_arg_t     args2[] = {
        BS_ARG_BOOL(NULL, "9", true, &value_bool),
        BS_ARG_SENTINEL(),
    };
    BS_TEST_VERIFY_FALSE(test_ptr, check_arg(args2));

    // Invalid characters.
    static const bs_arg_t     args3[] = {
        BS_ARG_BOOL(NULL, "a-b.", true, &value_bool),
        BS_ARG_SENTINEL(),
    };
    BS_TEST_VERIFY_FALSE(test_ptr, check_arg(args3));

    // Duplicate names.
    static const bs_arg_t     args4[] = {
        BS_ARG_BOOL("b", "d", true, &value_bool),
        BS_ARG_BOOL("b", "e", false, &value_bool),
        BS_ARG_SENTINEL(),
    };
    BS_TEST_VERIFY_FALSE(test_ptr, check_arg(args4));

    // Duplicate names, with the boolean extension.
    static const bs_arg_t     args5[] = {
        BS_ARG_BOOL("b", "d", true, &value_bool),
        BS_ARG_BOOL("nob", "e", false, &value_bool),
        BS_ARG_SENTINEL(),
    };
    BS_TEST_VERIFY_FALSE(test_ptr, check_arg(args5));

    // All valid.
    value_string = NULL;
    static const bs_arg_t     valid_args[] = {
        BS_ARG_BOOL("b", "d", true, &value_bool),
        BS_ARG_ENUM("e", "d", "alpha", enum_test_table, &value_enum),
        BS_ARG_STRING("s", "d", "default", &value_string),
        BS_ARG_UINT32("u32", "d", 42, 0, UINT32_MAX, &value_uint32),
        BS_ARG_SENTINEL(),
    };
    BS_TEST_VERIFY_TRUE(test_ptr, check_arg(valid_args));
}

/* == End of arg.c ========================================================= */
