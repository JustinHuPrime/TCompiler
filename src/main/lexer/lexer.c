// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// Implementation of lexer for T

#include "lexer/lexer.h"

#include "util/container/stringBuilder.h"
#include "util/format.h"
#include "util/functional.h"

#include <stdlib.h>
#include <string.h>

char const *tokenToName(TokenType tt) {
  switch (tt) {
    case TT_ERR:
      return "a read error";
    case TT_INVALID:
      return "an unexpected character";
    case TT_EMPTY_SQUOTE:
      return "empty single quotes";
    case TT_INVALID_ESCAPE:
      return "invalid escape sequence";
  }
}
void tokenInfoCleanup(TokenType tt, TokenInfo *info) {
  if (tt == TT_LITERALCHAR || tt == TT_LITERALWCHAR || tt == TT_LITERALSTRING ||
      tt == TT_LITERALWSTRING || tt == TT_LITERALINT_0 ||
      tt == TT_LITERALINT_B || tt == TT_LITERALINT_O || tt == TT_LITERALINT_D ||
      tt == TT_LITERALINT_H || tt == TT_LITERALFLOAT || tt == TT_ID ||
      tt == TT_SCOPED_ID) {
    free(info->data.string);
  }
}

static char const *const KEYWORDS[] = {
    "module", "using",    "struct", "union", "enum",   "typedef", "if",
    "else",   "while",    "do",     "for",   "switch", "case",    "default",
    "break",  "continue", "return", "asm",   "true",   "false",   "cast",
    "sizeof", "void",     "ubyte",  "byte",  "char",   "uint",    "int",
    "wchar",  "ulong",    "long",   "float", "double", "bool",    "const",
};
static TokenType const KEYWORD_TOKENS[] = {
    TT_MODULE, TT_USING,   TT_STRUCT, TT_UNION,    TT_ENUM,   TT_TYPEDEF,
    TT_IF,     TT_ELSE,    TT_WHILE,  TT_DO,       TT_FOR,    TT_SWITCH,
    TT_CASE,   TT_DEFAULT, TT_BREAK,  TT_CONTINUE, TT_RETURN, TT_ASM,
    TT_TRUE,   TT_FALSE,   TT_CAST,   TT_SIZEOF,   TT_VOID,   TT_UBYTE,
    TT_BYTE,   TT_CHAR,    TT_UINT,   TT_INT,      TT_WCHAR,  TT_ULONG,
    TT_LONG,   TT_FLOAT,   TT_DOUBLE, TT_BOOL,     TT_CONST,
};
static size_t const NUM_KEYWORDS = 35;

KeywordMap *keywordMapCreate(void) {
  KeywordMap *map = hashMapCreate();
  for (size_t idx = 0; idx < NUM_KEYWORDS; idx++) {
    // must turn cast-qual off; this is a generic
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    hashMapPut(map, (char *)KEYWORDS[idx], (TokenType *)&KEYWORD_TOKENS[idx],
               nullDtor);
#pragma GCC diagnostic pop
  }
  return map;
}
TokenType const *keywordMapGet(KeywordMap const *map, char const *key) {
  return hashMapGet(map, key);
}
void keywordMapDestroy(HashMap *map) { hashMapDestroy(map, nullDtor); }

LexerInfo *lexerInfoCreate(char const *fileName, HashMap const *keywords) {
  File *f = fOpen(fileName);
  if (f == NULL) return NULL;

  LexerInfo *li = malloc(sizeof(LexerInfo));
  li->line = 1;
  li->character = 0;
  li->file = f;
  li->keywords = keywords;
  li->fileName = fileName;

  return li;
}
void lexerInfoDestroy(LexerInfo *li) {
  fClose(li->file);
  free(li);
}

// lex
typedef enum {
  // start states
  LS_START,
  LS_SEEN_CR,

  // punctuation/operator states
  LS_COMMENT_OR_DIVIDE,
  LS_LNOT_OP,
  LS_MOD_OP,
  LS_MUL_OP,
  LS_ASSIGN_OP,
  LS_XOR_OP,
  LS_AND_OP,
  LS_OR_OP,
  LS_LT_OP,
  LS_GT_OP,
  LS_LAND_OP,
  LS_LOR_OP,
  LS_LTEQ_OP,
  LS_LSHIFT_OP,
  LS_ARSHIFT_OP,
  LS_LRSHIFT_OP,
  LS_ADD,
  LS_SUB,

  // comment states
  LS_LINE_COMMENT,
  LS_BLOCK_COMMENT,
  LS_LINE_COMMENT_MAYBE_ENDED,
  LS_BLOCK_COMMENT_MAYBE_ENDED,
  LS_BLOCK_COMMENT_SEEN_CR,

  // char states
  LS_CHARS,
  LS_CHAR_SINGLE,
  LS_CHAR_ESCAPED,
  LS_CHAR_HEX_DIGIT_1,
  LS_CHAR_HEX_DIGIT_2,
  LS_CHAR_WHEX_DIGIT_1,
  LS_CHAR_WHEX_DIGIT_2,
  LS_CHAR_WHEX_DIGIT_3,
  LS_CHAR_WHEX_DIGIT_4,
  LS_CHAR_WHEX_DIGIT_5,
  LS_CHAR_WHEX_DIGIT_6,
  LS_CHAR_WHEX_DIGIT_7,
  LS_CHAR_WHEX_DIGIT_8,
  LS_MAYBE_WCHAR,
  LS_EXPECT_WCHAR,
  LS_IS_WCHAR,
  LS_CHAR_ERROR_MUNCH,
  LS_CHAR_ERROR_MAYBE_WCHAR,
  LS_CHAR_BAD_ESCAPE,
  LS_CHAR_BAD_ESCAPE_MAYBE_WCHAR,

  // string states
  LS_STRING,
  LS_STRING_ESCAPED,
  LS_STRING_HEX_DIGIT_1,
  LS_STRING_HEX_DIGIT_2,
  LS_STRING_WHEX_DIGIT_1,
  LS_STRING_WHEX_DIGIT_2,
  LS_STRING_WHEX_DIGIT_3,
  LS_STRING_WHEX_DIGIT_4,
  LS_STRING_WHEX_DIGIT_5,
  LS_STRING_WHEX_DIGIT_6,
  LS_STRING_WHEX_DIGIT_7,
  LS_STRING_WHEX_DIGIT_8,
  LS_MAYBE_WSTRING,
  LS_STRING_WIDE,
  LS_STRING_WIDE_ESCAPED,
  LS_STRING_WIDE_HEX_DIGIT_1,
  LS_STRING_WIDE_HEX_DIGIT_2,
  LS_STRING_WIDE_WHEX_DIGIT_1,
  LS_STRING_WIDE_WHEX_DIGIT_2,
  LS_STRING_WIDE_WHEX_DIGIT_3,
  LS_STRING_WIDE_WHEX_DIGIT_4,
  LS_STRING_WIDE_WHEX_DIGIT_5,
  LS_STRING_WIDE_WHEX_DIGIT_6,
  LS_STRING_WIDE_WHEX_DIGIT_7,
  LS_STRING_WIDE_WHEX_DIGIT_8,
  LS_STRING_WIDE_END,
  LS_STRING_BAD_ESCAPE,
  LS_STRING_BAD_ESCAPE_MAYBE_WCHAR,

  // number states
  LS_ZERO,
  LS_DECIMAL_NUM,
  LS_BINARY_NUM,
  LS_OCTAL_NUM,
  LS_HEX_NUM,
  LS_FLOAT,

  // word states
  LS_WORD,
  LS_WORD_COLON,
  LS_SCOPED_WORD,
  LS_SCOPED_WORD_COLON,

  // special states
  LS_EOF,
} LexerState;

