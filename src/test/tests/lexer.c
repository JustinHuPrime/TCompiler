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

#include <assert.h>
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

  typedef struct {
    TokenType type;
    size_t const line;
    size_t const character;
    char const *string;
  } TokenRecord;
  TokenRecord const TOKENS[] = {
      {TT_MODULE, 1, 1, NULL},
      {TT_IMPORT, 1, 8, NULL},
      {TT_OPAQUE, 1, 15, NULL},
      {TT_STRUCT, 1, 22, NULL},
      {TT_UNION, 1, 29, NULL},
      {TT_ENUM, 1, 35, NULL},
      {TT_TYPEDEF, 1, 40, NULL},
      {TT_IF, 1, 48, NULL},
      {TT_ELSE, 1, 51, NULL},
      {TT_WHILE, 1, 56, NULL},
      {TT_DO, 1, 62, NULL},
      {TT_FOR, 1, 65, NULL},
      {TT_SWITCH, 1, 69, NULL},
      {TT_CASE, 1, 76, NULL},

      {TT_DEFAULT, 2, 1, NULL},
      {TT_BREAK, 2, 9, NULL},
      {TT_CONTINUE, 2, 15, NULL},
      {TT_RETURN, 2, 24, NULL},
      {TT_ASM, 2, 31, NULL},
      {TT_CAST, 2, 35, NULL},
      {TT_SIZEOF, 2, 40, NULL},
      {TT_TRUE, 2, 47, NULL},
      {TT_FALSE, 2, 52, NULL},
      {TT_NULL, 2, 58, NULL},
      {TT_VOID, 2, 63, NULL},
      {TT_UBYTE, 2, 68, NULL},
      {TT_BYTE, 2, 74, NULL},

      {TT_CHAR, 3, 1, NULL},
      {TT_USHORT, 3, 6, NULL},
      {TT_SHORT, 3, 13, NULL},
      {TT_UINT, 3, 19, NULL},
      {TT_INT, 3, 24, NULL},
      {TT_WCHAR, 3, 28, NULL},
      {TT_ULONG, 3, 34, NULL},
      {TT_LONG, 3, 40, NULL},
      {TT_FLOAT, 3, 45, NULL},
      {TT_DOUBLE, 3, 51, NULL},
      {TT_BOOL, 3, 58, NULL},
      {TT_CONST, 3, 63, NULL},
      {TT_VOLATILE, 3, 69, NULL},

      {TT_SEMI, 5, 1, NULL},
      {TT_COMMA, 5, 2, NULL},
      {TT_LPAREN, 5, 3, NULL},
      {TT_RPAREN, 5, 4, NULL},
      {TT_LSQUARE, 5, 5, NULL},
      {TT_RSQUARE, 5, 6, NULL},
      {TT_LBRACE, 5, 7, NULL},
      {TT_RBRACE, 5, 8, NULL},
      {TT_DOT, 5, 9, NULL},
      {TT_ARROW, 5, 10, NULL},
      {TT_INC, 5, 12, NULL},
      {TT_DEC, 5, 14, NULL},
      {TT_STAR, 5, 16, NULL},
      {TT_AMP, 5, 17, NULL},
      {TT_PLUS, 5, 18, NULL},
      {TT_MINUS, 5, 19, NULL},
      {TT_BANG, 5, 20, NULL},
      {TT_TILDE, 5, 21, NULL},
      {TT_NEGASSIGN, 5, 22, NULL},
      {TT_LNOTASSIGN, 5, 24, NULL},
      {TT_BITNOTASSIGN, 5, 26, NULL},
      {TT_SLASH, 5, 28, NULL},
      {TT_PERCENT, 5, 29, NULL},
      {TT_LSHIFT, 5, 30, NULL},
      {TT_ARSHIFT, 5, 32, NULL},
      {TT_LRSHIFT, 5, 35, NULL},
      {TT_LANGLE, 5, 38, NULL},
      {TT_RANGLE, 5, 39, NULL},
      {TT_LTEQ, 5, 40, NULL},
      {TT_GTEQ, 5, 42, NULL},
      {TT_EQ, 5, 44, NULL},
      {TT_NEQ, 5, 46, NULL},
      {TT_BAR, 5, 48, NULL},
      {TT_CARET, 5, 49, NULL},
      {TT_LAND, 5, 50, NULL},
      {TT_LOR, 5, 52, NULL},
      {TT_QUESTION, 5, 54, NULL},
      {TT_COLON, 5, 55, NULL},
      {TT_ASSIGN, 5, 56, NULL},
      {TT_MULASSIGN, 5, 57, NULL},
      {TT_DIVASSIGN, 5, 59, NULL},
      {TT_MODASSIGN, 5, 61, NULL},
      {TT_ADDASSIGN, 5, 63, NULL},
      {TT_SUBASSIGN, 5, 65, NULL},
      {TT_LSHIFTASSIGN, 5, 67, NULL},
      {TT_ARSHIFTASSIGN, 5, 70, NULL},
      {TT_LRSHIFTASSIGN, 5, 73, NULL},
      {TT_BITANDASSIGN, 5, 77, NULL},

      {TT_BITXORASSIGN, 6, 1, NULL},
      {TT_BITORASSIGN, 6, 3, NULL},
      {TT_LANDASSIGN, 6, 5, NULL},
      {TT_LORASSIGN, 6, 8, NULL},
      {TT_SCOPE, 6, 11, NULL},

      {TT_ID, 8, 1, "identifier"},
      {TT_ID, 8, 30, "identifier2"},

      {TT_LIT_STRING, 10, 23, "string literal"},
      {TT_LIT_WSTRING, 10, 39, "wstring literal"},
      {TT_LIT_CHAR, 10, 57, "c"},
      {TT_LIT_WCHAR, 10, 60, "w"},
      {TT_LIT_INT_D, 10, 64, "+1"},
      {TT_LIT_INT_H, 10, 66, "-0xf"},
      {TT_LIT_INT_B, 10, 71, "0b1"},
      {TT_LIT_INT_O, 10, 74, "+0377"},

      {TT_LIT_INT_0, 14, 1, "0"},
      {TT_LIT_DOUBLE, 14, 3, "1.1"},
      {TT_LIT_FLOAT, 14, 6, "+1.1f"},

      {TT_LIT_STRING, 16, 1, "testFiles/lexer/allTokens.tc"},
      {TT_LIT_INT_D, 16, 10, "16"},
      {TT_LIT_STRING, 16, 19, "T Language Compiler (tlc) version 0.2.0"},
      {TT_EOF, 16, 30, NULL},
  };

  size_t const numTokens = sizeof(TOKENS) / sizeof(TokenRecord);

  bool errorFlagOK = true;
  bool typeOK = true;
  bool characterOK = true;
  bool lineOK = true;
  bool additionalDataOK = true;
  char const *messageString = NULL;
  for (size_t idx = 0; idx < numTokens; ++idx) {
    Token token;
    lex(&entry, &token);

    if (entry.errored) {
      errorFlagOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.type != TOKENS[idx].type) {
      typeOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.character != TOKENS[idx].character) {
      characterOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.line != TOKENS[idx].line) {
      lineOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if ((TOKENS[idx].string == NULL) != (token.string == NULL)) {
      additionalDataOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (TOKENS[idx].string != NULL && token.string != NULL &&
        strcmp(TOKENS[idx].string, token.string) != 0) {
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

  typedef struct {
    bool error;
    TokenType type;
    size_t const line;
    size_t const character;
    char const *string;
  } TokenRecord;
  TokenRecord const TOKENS[] = {
      {true, TT_BAD_HEX, 1, 1, NULL},
      {false, TT_SEMI, 1, 3, NULL},
      {true, TT_BAD_BIN, 2, 1, NULL},
      {false, TT_SEMI, 2, 3, NULL},
      {true, TT_LIT_WSTRING, 3, 1, "\\u00000000"},
      {false, TT_SEMI, 3, 13, NULL},
      {true, TT_BAD_STRING, 4, 1, NULL},
      {false, TT_SEMI, 4, 7, NULL},
      {true, TT_BAD_STRING, 5, 1, NULL},
      {false, TT_SEMI, 5, 7, NULL},
      {true, TT_BAD_STRING, 6, 1, NULL},
      {false, TT_SEMI, 6, 5, NULL},
      {true, TT_LIT_STRING, 7, 1, ""},
      {false, TT_SEMI, 8, 1, NULL},
      {true, TT_BAD_CHAR, 9, 1, NULL},
      {false, TT_SEMI, 9, 3, NULL},
      {true, TT_BAD_CHAR, 10, 1, NULL},
      {false, TT_SEMI, 10, 7, NULL},
      {true, TT_BAD_CHAR, 11, 1, NULL},
      {false, TT_SEMI, 11, 7, NULL},
      {true, TT_BAD_CHAR, 12, 1, NULL},
      {false, TT_SEMI, 12, 5, NULL},
      {true, TT_BAD_CHAR, 13, 1, NULL},
      {false, TT_SEMI, 14, 1, NULL},
      {true, TT_LIT_CHAR, 15, 1, "a"},
      {false, TT_SEMI, 16, 1, NULL},
      {true, TT_BAD_CHAR, 17, 1, NULL},
      {false, TT_SEMI, 17, 5, NULL},
      {true, TT_LIT_CHAR, 18, 1, "a"},
      {false, TT_SEMI, 18, 6, NULL},
      {true, TT_LIT_WCHAR, 19, 1, "\\u00000000"},
      {false, TT_SEMI, 19, 13, NULL},
      {true, TT_SEMI, 20, 2, NULL},
      {true, TT_EOF, 22, 30, NULL},
  };
  size_t const numTokens = sizeof(TOKENS) / sizeof(TokenRecord);

  bool errorFlagOK = true;
  bool typeOK = true;
  bool characterOK = true;
  bool lineOK = true;
  bool additionalDataOK = true;
  char const *messageString = "everything";
  for (size_t idx = 0; idx < numTokens; ++idx) {
    lex(&entry, &token);

    if (entry.errored != TOKENS[idx].error) {
      errorFlagOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.type != TOKENS[idx].type) {
      typeOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.character != TOKENS[idx].character) {
      characterOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (token.line != TOKENS[idx].line) {
      lineOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if ((TOKENS[idx].string == NULL) != (token.string == NULL)) {
      additionalDataOK = false;
      messageString = TOKEN_NAMES[token.type];
      break;
    }
    if (TOKENS[idx].string != NULL && token.string != NULL &&
        strcmp(TOKENS[idx].string, token.string) != 0) {
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
  assert("can't bless lexer tests" && !status.bless);

  lexerInitMaps();

  testAllTokens();
  testErrors();

  lexerUninitMaps();
}