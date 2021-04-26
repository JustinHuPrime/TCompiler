// Copyright 2020-2021 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * tests for the lexer
 */

#include "lexer/lexer.h"

#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "fileList.h"
#include "tests.h"

static void testAllTokens(void) {
  FileListEntry entry;  // forge the entry
  entry.inputFilename = "testFiles/lexer/allTokens.tc";
  entry.isCode = true;
  entry.errored = false;

  test("lexer initializes okay", lexerStateInit(&entry) == 0);

  TokenType const types[] = {
      TT_MODULE,        TT_IMPORT,       TT_OPAQUE,
      TT_STRUCT,        TT_UNION,        TT_ENUM,
      TT_TYPEDEF,       TT_IF,           TT_ELSE,
      TT_WHILE,         TT_DO,           TT_FOR,
      TT_SWITCH,        TT_CASE,         TT_DEFAULT,
      TT_BREAK,         TT_CONTINUE,     TT_RETURN,
      TT_ASM,           TT_CAST,         TT_SIZEOF,
      TT_TRUE,          TT_FALSE,        TT_NULL,
      TT_VOID,          TT_UBYTE,        TT_BYTE,
      TT_CHAR,          TT_USHORT,       TT_SHORT,
      TT_UINT,          TT_INT,          TT_WCHAR,
      TT_ULONG,         TT_LONG,         TT_FLOAT,
      TT_DOUBLE,        TT_BOOL,         TT_CONST,
      TT_VOLATILE,      TT_SEMI,         TT_COMMA,
      TT_LPAREN,        TT_RPAREN,       TT_LSQUARE,
      TT_RSQUARE,       TT_LBRACE,       TT_RBRACE,
      TT_DOT,           TT_ARROW,        TT_INC,
      TT_DEC,           TT_STAR,         TT_AMP,
      TT_PLUS,          TT_MINUS,        TT_BANG,
      TT_TILDE,         TT_NEGASSIGN,    TT_LNOTASSIGN,
      TT_BITNOTASSIGN,  TT_SLASH,        TT_PERCENT,
      TT_LSHIFT,        TT_ARSHIFT,      TT_LRSHIFT,
      TT_SPACESHIP,     TT_LANGLE,       TT_RANGLE,
      TT_LTEQ,          TT_GTEQ,         TT_EQ,
      TT_NEQ,           TT_BAR,          TT_CARET,
      TT_LAND,          TT_LOR,          TT_QUESTION,
      TT_COLON,         TT_ASSIGN,       TT_MULASSIGN,
      TT_DIVASSIGN,     TT_MODASSIGN,    TT_ADDASSIGN,
      TT_SUBASSIGN,     TT_LSHIFTASSIGN, TT_ARSHIFTASSIGN,
      TT_LRSHIFTASSIGN, TT_BITANDASSIGN, TT_BITXORASSIGN,
      TT_BITORASSIGN,   TT_LANDASSIGN,   TT_LORASSIGN,
      TT_SCOPE,         TT_ID,           TT_ID,
      TT_LIT_STRING,    TT_LIT_WSTRING,  TT_LIT_CHAR,
      TT_LIT_WCHAR,     TT_LIT_INT_D,    TT_LIT_INT_H,
      TT_LIT_INT_B,     TT_LIT_INT_O,    TT_LIT_INT_0,
      TT_LIT_DOUBLE,    TT_LIT_FLOAT,    TT_LIT_STRING,
      TT_LIT_INT_D,     TT_LIT_STRING,   TT_EOF,
  };
  size_t const characters[] = {
      1,  8,  15, 22, 29, 35, 40, 48, 51, 56, 62, 65, 69, 76,

      1,  9,  15, 24, 31, 35, 40, 47, 52, 58, 63, 68, 74,

      1,  6,  13, 19, 24, 28, 34, 40, 45, 51, 58, 63, 69,

      1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 12, 14, 16, 17, 18, 19,
      20, 21, 22, 24, 26, 28, 29, 30, 32, 35, 38, 41, 42, 43, 46, 48,
      50, 52, 53, 54, 56, 58, 59, 60, 61, 63, 65, 67, 69, 71, 74,

      1,  5,  7,  9,  11, 14, 17,

      1,  30,

      23, 39, 57, 60, 64, 66, 71, 74,

      1,  3,  6,

      1,  10, 19, 30,
  };
  size_t const lines[] = {
      1,  1,  1,  1,  1,  1,  1,  1,  1, 1, 1, 1, 1, 1,

      2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2,

      3,  3,  3,  3,  3,  3,  3,  3,  3, 3, 3, 3, 3,

      5,  5,  5,  5,  5,  5,  5,  5,  5, 5, 5, 5, 5, 5, 5, 5,
      5,  5,  5,  5,  5,  5,  5,  5,  5, 5, 5, 5, 5, 5, 5, 5,
      5,  5,  5,  5,  5,  5,  5,  5,  5, 5, 5, 5, 5, 5, 5,

      6,  6,  6,  6,  6,  6,  6,

      8,  8,

      10, 10, 10, 10, 10, 10, 10, 10,

      14, 14, 14,

      16, 16, 16, 16,
  };
  char const *const strings[] = {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,

      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,

      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,

      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,

      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,

      "identifier",
      "identifier2",

      "string literal",
      "wstring literal",
      "c",
      "w",
      "+1",
      "-0xf",
      "0b1",
      "+0377",

      "0",
      "1.1",
      "+1.1f",

      "testFiles/lexer/allTokens.tc",
      "16",
      "T Language Compiler (tlc) version 0.2.0",
      NULL,
  };
  size_t const numTokens = sizeof(types) / sizeof(TokenType);

  bool errorFlagOK = true;
  bool typeOK = true;
  bool characterOK = true;
  bool lineOK = true;
  bool additionalDataOK = true;
  char const *messageString = "everything";
  for (size_t idx = 0; idx < numTokens; ++idx) {
    Token token;
    lex(&entry, &token);

    if (entry.errored) {
      errorFlagOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.type != types[idx]) {
      typeOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.character != characters[idx]) {
      characterOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.line != lines[idx]) {
      lineOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if ((strings[idx] == NULL) != (token.string == NULL)) {
      additionalDataOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (strings[idx] != NULL && token.string != NULL &&
        strcmp(strings[idx], token.string) != 0) {
      additionalDataOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }

    if (token.string != NULL) free(token.string);
    entry.errored = false;
  }
  testDynamic(format("lex accepts token for %s", messageString), errorFlagOK);
  testDynamic(format("token has expected type for %s", messageString), typeOK);
  testDynamic(format("token is at expected character for %s", messageString),
              characterOK);
  testDynamic(format("token is at expected line for %s", messageString),
              lineOK);
  testDynamic(format("token has correct additional data for %s", messageString),
              additionalDataOK);

  lexerStateUninit(&entry);
}

static void testErrors(void) {
  Token token;

  FileListEntry entry;  // forge the entry
  entry.inputFilename = "testFiles/lexer/errors.tc";
  entry.isCode = true;
  entry.errored = false;

  test("lexer initializes okay", lexerStateInit(&entry) == 0);

  bool const errors[] = {
      true,  false, true,  false, true,  false, true,  false, true,
      false, true,  false, true,  false, true,  false, true,  false,
      true,  false, true,  false, true,  false, true,  false, true,
      false, true,  false, true,  false, true,  true,
  };
  TokenType const types[] = {
      TT_BAD_HEX,    TT_SEMI, TT_BAD_BIN,    TT_SEMI, TT_LIT_WSTRING, TT_SEMI,
      TT_BAD_STRING, TT_SEMI, TT_BAD_STRING, TT_SEMI, TT_BAD_STRING,  TT_SEMI,
      TT_LIT_STRING, TT_SEMI, TT_BAD_CHAR,   TT_SEMI, TT_BAD_CHAR,    TT_SEMI,
      TT_BAD_CHAR,   TT_SEMI, TT_BAD_CHAR,   TT_SEMI, TT_BAD_CHAR,    TT_SEMI,
      TT_LIT_CHAR,   TT_SEMI, TT_BAD_CHAR,   TT_SEMI, TT_LIT_CHAR,    TT_SEMI,
      TT_LIT_WCHAR,  TT_SEMI, TT_SEMI,       TT_EOF,
  };
  size_t const characters[] = {
      1, 3, 1, 3, 1, 13, 1, 7, 1, 7, 1, 5, 1, 1, 1,  3, 1,
      7, 1, 7, 1, 5, 1,  1, 1, 1, 1, 5, 1, 6, 1, 13, 2, 30,
  };
  size_t const lines[] = {
      1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  8,  9,  9,  10,
      10, 11, 11, 12, 12, 13, 14, 15, 16, 17, 17, 18, 18, 19, 19, 20, 22,
  };
  char const *const strings[] = {
      NULL, NULL, NULL,          NULL, "\\u00000000", NULL, NULL,
      NULL, NULL, NULL,          NULL, NULL,          "",   NULL,
      NULL, NULL, NULL,          NULL, NULL,          NULL, NULL,
      NULL, NULL, NULL,          "a",  NULL,          NULL, NULL,
      "a",  NULL, "\\u00000000", NULL, NULL,          NULL,
  };
  size_t const numTokens = sizeof(types) / sizeof(TokenType);

  bool errorFlagOK = true;
  bool typeOK = true;
  bool characterOK = true;
  bool lineOK = true;
  bool additionalDataOK = true;
  char const *messageString = "everything";
  for (size_t idx = 0; idx < numTokens; ++idx) {
    lex(&entry, &token);

    if (entry.errored != errors[idx]) {
      errorFlagOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.type != types[idx]) {
      typeOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.character != characters[idx]) {
      characterOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.line != lines[idx]) {
      lineOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if ((strings[idx] == NULL) != (token.string == NULL)) {
      additionalDataOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (strings[idx] != NULL && token.string != NULL &&
        strcmp(strings[idx], token.string) != 0) {
      additionalDataOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }

    if (token.string != NULL) free(token.string);
    entry.errored = false;
  }
  testDynamic(format("token has expected error flag for %s", messageString),
              errorFlagOK);
  testDynamic(format("token has expected type for %s", messageString), typeOK);
  testDynamic(format("token is at expected character for %s", messageString),
              characterOK);
  testDynamic(format("token is at expected line for %s", messageString),
              lineOK);
  testDynamic(format("token has correct additional data for %s", messageString),
              additionalDataOK);

  lexerStateUninit(&entry);

  entry.inputFilename = "testFiles/lexer/unterminatedCharLit.tc";

  test("lexer initializes okay", lexerStateInit(&entry) == 0);

  lex(&entry, &token);
  test("unterminated char literal is an error", entry.errored == true);
  test("unterminated char literal is bad char", token.type == TT_BAD_CHAR);
  test("unterminated char literal is at expected character",
       token.character == 1);
  test("unterminated char literal is at expected line", token.line == 1);
  test("unterminated char literal has no additional data",
       token.string == NULL);
  entry.errored = false;

  lex(&entry, &token);
  test("token after unterminated char literal is accepted",
       entry.errored == false);
  test("token after unterminated char literal is eof", token.type == TT_EOF);
  test("token after unterminated char literal is at expected character",
       token.character == 2);
  test("token after unterminated char literal is at expected line",
       token.line == 1);

  lexerStateUninit(&entry);

  entry.inputFilename = "testFiles/lexer/unterminatedStringLit.tc";

  test("lexer initializes okay", lexerStateInit(&entry) == 0);

  lex(&entry, &token);
  test("unterminated string literal is an error", entry.errored == true);
  test("unterminated string literal is string", token.type == TT_LIT_STRING);
  test("unterminated string literal is at expected character",
       token.character == 1);
  test("unterminated string literal is at expected line", token.line == 1);
  test("unterminated string literal's additional data is correct",
       strcmp(token.string, "") == 0);
  free(token.string);
  entry.errored = false;

  lex(&entry, &token);
  test("token after unterminated string literal is accepted",
       entry.errored == false);
  test("token after unterminated string literal is eof", token.type == TT_EOF);
  test("token after unterminated string literal is at expected character",
       token.character == 2);
  test("token after unterminated string literal is at expected line",
       token.line == 1);

  lexerStateUninit(&entry);
}

void testLexer(void) {
  lexerInitMaps();

  testAllTokens();
  testErrors();

  lexerUninitMaps();
}