static bool isAlphaOrUnderscore(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'A') || c == '_';
}
static bool isAlnumOrUnderscore(char c) {
  return isAlphaOrUnderscore(c) || ('0' <= c && c <= '9');
}
// common action used by all hex digits in char or string literals
static LexerState hexDigitAction(Report *report, char c, StringBuilder *buffer,
                                 TokenInfo *tokenInfo, LexerInfo *lexerInfo,
                                 LexerState state, LexerState nextState,
                                 LexerState nextError, size_t offset,
                                 bool isCharLiteral) {
  switch (c) {
    case -2: {  // F_EOF
      stringBuilderDestroy(buffer);

      reportError(
          report,
          format(isCharLiteral
                     ? "%s:%zu:%zu: error: unterminated character literal"
                     : "%s:%zu:%zu: error: unterminated string literal",
                 lexerInfo->fileName, tokenInfo->line, tokenInfo->character));

      tokenInfo->line = lexerInfo->line;
      tokenInfo->character = lexerInfo->character;

      return LS_EOF;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'A':
    case 'b':
    case 'B':
    case 'c':
    case 'C':
    case 'd':
    case 'D':
    case 'e':
    case 'E':
    case 'f':
    case 'F': {
      stringBuilderPush(buffer, c);
      return LS_CHAR_HEX_DIGIT_2;
    }
    default: {
      reportError(report, format("%s:%zu:%zu: error: invalid escape sequence",
                                 lexerInfo->fileName, lexerInfo->line,
                                 lexerInfo->character - offset));

      stringBuilderDestroy(buffer);
      buffer = NULL;
      return LS_CHAR_BAD_ESCAPE;
    }
  }
}

TokenType lex(Report *report, LexerInfo *lexerInfo, TokenInfo *tokenInfo) {
  LexerState state = LS_START;

  StringBuilder *buffer = NULL;  // set when a stringbuilder is actually needed

  // lexerInfo holds the position of the character just read

  // implemented as a DFA
  while (true) {
    // look at a character
    char c = fGet(lexerInfo->file);
    if (c == F_ERR) {
      // cleanup, and produce error
      tokenInfo->line = lexerInfo->line;
      tokenInfo->character = lexerInfo->character;

      reportError(
          report,
          format("%s:%zu:%zu: error: could not read next character; "
                 "filesystem error?",
                 lexerInfo->fileName, lexerInfo->line, lexerInfo->character));

      if (buffer != NULL) stringBuilderDestroy(buffer);

      return TT_ERR;
    }

    // default action - newlines and pushback handled specially
    lexerInfo->character++;

    switch (state) {
      // all states must handle all character possibilities
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wswitch-default"
      // start
      case LS_START: {
        switch (c) {
          case -2: {  // F_EOF
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_EOF;
          }
          case ' ':
          case '\t': {
            break;
          }
          case '\n': {
            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          case '\r': {
            state = LS_SEEN_CR;

            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          case '/': {
            state = LS_COMMENT_OR_DIVIDE;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '\'': {
            state = LS_CHARS;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            buffer = stringBuilderCreate();
            break;
          }
          case '"': {
            state = LS_STRING;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            buffer = stringBuilderCreate();
            break;
          }
          case '0': {
            state = LS_ZERO;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            buffer = stringBuilderCreate();
            break;
          }
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': {
            state = LS_DECIMAL_NUM;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            buffer = stringBuilderCreate();
            break;
          }
          case '-': {
            state = LS_SUB;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            buffer = stringBuilderCreate();
            break;
          }
          case '+': {
            state = LS_ADD;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            buffer = stringBuilderCreate();
            break;
          }
          case '(': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_LPAREN;
          }
          case ')': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_RPAREN;
          }
          case '.': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_DOT;
          }
          case ',': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_COMMA;
          }
          case ';': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_SEMI;
          }
          case '?': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_QUESTION;
          }
          case '[': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_LSQUARE;
          }
          case ']': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_RSQUARE;
          }
          case '{': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_LBRACE;
          }
          case '}': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_RBRACE;
          }
          case '~': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_TILDE;
          }
          case ':': {
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_COLON;
          }
          case '!': {
            state = LS_LNOT_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '%': {
            state = LS_MOD_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '*': {
            state = LS_MUL_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '=': {
            state = LS_ASSIGN_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '^': {
            state = LS_XOR_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '&': {
            state = LS_AND_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '|': {
            state = LS_OR_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '<': {
            state = LS_LT_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          case '>': {
            state = LS_GT_OP;

            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            break;
          }
          default: {
            if (isAlphaOrUnderscore(c)) {
              state = LS_WORD;

              tokenInfo->line = lexerInfo->line;
              tokenInfo->character = lexerInfo->character;
              buffer = stringBuilderCreate();
              stringBuilderPush(buffer, c);
              break;
            } else {
              tokenInfo->line = lexerInfo->line;
              tokenInfo->character = lexerInfo->character;
              tokenInfo->data.invalidChar = c;

              reportError(report, format("%s:%zu:%zu: error: unexpected '%c'",
                                         lexerInfo->fileName, lexerInfo->line,
                                         lexerInfo->character, c));
              return TT_INVALID;
            }
          }
        }
        break;
      }  // LS_START
      case LS_SEEN_CR: {
        switch (c) {
          case '\n': {
            state = LS_START;

            lexerInfo->character = 0;
            break;
          }
          default: {
            fUnget(lexerInfo->file);

            state = LS_START;

            lexerInfo->character = 0;
            break;
          }
        }
        break;
      }  // LS_SEEN_CR

      // punctuation/operators
      case LS_COMMENT_OR_DIVIDE: {
        switch (c) {
          case '=': {
            return TT_DIVASSIGN;
          }
          case '*': {
            state = LS_BLOCK_COMMENT;
            break;
          }
          case '/': {
            state = LS_LINE_COMMENT;
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            return TT_SLASH;
          }
        }
        break;
      }  // LS_COMMENT_OR_DIVIDE
      case LS_LNOT_OP: {
        switch (c) {
          case '=': {
            return TT_NEQ;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_BANG;
          }
        }
        break;
      }  // LS_LNOT_OP
      case LS_MOD_OP: {
        switch (c) {
          case '=': {
            return TT_MODASSIGN;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_PERCENT;
          }
        }
        break;
      }  // LS_MOD_OP
      case LS_MUL_OP: {
        switch (c) {
          case '=': {
            return TT_MULASSIGN;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_STAR;
          }
        }
        break;
      }  // LS_MUL_OP
      case LS_ASSIGN_OP: {
        switch (c) {
          case '=': {
            return TT_EQ;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_ASSIGN;
          }
        }
        break;
      }  // LS_ASSIGN_OP
      case LS_XOR_OP: {
        switch (c) {
          case '=': {
            return TT_BITXORASSIGN;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_CARET;
          }
        }
        break;
      }  // LS_XOR_OP
      case LS_AND_OP: {
        switch (c) {
          case '=': {
            return TT_BITANDASSIGN;
          }
          case '&': {
            state = LS_LAND_OP;
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_AMPERSAND;
          }
        }
        break;
      }  // LS_AND_OP
      case LS_OR_OP: {
        switch (c) {
          case '=': {
            return TT_BITORASSIGN;
          }
          case '|': {
            state = LS_LOR_OP;
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_PIPE;
          }
        }
        break;
      }  // LS_OR_OP
      case LS_LT_OP: {
        switch (c) {
          case '<': {
            state = LS_LSHIFT_OP;
            break;
          }
          case '=': {
            state = LS_LTEQ_OP;
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LANGLE;
          }
        }
        break;
      }  // LS_LT_OP
      case LS_GT_OP: {
        switch (c) {
          case '=': {
            return TT_GTEQ;
          }
          case '>': {
            state = LS_ARSHIFT_OP;
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_RANGLE;
          }
        }
        break;
      }  // LS_GT_OP
      case LS_LAND_OP: {
        switch (c) {
          case '=': {
            return TT_LANDASSIGN;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LAND;
          }
        }
        break;
      }  // LS_LAND_OP
      case LS_LOR_OP: {
        switch (c) {
          case '=': {
            return TT_LORASSIGN;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LOR;
          }
        }
        break;
      }  // LS_LOR_OP
      case LS_LTEQ_OP: {
        switch (c) {
          case '>': {
            return TT_SPACESHIP;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LTEQ;
          }
        }
        break;
      }  // LS_LTEQ_OP
      case LS_LSHIFT_OP: {
        switch (c) {
          case '=': {
            return TT_LSHIFTASSIGN;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LSHIFT;
          }
        }
        break;
      }  // LS_LSHIFT_OP
      case LS_ARSHIFT_OP: {
        switch (c) {
          case '=': {
            return TT_ARSHIFTASSIGN;
          }
          case '>': {
            state = LS_LRSHIFT_OP;
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_ARSHIFT;
          }
        }
        break;
      }  // LS_ARSHIFT_OP
      case LS_LRSHIFT_OP: {
        switch (c) {
          case '=': {
            return TT_LRSHIFTASSIGN;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LRSHIFT;
          }
        }
        break;
      }  // LS_LRSHIFT_OP
      case LS_ADD: {
        switch (c) {
          case '+': {
            stringBuilderDestroy(buffer);
            return TT_PLUSPLUS;
          }
          case '=': {
            stringBuilderDestroy(buffer);
            return TT_ADDASSIGN;
          }
          case '0': {
            state = LS_ZERO;

            stringBuilderPush(buffer, c);
            break;
          }
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': {
            state = LS_DECIMAL_NUM;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            stringBuilderDestroy(buffer);
            return TT_PLUS;
          }
        }
        break;
      }  // LS_ADD
      case LS_SUB: {
        switch (c) {
          case '-': {
            stringBuilderDestroy(buffer);
            return TT_MINUSMINUS;
          }
          case '=': {
            stringBuilderDestroy(buffer);
            return TT_SUBASSIGN;
          }
          case '>': {
            stringBuilderDestroy(buffer);
            return TT_ARROW;
          }
          case '0': {
            state = LS_ZERO;

            stringBuilderPush(buffer, c);
            break;
          }
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': {
            state = LS_DECIMAL_NUM;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            stringBuilderDestroy(buffer);
            return TT_MINUS;
          }
        }
        break;
      }  // LS_SUB

      // comment states
      case LS_LINE_COMMENT: {
        switch (c) {
          case -2: {  // F_EOF
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_EOF;
          }
          case '\n': {
            state = LS_START;

            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          case '\r': {
            state = LS_LINE_COMMENT_MAYBE_ENDED;

            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          default: { break; }
        }
        break;
      }  // LS_LINE_COMMENT
      case LS_BLOCK_COMMENT: {
        switch (c) {
          case -2: {  // F_EOF
            reportError(report, format("%s:%zu:%zu: error: unterminated block "
                                       "comment at end of file",
                                       lexerInfo->fileName, tokenInfo->line,
                                       tokenInfo->character));
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_EOF;
          }
          case '\n': {
            state = LS_BLOCK_COMMENT;

            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          case '\r': {
            state = LS_BLOCK_COMMENT_SEEN_CR;

            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          case '*': {
            state = LS_BLOCK_COMMENT_MAYBE_ENDED;
            break;
          }
          default: { break; }
        }
        break;
      }  // LS_BLOCK_COMMENT
      case LS_LINE_COMMENT_MAYBE_ENDED: {
        switch (c) {
          case '\n': {
            state = LS_START;

            lexerInfo->character = 0;
            break;
          }
          default: {
            fUnget(lexerInfo->file);

            state = LS_START;

            lexerInfo->character = 0;
            break;
          }
        }
        break;
      }  // LS_LINE_COMMENT_MAYBE_ENDED
      case LS_BLOCK_COMMENT_MAYBE_ENDED: {
        switch (c) {
          case -2: {  // F_EOF
            reportError(report, format("%s:%zu:%zu: error: unterminated block "
                                       "comment at end of file",
                                       lexerInfo->fileName, tokenInfo->line,
                                       tokenInfo->character));
            tokenInfo->line = lexerInfo->line;
            tokenInfo->character = lexerInfo->character;
            return TT_EOF;
          }
          case '/': {
            state = LS_START;
            break;
          }
          case '\n': {
            state = LS_BLOCK_COMMENT;

            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          case '\r': {
            state = LS_BLOCK_COMMENT_SEEN_CR;

            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          default: {
            state = LS_BLOCK_COMMENT;
            break;
          }
        }
        break;
      }  // LS_BLOCK_COMMENT_MAYBE_ENDED
      case LS_BLOCK_COMMENT_SEEN_CR: {
        switch (c) {
          case '\n': {
            state = LS_BLOCK_COMMENT;
            break;
          }
          case '\r': {
            lexerInfo->line++;
            lexerInfo->character = 0;
            break;
          }
          case '*': {
            state = LS_BLOCK_COMMENT_MAYBE_ENDED;
            break;
          }
          default: {
            state = LS_BLOCK_COMMENT;
            break;
          }
        }
        break;
      }  // LS_BLOCK_COMMENT_SEEN_CR

      // char states
      case LS_CHARS: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(
                report,
                format("%s:%zu:%zu: error: unterminated character literal",
                       lexerInfo->fileName, tokenInfo->line,
                       tokenInfo->character));

            return TT_EOF;
          }
          case '\'': {
            stringBuilderDestroy(buffer);

            reportError(report,
                        format("%s:%zu:%zu: error: empty character literal",
                               lexerInfo->fileName, lexerInfo->line,
                               lexerInfo->character - 1));
            return TT_EMPTY_SQUOTE;
          }
          case '\\': {
            state = LS_CHAR_ESCAPED;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            state = LS_CHAR_SINGLE;

            stringBuilderPush(buffer, c);
            break;
          }
        }
        break;
      }  // LS_CHARS
      case LS_CHAR_SINGLE: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(
                report,
                format("%s:%zu:%zu: error: unterminated character literal",
                       lexerInfo->fileName, tokenInfo->line,
                       tokenInfo->character));

            return TT_EOF;
          }
          case '\'': {
            state = LS_MAYBE_WCHAR;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
          default: {
            state = LS_CHAR_ERROR_MUNCH;

            reportError(
                report,
                format(
                    "%s:%zu:%zu: error: multiple characters in single quotes",
                    lexerInfo->fileName, lexerInfo->line,
                    lexerInfo->character - buffer->size));

            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
        }
        break;
      }  // LS_CHAR_SINGLE
      case LS_CHAR_ESCAPED: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(
                report,
                format("%s:%zu:%zu: error: unterminated character literal",
                       lexerInfo->fileName, tokenInfo->line,
                       tokenInfo->character));

            return TT_EOF;
          }
          case 'u': {
            state = LS_CHAR_WHEX_DIGIT_1;

            stringBuilderPush(buffer, c);
            break;
          }
          case 'x': {
            state = LS_CHAR_HEX_DIGIT_1;

            stringBuilderPush(buffer, c);
            break;
          }
          case 'n':
          case 'r':
          case 't':
          case '0':
          case '\\':
          case '\'': {
            state = LS_CHAR_SINGLE;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            state = LS_CHAR_BAD_ESCAPE;

            reportError(report,
                        format("%s:%zu:%zu: error: invalid escape sequence",
                               lexerInfo->fileName, lexerInfo->line,
                               lexerInfo->character - 1));

            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
        }
        break;
      }  // LS_CHAR_ESCAPED
      case LS_CHAR_HEX_DIGIT_1: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_HEX_DIGIT_2, LS_CHAR_BAD_ESCAPE, true, 2);
        break;
      }  // LS_CHAR_HEX_DIGIT_1
      case LS_CHAR_HEX_DIGIT_2: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_CHAR_SINGLE, LS_CHAR_BAD_ESCAPE, true, 3);
        break;
      }  // LS_CHAR_HEX_DIGIT_2
      case LS_CHAR_WHEX_DIGIT_1: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_WHEX_DIGIT_2, LS_CHAR_BAD_ESCAPE, true, 2);
        break;
      }  // LS_CHAR_WHEX_DIGIT_1
      case LS_CHAR_WHEX_DIGIT_2: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_WHEX_DIGIT_3, LS_CHAR_BAD_ESCAPE, true, 3);
        break;
      }  // LS_CHAR_WHEX_DIGIT_2
      case LS_CHAR_WHEX_DIGIT_3: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_WHEX_DIGIT_4, LS_CHAR_BAD_ESCAPE, true, 4);
        break;
      }  // LS_CHAR_WHEX_DIGIT_3
      case LS_CHAR_WHEX_DIGIT_4: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_WHEX_DIGIT_5, LS_CHAR_BAD_ESCAPE, true, 5);
        break;
      }  // LS_CHAR_WHEX_DIGIT_4
      case LS_CHAR_WHEX_DIGIT_5: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_WHEX_DIGIT_6, LS_CHAR_BAD_ESCAPE, true, 6);
        break;
      }  // LS_CHAR_WHEX_DIGIT_5
      case LS_CHAR_WHEX_DIGIT_6: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_WHEX_DIGIT_7, LS_CHAR_BAD_ESCAPE, true, 7);
        break;
      }  // LS_CHAR_WHEX_DIGIT_6
      case LS_CHAR_WHEX_DIGIT_7: {
        state =
            hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                           LS_CHAR_WHEX_DIGIT_8, LS_CHAR_BAD_ESCAPE, true, 8);
        break;
      }  // LS_CHAR_WHEX_DIGIT_7
      case LS_CHAR_WHEX_DIGIT_8: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_EXPECT_WCHAR, LS_CHAR_BAD_ESCAPE, true, 9);
        break;
      }  // LS_CHAR_WHEX_DIGIT_8
      case LS_MAYBE_WCHAR: {
        switch (c) {
          case 'w': {
            return TT_LITERALWCHAR;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LITERALCHAR;
          }
        }
        break;
      }  // LS_CHAR_MAYBE_WCHAR
      case LS_EXPECT_WCHAR: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(
                report,
                format("%s:%zu:%zu: error: unterminated character literal",
                       lexerInfo->fileName, tokenInfo->line,
                       tokenInfo->character));

            return TT_EOF;
          }
          case '\'': {
            state = LS_IS_WCHAR;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
          default: {
            state = LS_CHAR_ERROR_MUNCH;

            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
        }
        break;
      }  // LS_EXPECT_WCHAR
      case LS_IS_WCHAR: {
        switch (c) {
          case 'w': {
            return TT_LITERALWCHAR;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            reportError(report, format("%s:%zu:%zu: error: expected wide "
                                       "character, but no specifier found",
                                       lexerInfo->fileName, lexerInfo->line,
                                       lexerInfo->character -
                                           strlen(tokenInfo->data.string) - 1));

            free(tokenInfo->data.string);
            return TT_NOT_WIDE;
          }
        }
        break;
      }  // LS_IS_WCHAR
      case LS_CHAR_ERROR_MUNCH: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(
                report,
                format("%s:%zu:%zu: error: unterminated character literal",
                       lexerInfo->fileName, tokenInfo->line,
                       tokenInfo->character));

            return TT_EOF;
          }
          case '\'': {
            state = LS_CHAR_ERROR_MAYBE_WCHAR;
            break;
          }
          default: { break; }
        }
        break;
      }  // LS_CHAR_ERROR_MUNCH
      case LS_CHAR_ERROR_MAYBE_WCHAR: {
        switch (c) {
          case 'w': {
            return TT_MULTICHAR_CHAR;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_MULTICHAR_CHAR;
          }
        }
        break;
      }  // LS_CHAR_ERROR_MAYBE_WCHAR
      case LS_CHAR_BAD_ESCAPE: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(
                report,
                format("%s:%zu:%zu: error: unterminated character literal",
                       lexerInfo->fileName, tokenInfo->line,
                       tokenInfo->character));

            return TT_EOF;
          }
          case '\'': {
            state = LS_CHAR_BAD_ESCAPE_MAYBE_WCHAR;
            break;
          }
          default: { break; }
        }
        break;
      }  // LS_CHAR_BAD_ESCAPE
      case LS_CHAR_BAD_ESCAPE_MAYBE_WCHAR: {
        switch (c) {
          case 'w': {
            return TT_INVALID_ESCAPE;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_INVALID_ESCAPE;
          }
        }
        break;
      }  // LS_CHAR_BAD_ESCAPE_MAYBE_WCHAR

      // string states
      case LS_STRING: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(report,
                        format("%s:%zu:%zu: error: unterminated string literal",
                               lexerInfo->fileName, tokenInfo->line,
                               tokenInfo->character));

            return TT_EOF;
          }
          case '\\': {
            state = LS_STRING_ESCAPED;

            stringBuilderPush(buffer, c);
            break;
          }
          case '"': {
            state = LS_MAYBE_WSTRING;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
          default: {
            stringBuilderPush(buffer, c);
            break;
          }
        }
        break;
      }  // LS_STRING
      case LS_STRING_ESCAPED: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(report,
                        format("%s:%zu:%zu: error: unterminated string literal",
                               lexerInfo->fileName, tokenInfo->line,
                               tokenInfo->character));

            return TT_EOF;
          }
          case 'n':
          case 'r':
          case 't':
          case '0':
          case '\\':
          case '"': {
            state = LS_STRING;

            stringBuilderPush(buffer, c);
            break;
          }
          case 'x': {
            state = LS_STRING_HEX_DIGIT_1;

            stringBuilderPush(buffer, c);
            break;
          }
          case 'u': {
            state = LS_STRING_WHEX_DIGIT_1;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            state = LS_STRING_BAD_ESCAPE;

            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
        }
        break;
      }  // LS_STRING_ESCAPED
      case LS_STRING_HEX_DIGIT_1: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_HEX_DIGIT_2, LS_STRING_BAD_ESCAPE,
                               false, 2);
        break;
      }  // LS_STRING_HEX_DIGIT_1
      case LS_STRING_HEX_DIGIT_2: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING, LS_STRING_BAD_ESCAPE, false, 3);
        break;
      }  // LS_STRING_HEX_DIGIT_2
      case LS_STRING_WHEX_DIGIT_1: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WHEX_DIGIT_2, LS_STRING_BAD_ESCAPE,
                               false, 2);
        break;
      }  // LS_STRING_WHEX_DIGIT_1
      case LS_STRING_WHEX_DIGIT_2: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WHEX_DIGIT_3, LS_STRING_BAD_ESCAPE,
                               false, 3);
        break;
      }  // LS_STRING_WHEX_DIGIT_2
      case LS_STRING_WHEX_DIGIT_3: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WHEX_DIGIT_4, LS_STRING_BAD_ESCAPE,
                               false, 4);
        break;
      }  // LS_STRING_WHEX_DIGIT_4
      case LS_STRING_WHEX_DIGIT_4: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WHEX_DIGIT_5, LS_STRING_BAD_ESCAPE,
                               false, 5);
        break;
      }  // LS_STRING_WHEX_DIGIT_4
      case LS_STRING_WHEX_DIGIT_5: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WHEX_DIGIT_6, LS_STRING_BAD_ESCAPE,
                               false, 6);
        break;
      }  // LS_STRING_WHEX_DIGIT_5
      case LS_STRING_WHEX_DIGIT_6: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WHEX_DIGIT_7, LS_STRING_BAD_ESCAPE,
                               false, 7);
        break;
      }  // LS_STRING_WHEX_DIGIT_6
      case LS_STRING_WHEX_DIGIT_7: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WHEX_DIGIT_8, LS_STRING_BAD_ESCAPE,
                               false, 8);
        break;
      }  // LS_STRING_WHEX_DIGIT_7
      case LS_STRING_WHEX_DIGIT_8: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE, LS_STRING_BAD_ESCAPE, false, 9);
        break;
      }  // LS_STRING_WHEX_DIGIT_8
      case LS_MAYBE_WSTRING: {
        switch (c) {
          case 'w': {
            return TT_LITERALWSTRING;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_LITERALSTRING;
          }
        }
        break;
      }  // LS_MAYBE_WSTRING
      case LS_STRING_WIDE: {
        switch (c) {
          case '\\': {
            state = LS_STRING_WIDE_ESCAPED;

            stringBuilderPush(buffer, c);
            break;
          }
          case '"': {
            state = LS_STRING_WIDE_END;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
          default: {
            stringBuilderPush(buffer, c);
            break;
          }
        }
        break;
      }  // LS_STRING_WIDE
      case LS_STRING_WIDE_ESCAPED: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(report,
                        format("%s:%zu:%zu: error: unterminated string literal",
                               lexerInfo->fileName, tokenInfo->line,
                               tokenInfo->character));

            return TT_EOF;
          }
          case 'n':
          case 'r':
          case 't':
          case '0':
          case '\\':
          case '"': {
            state = LS_STRING_WIDE;

            stringBuilderPush(buffer, c);
            break;
          }
          case 'x': {
            state = LS_STRING_WIDE_HEX_DIGIT_1;

            stringBuilderPush(buffer, c);
            break;
          }
          case 'u': {
            state = LS_STRING_WIDE_WHEX_DIGIT_1;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            state = LS_STRING_BAD_ESCAPE;

            reportError(report,
                        format("%s:%zu:%zu: error: invalid escape sequence",
                               lexerInfo->fileName, lexerInfo->line,
                               lexerInfo->character - 1));

            stringBuilderDestroy(buffer);
            buffer = NULL;
            break;
          }
        }
        break;
      }  // LS_STRING_WIDE_ESCAPED
      case LS_STRING_WIDE_HEX_DIGIT_1: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_HEX_DIGIT_2, LS_STRING_BAD_ESCAPE,
                               false, 2);
        break;
      }  // LS_STRING_WIDE_HEX_DIGIT_1
      case LS_STRING_WIDE_HEX_DIGIT_2: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE, LS_STRING_BAD_ESCAPE, false, 3);
        break;
      }  // LS_STRING_WIDE_HEX_DIGIT_2
      case LS_STRING_WIDE_WHEX_DIGIT_1: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_WHEX_DIGIT_2,
                               LS_STRING_BAD_ESCAPE, false, 2);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_1
      case LS_STRING_WIDE_WHEX_DIGIT_2: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_WHEX_DIGIT_3,
                               LS_STRING_BAD_ESCAPE, false, 3);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_2
      case LS_STRING_WIDE_WHEX_DIGIT_3: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_WHEX_DIGIT_4,
                               LS_STRING_BAD_ESCAPE, false, 4);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_3
      case LS_STRING_WIDE_WHEX_DIGIT_4: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_WHEX_DIGIT_5,
                               LS_STRING_BAD_ESCAPE, false, 5);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_4
      case LS_STRING_WIDE_WHEX_DIGIT_5: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_WHEX_DIGIT_6,
                               LS_STRING_BAD_ESCAPE, false, 6);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_5
      case LS_STRING_WIDE_WHEX_DIGIT_6: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_WHEX_DIGIT_7,
                               LS_STRING_BAD_ESCAPE, false, 7);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_6
      case LS_STRING_WIDE_WHEX_DIGIT_7: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE_WHEX_DIGIT_8,
                               LS_STRING_BAD_ESCAPE, false, 8);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_7
      case LS_STRING_WIDE_WHEX_DIGIT_8: {
        state = hexDigitAction(report, c, buffer, tokenInfo, lexerInfo, state,
                               LS_STRING_WIDE, LS_STRING_BAD_ESCAPE, false, 9);
        break;
      }  // LS_STRING_WIDE_WHEX_DIGIT_8
      case LS_STRING_WIDE_END: {
        switch (c) {
          case 'w': {
            return TT_LITERALWSTRING;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            reportError(report, format("%s:%zu:%zu: error: expected wide "
                                       "string, but no specifier found",
                                       lexerInfo->fileName, lexerInfo->line,
                                       lexerInfo->character -
                                           strlen(tokenInfo->data.string) - 1));

            free(tokenInfo->data.string);
            return TT_NOT_WIDE;
          }
        }
        break;
      }  // LS_STRING_WIDE_END
      case LS_STRING_BAD_ESCAPE: {
        switch (c) {
          case -2: {  // F_EOF
            stringBuilderDestroy(buffer);

            reportError(report,
                        format("%s:%zu:%zu: error: unterminated string literal",
                               lexerInfo->fileName, tokenInfo->line,
                               tokenInfo->character));

            return TT_EOF;
          }
          case '"': {
            state = LS_STRING_BAD_ESCAPE_MAYBE_WCHAR;
            break;
          }
          default: { break; }
        }
        break;
      }  // LS_STRING_BAD_ESCAPE
      case LS_STRING_BAD_ESCAPE_MAYBE_WCHAR: {
        switch (c) {
          case 'w': {
            return TT_INVALID_ESCAPE;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;
            return TT_INVALID_ESCAPE;
          }
        }
        break;
      }  // LS_STRING_BAD_ESCAPE_MAYBE_WCHAR

      // number states
      case LS_ZERO: {
        switch (c) {
          case 'b': {
            state = LS_BINARY_NUM;

            stringBuilderPush(buffer, c);
            break;
          }
          case 'x': {
            state = LS_HEX_NUM;

            stringBuilderPush(buffer, c);
            break;
          }
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7': {
            state = LS_OCTAL_NUM;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            return TT_LITERALINT_0;
          }
        }
        break;
      }  // LS_ZERO
      case LS_DECIMAL_NUM: {
        switch (c) {
          case '.': {
            state = LS_FLOAT;

            stringBuilderPush(buffer, c);
            break;
          }
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': {
            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            return TT_LITERALINT_D;
          }
        }
        break;
      }  // LS_DECIMAL_NUM
      case LS_BINARY_NUM: {
        switch (c) {
          case '0':
          case '1': {
            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            return TT_LITERALINT_B;
          }
        }
        break;
      }  // LS_BINARY_NUM
      case LS_OCTAL_NUM: {
        switch (c) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7': {
            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            return TT_LITERALINT_O;
          }
        }
        break;
      }  // LS_BINARY_NUM
      case LS_HEX_NUM: {
        switch (c) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case 'a':
          case 'A':
          case 'b':
          case 'B':
          case 'c':
          case 'C':
          case 'd':
          case 'D':
          case 'e':
          case 'E':
          case 'f':
          case 'F': {
            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            return TT_LITERALINT_H;
          }
        }
        break;
      }  // LS_HEX_NUM
      case LS_FLOAT: {
        switch (c) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': {
            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            lexerInfo->character--;

            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            return TT_LITERALFLOAT;
          }
        }
        break;
      }  // LS_FLOAT

      // word states
      case LS_WORD: {
        if (isAlnumOrUnderscore(c)) {
          stringBuilderPush(buffer, c);
        } else if (c == ':') {
          state = LS_WORD_COLON;

          stringBuilderPush(buffer, c);
        } else {
          fUnget(lexerInfo->file);
          lexerInfo->character--;

          tokenInfo->data.string = stringBuilderData(buffer);
          stringBuilderDestroy(buffer);
          TokenType const *keyword =
              keywordMapGet(lexerInfo->keywords, tokenInfo->data.string);
          if (keyword != NULL) free(tokenInfo->data.string);
          return keyword != NULL ? *keyword : TT_ID;
        }
        break;
      }  // LS_WORD
      case LS_WORD_COLON: {
        switch (c) {
          case ':': {
            state = LS_SCOPED_WORD;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            fUnget(lexerInfo->file);
            lexerInfo->character -= 2;

            stringBuilderPop(buffer);
            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            TokenType const *keyword =
                keywordMapGet(lexerInfo->keywords, tokenInfo->data.string);
            if (keyword != NULL) free(tokenInfo->data.string);
            return keyword != NULL ? *keyword : TT_ID;
          }
        }
        break;
      }  // LS_WORD_COLON
      case LS_SCOPED_WORD: {
        if (isAlnumOrUnderscore(c)) {
          stringBuilderPush(buffer, c);
        } else if (c == ':') {
          state = LS_SCOPED_WORD_COLON;

          stringBuilderPush(buffer, c);
        } else {
          fUnget(lexerInfo->file);
          lexerInfo->character--;

          tokenInfo->data.string = stringBuilderData(buffer);
          stringBuilderDestroy(buffer);
          return TT_SCOPED_ID;
        }
        break;
      }  // LS_SCOPED_WORD
      case LS_SCOPED_WORD_COLON: {
        switch (c) {
          case ':': {
            state = LS_SCOPED_WORD;

            stringBuilderPush(buffer, c);
            break;
          }
          default: {
            fUnget(lexerInfo->file);
            fUnget(lexerInfo->file);
            lexerInfo->character -= 2;

            stringBuilderPop(buffer);
            tokenInfo->data.string = stringBuilderData(buffer);
            stringBuilderDestroy(buffer);
            return TT_SCOPED_ID;
          }
        }
        break;
      }  // LS_SCOPED_WORD_COLON

      // special states
      case LS_EOF: {
        // seen an EOF, but must delay processing for ease of control flow
        fUnget(lexerInfo->file);
        lexerInfo->character--;

        return TT_EOF;
      }
