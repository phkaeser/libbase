/* ========================================================================= */
/**
 * @file analyzer.l
 *
 * See https://westes.github.io/flex/manual/.
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

%{
#include <libbase/libbase.h>
#include <unistd.h>  // IWYU pragma: keep

#include "grammar.h"

struct yyguts_t;  // For IWYU.

/** Addresses valgrind unitialized memory warnings. */
#define YY_USER_INIT do {                               \
  yylineno = 1; yycolumn = 0; yyleng = 0;               \
  memset(yylloc, 0, sizeof(*yylloc));                   \
} while (0);

/** Permits location tracking, for positioned error reports. */
#define YY_USER_ACTION                                  \
do {                                                    \
  yylloc->first_line = yylloc->last_line = yylineno;    \
  yylloc->first_column = yycolumn - 1;                  \
  yylloc->last_column = yycolumn + yyleng - 1;          \
  yycolumn += yyleng;                                   \
} while (0);

%}

/* == Definitions section ===================================================*/

%option batch
%option bison-bridge
%option bison-locations
%option never-interactive
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option reentrant
%option yylineno

ws                  [[:blank:]\r\n]

/* == Rules section ======================================================== */
%%

{ws}+               { /* whitespace */ }

"//".*              { /* comment. */ }

\"[^\"]*\"          { yylval->string = logged_strdup(yytext);
                      return TK_QUOTED_STRING; }
[a-zA-Z0-9_.$]+     { yylval->string = logged_strdup(yytext);
                      return TK_STRING; }
"("                 { return TK_LPAREN; }
")"                 { return TK_RPAREN; }
"{"                 { return TK_LBRACE; }
"}"                 { return TK_RBRACE; }
","                 { return TK_COMMA; }
"="                 { return TK_EQUAL; }
";"                 { return TK_SEMICOLON; }

.                   { char msg[256];
                      snprintf(msg, sizeof(msg), "Unexpected character: '%s'",
                               yytext);
                      yyerror(yylloc, yyscanner, yyextra, msg);
                      return YYerror; }

%%
/* == User code section ==================================================== */


/* == End of analyzer.l ==================================================== */
