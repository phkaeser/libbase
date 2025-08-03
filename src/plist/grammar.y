/* ========================================================================= */
/**
 * @file grammar.y
 *
 * See https://www.gnu.org/software/bison/manual/bison.html.
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

/* == Prologue ============================================================= */
%{

#include "grammar.h"  // IWYU pragma: keep
#include "analyzer.h"  // IWYU pragma: keep

#include <libbase/libbase.h>  // IWYU pragma: keep
#include <libbase/plist.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

%}

/* == Bison declarations =================================================== */

%union{
    char *string;
}

%locations
%define parse.error verbose

%define api.pure full
%parse-param { void* scanner }
%parse-param { bspl_parser_context_t *ctx_ptr }
%lex-param { yyscan_t scanner }

%code requires {
#include "parser_context.h"

struct YYLTYPE;  // For IWYU.
}

%code provides {
    extern int yyerror(
        YYLTYPE *loc_ptr,
        void* scanner,
        bspl_parser_context_t *ctx_ptr,
        const char* msg_ptr);
}

%token TK_LPAREN "("
%token TK_RPAREN ")"
%token TK_LBRACE "{"
%token TK_RBRACE "}"
%token TK_COMMA ","
%token TK_EQUAL "="
%token TK_SEMICOLON ";"
%token <string> TK_IDENTIFIER_STRING "identifier string"
%token <string> TK_QUOTED_STRING "quoted string"

%destructor { free($$); } <string>

%%
/* == Grammar rules ======================================================== */
/* See https://code.google.com/archive/p/networkpx/wikis/PlistSpec.wiki. */

start:          object;

object:         string
                | dict
                | array;

string:         TK_IDENTIFIER_STRING {
    bspl_string_t *string_ptr = bspl_string_create($1);
    free($1);
    bs_ptr_stack_push(&ctx_ptr->object_stack,
                      bspl_object_from_string(string_ptr)); }
                | TK_QUOTED_STRING {
    size_t len = strlen($1);
    BS_ASSERT(2 <= len);  // It is supposed to be quoted.
    // Un-escape the escaped backslash and double quotes.
    char *dest_ptr = $1;
    for (size_t i = 1; i < len - 1; ++i, ++dest_ptr) {
        if ($1[i] == '\\') ++i;
        *dest_ptr = $1[i];
    }
    *dest_ptr = '\0';
    bspl_string_t *string_ptr = bspl_string_create($1);
    free($1);
    bs_ptr_stack_push(&ctx_ptr->object_stack,
                      bspl_object_from_string(string_ptr)); };

dict:           TK_LBRACE {
    bspl_dict_t *dict_ptr = bspl_dict_create();
    bs_ptr_stack_push(
        &ctx_ptr->object_stack,
        bspl_object_from_dict(dict_ptr));
                } kv_list TK_RBRACE;

kv_list:        kv TK_SEMICOLON kv_list
                | kv
                | %empty;

kv:             string TK_EQUAL object {
    bspl_object_t *object_ptr = bs_ptr_stack_pop(&ctx_ptr->object_stack);
    bspl_string_t *key_string_ptr = bspl_string_from_object(
        bs_ptr_stack_pop(&ctx_ptr->object_stack));
    const char *key_ptr = bspl_string_value(key_string_ptr);

    bspl_dict_t *dict_ptr = bspl_dict_from_object(
        bs_ptr_stack_peek(&ctx_ptr->object_stack, 0));
    bool rv = bspl_dict_add(dict_ptr, key_ptr, object_ptr);
    if (!rv) {
        // TODO(kaeser@gubbe.ch): Keep this as error in context.
        bs_log(BS_WARNING, "Failed bspl_dict_add(%p, %s, %p)",
               dict_ptr, key_ptr, object_ptr);
    }
    bspl_object_unref(object_ptr);
    bspl_string_unref(key_string_ptr);
    if (!rv) return -1; };

array:          TK_LPAREN {
    bspl_array_t *array_ptr = bspl_array_create();
    bs_ptr_stack_push(
        &ctx_ptr->object_stack,
        bspl_object_from_array(array_ptr));
                } element_list TK_RPAREN;

element_list:   element TK_COMMA element_list
                | element
                | %empty;

element:        object {
    bspl_array_t *array_ptr = bspl_array_from_object(
        bs_ptr_stack_peek(&ctx_ptr->object_stack, 1));
    bspl_object_t *object_ptr = bs_ptr_stack_pop(&ctx_ptr->object_stack);
    bool rv = bspl_array_push_back(array_ptr, object_ptr);
    if (!rv) {
        // TODO(kaeser@gubbe.ch): Keep this as error in context.
        bs_log(BS_WARNING, "Failed bspl_array_push_back(%p, %p)",
               array_ptr, object_ptr);
    }
    bspl_object_unref(object_ptr);
                };

%%
/* == Epilogue ============================================================= */

#include <libbase/libbase.h>

int yyerror(
    YYLTYPE *loc_ptr,
    __UNUSED__ void* scanner,
    __UNUSED__ bspl_parser_context_t *ctx_ptr,
    const char* msg_ptr)
{
    bs_log(BS_ERROR, "Parse error at %d,%d: %s",
           loc_ptr->first_line, loc_ptr->first_column,
           msg_ptr);
    return -1;
}

/* == End of grammar.y ===================================================== */