#pragma GCC diagnostic pop
    }
  }
}

static void lexDumpOne(Report *report, KeywordMap *keywords,
                       char const *filename) {
  LexerInfo *info = lexerInfoCreate(filename, keywords);
  TokenType type;
  printf("%s:\n", filename);
  do {
    TokenInfo tokenInfo;
    type = lex(report, info, &tokenInfo);
    switch (type) {
      // errors
      case TT_ERR: {
        printf("%s:%zu:%zu: ERR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_EOF: {
        printf("%s:%zu:%zu: EOF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_INVALID: {
        printf("%s:%zu:%zu: INVALID_CHAR(%c)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.invalidChar);
        break;
      }
      case TT_EMPTY_SQUOTE: {
        printf("%s:%zu:%zu: EMPTY_SQUOTE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_INVALID_ESCAPE: {
        printf("%s:%zu:%zu: INVALID_ESCAPE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_NOT_WIDE: {
        printf("%s:%zu:%zu: NOT_WIDE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MULTICHAR_CHAR: {
        printf("%s:%zu:%zu: MULTICHAR_CHAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      // keywords
      case TT_MODULE: {
        printf("%s:%zu:%zu: MODULE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_USING: {
        printf("%s:%zu:%zu: USING\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_STRUCT: {
        printf("%s:%zu:%zu: STRUCT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_UNION: {
        printf("%s:%zu:%zu: UNION\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ENUM: {
        printf("%s:%zu:%zu: ENUM\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_TYPEDEF: {
        printf("%s:%zu:%zu: TYPEDEF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_IF: {
        printf("%s:%zu:%zu: IF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ELSE: {
        printf("%s:%zu:%zu: ELSE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_WHILE: {
        printf("%s:%zu:%zu: WHILE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DO: {
        printf("%s:%zu:%zu: DO\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_FOR: {
        printf("%s:%zu:%zu: FOR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SWITCH: {
        printf("%s:%zu:%zu: SWITCH\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CASE: {
        printf("%s:%zu:%zu: CASE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DEFAULT: {
        printf("%s:%zu:%zu: DEFAULT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BREAK: {
        printf("%s:%zu:%zu: BREAK\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CONTINUE: {
        printf("%s:%zu:%zu: CONTINUE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RETURN: {
        printf("%s:%zu:%zu: RETURN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ASM: {
        printf("%s:%zu:%zu: ASM\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_TRUE: {
        printf("%s:%zu:%zu: TRUE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_FALSE: {
        printf("%s:%zu:%zu: FALSE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CAST: {
        printf("%s:%zu:%zu: CAST\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SIZEOF: {
        printf("%s:%zu:%zu: SIZEOF\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_VOID: {
        printf("%s:%zu:%zu: VOID\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_UBYTE: {
        printf("%s:%zu:%zu: UBYTE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BYTE: {
        printf("%s:%zu:%zu: BYTE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CHAR: {
        printf("%s:%zu:%zu: CHAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_UINT: {
        printf("%s:%zu:%zu: UINT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_INT: {
        printf("%s:%zu:%zu: INT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_WCHAR: {
        printf("%s:%zu:%zu: WCHAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ULONG: {
        printf("%s:%zu:%zu: ULONG\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LONG: {
        printf("%s:%zu:%zu: LONG\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_FLOAT: {
        printf("%s:%zu:%zu: FLOAT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DOUBLE: {
        printf("%s:%zu:%zu: DOUBLE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BOOL: {
        printf("%s:%zu:%zu: BOOL\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CONST: {
        printf("%s:%zu:%zu: CONST\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      // punctuation
      case TT_SEMI: {
        printf("%s:%zu:%zu: SEMI\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_COMMA: {
        printf("%s:%zu:%zu: COMMA\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LPAREN: {
        printf("%s:%zu:%zu: LPAREN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RPAREN: {
        printf("%s:%zu:%zu: RPAREN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LSQUARE: {
        printf("%s:%zu:%zu: LSQUARE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RSQUARE: {
        printf("%s:%zu:%zu: RSQUARE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LBRACE: {
        printf("%s:%zu:%zu: LBRACE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RBRACE: {
        printf("%s:%zu:%zu: RBRACE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DOT: {
        printf("%s:%zu:%zu: DOT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ARROW: {
        printf("%s:%zu:%zu: ARROW\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PLUSPLUS: {
        printf("%s:%zu:%zu: PLUSPLUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MINUSMINUS: {
        printf("%s:%zu:%zu: MINUSMINUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_STAR: {
        printf("%s:%zu:%zu: STAR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_AMPERSAND: {
        printf("%s:%zu:%zu: AMPERSAND\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PLUS: {
        printf("%s:%zu:%zu: PLUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MINUS: {
        printf("%s:%zu:%zu: MINUS\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BANG: {
        printf("%s:%zu:%zu: BANG\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_TILDE: {
        printf("%s:%zu:%zu: TILDE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SLASH: {
        printf("%s:%zu:%zu: SLASH\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PERCENT: {
        printf("%s:%zu:%zu: PERCENT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LSHIFT: {
        printf("%s:%zu:%zu: LSHIFT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LRSHIFT: {
        printf("%s:%zu:%zu: LRSHIFT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ARSHIFT: {
        printf("%s:%zu:%zu: ARSHIFT\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SPACESHIP: {
        printf("%s:%zu:%zu: SPACESHIP\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LANGLE: {
        printf("%s:%zu:%zu: LANGLE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_RANGLE: {
        printf("%s:%zu:%zu: RANGLE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LTEQ: {
        printf("%s:%zu:%zu: LTEQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_GTEQ: {
        printf("%s:%zu:%zu: GTEQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_EQ: {
        printf("%s:%zu:%zu: EQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_NEQ: {
        printf("%s:%zu:%zu: NEQ\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_PIPE: {
        printf("%s:%zu:%zu: PIPE\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_CARET: {
        printf("%s:%zu:%zu: CARET\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LAND: {
        printf("%s:%zu:%zu: LAND\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LOR: {
        printf("%s:%zu:%zu: LOR\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_QUESTION: {
        printf("%s:%zu:%zu: QUESTION\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_COLON: {
        printf("%s:%zu:%zu: COLON\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ASSIGN: {
        printf("%s:%zu:%zu: ASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MULASSIGN: {
        printf("%s:%zu:%zu: MULASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_DIVASSIGN: {
        printf("%s:%zu:%zu: DIVASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_MODASSIGN: {
        printf("%s:%zu:%zu: MODASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ADDASSIGN: {
        printf("%s:%zu:%zu: ADDASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_SUBASSIGN: {
        printf("%s:%zu:%zu: SUBASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LSHIFTASSIGN: {
        printf("%s:%zu:%zu: LSHIFTASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LRSHIFTASSIGN: {
        printf("%s:%zu:%zu: LRSHIFTASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ARSHIFTASSIGN: {
        printf("%s:%zu:%zu: ARSHIFTASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BITANDASSIGN: {
        printf("%s:%zu:%zu: BITANDASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BITXORASSIGN: {
        printf("%s:%zu:%zu: BITXORASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_BITORASSIGN: {
        printf("%s:%zu:%zu: BITORASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LANDASSIGN: {
        printf("%s:%zu:%zu: LANDASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_LORASSIGN: {
        printf("%s:%zu:%zu: LORASSIGN\n", filename, tokenInfo.line,
               tokenInfo.character);
        break;
      }
      case TT_ID: {
        printf("%s:%zu:%zu: ID(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_SCOPED_ID: {
        printf("%s:%zu:%zu: SCOPED_ID(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_0: {
        printf("%s:%zu:%zu: LITERALINT_0(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_B: {
        printf("%s:%zu:%zu: LITERALINT_B(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_D: {
        printf("%s:%zu:%zu: LITERALINT_D(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_O: {
        printf("%s:%zu:%zu: LITERALINT_O(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALINT_H: {
        printf("%s:%zu:%zu: LITERALINT_H(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALFLOAT: {
        printf("%s:%zu:%zu: LITERALFLOAT(%s)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALSTRING: {
        printf("%s:%zu:%zu: LITERALSTRING(\"%s\")\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALCHAR: {
        printf("%s:%zu:%zu: LITERALCHAR(\'%s\')\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALWSTRING: {
        printf("%s:%zu:%zu: LITERALWSTRING(\"%s\"w)\n", filename,
               tokenInfo.line, tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
      case TT_LITERALWCHAR: {
        printf("%s:%zu:%zu: LITERALWCHAR(\'%s\'w)\n", filename, tokenInfo.line,
               tokenInfo.character, tokenInfo.data.string);
        free(tokenInfo.data.string);
        break;
      }
    }
  } while (type != TT_EOF && type != TT_ERR);
}

void lexDump(Report *report, FileList *files) {
  KeywordMap *keywords = keywordMapCreate();
  for (size_t idx = 0; idx < files->decls.size; idx++)
    lexDumpOne(report, keywords, files->decls.elements[idx]);
  for (size_t idx = 0; idx < files->codes.size; idx++)
    lexDumpOne(report, keywords, files->codes.elements[idx]);
  keywordMapDestroy(keywords);
}