/* ========================================================================= */
/**
 * @file parse.c
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

#include "grammar.h"
#include "analyzer.h"

#include "parser_context.h"
#include <libbase/libbase.h>
#include <libbase/plist.h>
#include <stdint.h>
#include <stdio.h>

/* == Declarations ========================================================= */

static bspl_object_t *_bspl_create_object_from_plist_scanner(
    yyscan_t scanner);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_create_object_from_plist_string(const char *buf_ptr)
{
    yyscan_t scanner;
    yylex_init(&scanner);

    YY_BUFFER_STATE buf_state;
    buf_state = yy_scan_string(buf_ptr, scanner);
    yy_switch_to_buffer(buf_state, scanner);

    bspl_object_t *obj = _bspl_create_object_from_plist_scanner(scanner);

    yy_delete_buffer(buf_state, scanner);
    yylex_destroy(scanner);

    return obj;
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_create_object_from_plist_data(
    const uint8_t *data_ptr, size_t data_size)
{
    yyscan_t scanner;
    yylex_init(&scanner);

    YY_BUFFER_STATE buf_state;
    buf_state = yy_scan_bytes((const char*)data_ptr, data_size, scanner);
    yy_switch_to_buffer(buf_state, scanner);

    bspl_object_t *obj = _bspl_create_object_from_plist_scanner(scanner);

    yy_delete_buffer(buf_state, scanner);
    yylex_destroy(scanner);

    return obj;
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_create_object_from_plist_file(const char *fname_ptr)
{
    FILE *file_ptr = fopen(fname_ptr, "r");
    if (NULL == file_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed fopen(%s, 'r')", fname_ptr);
        return NULL;
    }

    yyscan_t scanner;
    yylex_init(&scanner);

    YY_BUFFER_STATE buf_state;
    buf_state = yy_create_buffer(file_ptr, YY_BUF_SIZE, scanner);
    yy_switch_to_buffer(buf_state, scanner);

    bspl_object_t *obj = _bspl_create_object_from_plist_scanner(scanner);
    yy_delete_buffer(buf_state, scanner);
    yylex_destroy(scanner);
    fclose(file_ptr);

    return obj;
}

/* ------------------------------------------------------------------------- */
bspl_object_t *bspl_create_object_from_dynbuf(bs_dynbuf_t *dynbuf_ptr)
{
    return bspl_create_object_from_plist_data(
        dynbuf_ptr->data_ptr,
        dynbuf_ptr->length);
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/** Does a parser run, from the initialized scanner. */
bspl_object_t *_bspl_create_object_from_plist_scanner(yyscan_t scanner)
{
    bspl_parser_context_t ctx = {};
    if (!bs_ptr_stack_init(&ctx.object_stack)) return NULL;
    // TODO(kaeser@gubbe.ch): Clean up stack on error!
    yyset_extra(&ctx, scanner);
    int rv = yyparse(scanner, &ctx);
    bspl_object_t *object_ptr = bs_ptr_stack_pop(&ctx.object_stack);
    bs_ptr_stack_fini(&ctx.object_stack);

    if (0 != rv) {
        bspl_object_unref(object_ptr);
        object_ptr = NULL;
    }
    return object_ptr;
}

/* == Unit tests =========================================================== */

static void test_from_string(bs_test_t *test_ptr);
static void test_from_file(bs_test_t *test_ptr);
static void test_from_data(bs_test_t *test_ptr);
static void test_from_dynbuf(bs_test_t *test_ptr);
static void test_escaped_string(bs_test_t *test_ptr);

const bs_test_case_t bspl_plist_test_cases[] = {
    { 1, "from_string", test_from_string },
    { 1, "from_file", test_from_file },
    { 1, "from_data", test_from_data },
    { 1, "from_dynbuf", test_from_dynbuf },
    { 1, "escaped_string", test_escaped_string },
    { 0, NULL, NULL }
};

static const uint8_t _test_data[] = { 'v', 'a', 'l', 'u', 'e' };

/* ------------------------------------------------------------------------- */
/** Tests plist object creation from string. */
void test_from_string(bs_test_t *test_ptr)
{
    bspl_object_t *object_ptr, *v_ptr;
    bspl_array_t *array_ptr;
    bspl_dict_t *dict_ptr;

    // A string.
    object_ptr = bspl_create_object_from_plist_string("value");
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, object_ptr);
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "value",
        bspl_string_value(bspl_string_from_object(object_ptr)));
    bspl_object_unref(object_ptr);

    // A string that should be quoted.
    object_ptr = bspl_create_object_from_plist_string("va:lue");
    BS_TEST_VERIFY_EQ(test_ptr, NULL, object_ptr);

    // A dict.
    object_ptr = bspl_create_object_from_plist_string(
        "{key1=dict_value1;key2=dict_value2}");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, object_ptr);
    dict_ptr = bspl_dict_from_object(object_ptr);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, dict_ptr);
    v_ptr = bspl_dict_get(dict_ptr, "key1");
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "dict_value1",
        bspl_string_value(bspl_string_from_object(v_ptr)));
    v_ptr = bspl_dict_get(dict_ptr, "key2");
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "dict_value2",
        bspl_string_value(bspl_string_from_object(v_ptr)));
    bspl_object_unref(object_ptr);

    // A dict, with semicolon at the end.
    object_ptr = bspl_create_object_from_plist_string(
        "{key1=dict_value1;key2=dict_value2;}");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, bspl_dict_from_object(object_ptr));
    bspl_object_unref(object_ptr);

    // A dict with a duplicate key. Will return NULL, no need to unref.
    object_ptr = bspl_create_object_from_plist_string(
        "{key1=dict_value1;key1=dict_value2}");
    BS_TEST_VERIFY_EQ(test_ptr, NULL, object_ptr);

    // An empty dict.
    object_ptr = bspl_create_object_from_plist_string("{}");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, object_ptr);
    bspl_object_unref(object_ptr);

    // An array.
    object_ptr = bspl_create_object_from_plist_string(
        "(elem0,elem1)");
    array_ptr = bspl_array_from_object(object_ptr);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, array_ptr);
    if (NULL != array_ptr) {
        BS_TEST_VERIFY_STREQ(
            test_ptr,
            "elem0",
            bspl_string_value(bspl_string_from_object(
                                    bspl_array_at(array_ptr, 0))));
        BS_TEST_VERIFY_STREQ(
            test_ptr,
            "elem1",
            bspl_string_value(bspl_string_from_object(
                                    bspl_array_at(array_ptr, 1))));
    }
    bspl_object_unref(object_ptr);

    // An array with a comma at the end.
    object_ptr = bspl_create_object_from_plist_string(
        "(elem0,elem1,)");
    array_ptr = bspl_array_from_object(object_ptr);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, array_ptr);
    bspl_object_unref(object_ptr);

    // An empty array.
    object_ptr = bspl_create_object_from_plist_string("()");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, object_ptr);
    array_ptr = bspl_array_from_object(object_ptr);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, array_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, 0, bspl_array_size(array_ptr));
    bspl_object_unref(object_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests plist object creation from string. */
void test_from_file(bs_test_t *test_ptr)
{
    bspl_object_t *object_ptr, *v_ptr;

    object_ptr = bspl_create_object_from_plist_file(
        bs_test_resolve_path("string.plist"));
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, object_ptr);
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "file_value",
        bspl_string_value(bspl_string_from_object(object_ptr)));
    bspl_object_unref(object_ptr);

    object_ptr = bspl_create_object_from_plist_file(
        bs_test_resolve_path("dict.plist"));
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, object_ptr);
    bspl_dict_t *dict_ptr = bspl_dict_from_object(object_ptr);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, dict_ptr);

    v_ptr = BS_ASSERT_NOTNULL(bspl_dict_get(dict_ptr, "key0"));
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "value0",
        bspl_string_value(bspl_string_from_object(v_ptr)));

    v_ptr = BS_ASSERT_NOTNULL(bspl_dict_get(dict_ptr, "quoted+key"));
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "value",
        bspl_string_value(bspl_string_from_object(v_ptr)));

    bspl_object_unref(object_ptr);

    object_ptr = bspl_create_object_from_plist_file(
        bs_test_resolve_path("array.plist"));
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, object_ptr);
    bspl_array_t *array_ptr = bspl_array_from_object(object_ptr);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, array_ptr);
    bspl_object_unref(object_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests plist object creation from a sized data buffer. */
void test_from_data(bs_test_t *test_ptr)
{
    // A string.
    bspl_object_t *object_ptr = bspl_create_object_from_plist_data(
        _test_data, sizeof(_test_data));
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, object_ptr);
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "value",
        bspl_string_value(bspl_string_from_object(object_ptr)));
    bspl_object_unref(object_ptr);
}

/* ------------------------------------------------------------------------- */
/** Tests plist object creation from a dynamic data buffer. */
void test_from_dynbuf(bs_test_t *test_ptr)
{
    bs_dynbuf_t dynbuf = {
        .data_ptr = (void*)_test_data,
        .length  = sizeof(_test_data)
    };
    bspl_object_t *object_ptr = bspl_create_object_from_dynbuf(&dynbuf);

    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, object_ptr);
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "value",
        bspl_string_value(bspl_string_from_object(object_ptr)));
    bspl_object_unref(object_ptr);
}

/* ------------------------------------------------------------------------- */
void test_escaped_string(bs_test_t *test_ptr)
{
    bspl_object_t *o;
    // A string with escaped backslash and double quote.
    o = bspl_create_object_from_plist_string("\"backslash\\\\dquote\\\"end\"");
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, o);
    BS_TEST_VERIFY_STREQ(
        test_ptr,
        "backslash\\dquote\"end",
        bspl_string_value(bspl_string_from_object(o)));
    bspl_object_unref(o);
}

/* == End of plist.c ======================================================= */
