// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of lexer for T

#include "lexer/lexer.h"

#include "util/format.h"
#include "util/stringBuilder.h"

#include <stdlib.h>

char const *const KEYWORDS[] = {
    "module", "using",    "struct", "union", "enum",   "typedef", "if",
    "else",   "while",    "do",     "for",   "switch", "case",    "default",
    "break",  "continue", "return", "asm",   "true",   "false",   "cast",
    "sizeof", "void",     "ubyte",  "byte",  "char",   "uint",    "int",
    "wchar",  "ulong",    "long",   "float", "double", "bool",    "const",
};
size_t const NUM_KEYWORDS = 35;

HashSet *keywordSetCreate(void) {
  HashSet *hs = hashSetCreate();
  for (size_t idx = 0; idx < NUM_KEYWORDS; idx++) {
    // must turn cast-qual off; this is a generic
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    hashSetAdd(hs, (char *)KEYWORDS[idx], false);
#pragma GCC diagnostic pop
  }
  return hs;
}
void keywordSetDestroy(HashSet *hs) { hashSetDestroy(hs, false); }

LexerInfo *lexerInfoCreate(char const *fileName, HashSet const *keywords) {
  File *f = fOpen(fileName);
  if (f == NULL) return NULL;

  LexerInfo *li = malloc(sizeof(LexerInfo));
  li->line = 0;
  li->character = 0;
  li->file = f;
  li->keywords = keywords;

  return li;
}
void lexerInfoDestroy(LexerInfo *li) {
  fClose(li->file);
  free(li);
}

typedef enum {
  // main states
  LS_START,
  LS_SEEN_CR,
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
  LS_CHARS,
  LS_STRINGS,
  LS_ZERO,
  LS_DECIMAL_NUM,
  LS_WORD,

  // COMMENT_OR_DIVIDE states
  LS_LINE_COMMENT,
  LS_BLOCK_COMMENT,
  LS_LINE_COMMENT_MAYBE_ENDED,
  LS_BLOCK_COMMENT_MAYBE_ENDED,

  // AND_OP states
  LS_LAND_OP,

  // OR_OP states
  LS_LOR_OP,

  // LT_OP states
  LS_LTEQ_OP,
  LS_LSHIFT_OP,

  // GT_OP states
  LS_ARSHIFT_OP,
  LS_LRSHIFT_OP,

  // CHARS states
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
  LS_MAYBE_WCHAR,

  // STRINGS states
  LS_STRING_ESCAPED,
  LS_STRING_HEX_DIGIT_1,
  LS_STRING_HEX_DIGIT_2,
  LS_STRING_WHEX_DIGIT_1,
  LS_STRING_WHEX_DIGIT_2,
  LS_STRING_WHEX_DIGIT_3,
  LS_STRING_WHEX_DIGIT_4,
  LS_STRING_WHEX_DIGIT_5,
  LS_STRING_WHEX_DIGIT_6,
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
  LS_STRING_WIDE_END,

  // NUMBER_ADD_OR_SUB states
  LS_BINARY_NUM,
  LS_OCTAL_NUM,
  LS_HEX_NUM,
  LS_FLOAT,
  LS_ADD,
  LS_SUB,

  // WORD states
  LS_WORD_COLON,
} LexerState;

static bool isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
static bool isAlphaOrUnderscore(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'A') || c == '_';
}
static bool isDigit(char c) { return '0' <= c && c <= '9'; }
static bool isAlnumOrUnderscore(char c) {
  return isAlphaOrUnderscore(c) || isDigit(c);
}

TokenType lex(Report *report, LexerInfo *info, TokenInfo *tokenInfo) {
  LexerState state = LS_START;
  StringBuilder *buffer = stringBuilderCreate();
  size_t line = info->line;  // start of token line/character; next char read
  size_t character = info->character;
  // note that tokenInfo stores the character one past the end of the last token
  // tokens won't ever end on whitespace except at the end of a file.

  // implemented as a DFA
  while (true) {
    // look at a character
    char c = fGet(info->file);
    if (c == F_ERR) {
      tokenInfo->line = line;
      tokenInfo->character = character;
      return TT_ERR;
      break;
    }

    switch (state) {
      case LS_START: {
        switch (c) {
          case -2: {  // F_EOF
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_EOF;
          }
          case ' ':
          case '\t': {
            info->character++;
            character++;
            break;
          }
          case '\n': {
            info->line++;
            info->character = 1;
            line++;
            character = 1;
            break;
          }
          case '\r': {
            state = LS_SEEN_CR;
            info->line++;
            info->character = 1;
            line++;
            character = 1;
            break;
          }
          case '/': {
            state = LS_COMMENT_OR_DIVIDE;
            info->character++;  // might be a divide token
            break;
          }
          case '\'': {
            state = LS_CHARS;
            info->character++;
            break;
          }
          case '"': {
            state = LS_STRINGS;
            info->character++;
            break;
          }
          case '0': {
            state = LS_ZERO;
            info->character++;
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
            info->character++;
            break;
          }
          case '-': {
            state = LS_SUB;
            info->character++;
            break;
          }
          case '+': {
            state = LS_ADD;
            info->character++;
            break;
          }
          case '(': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LPAREN;
          }
          case ')': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_RPAREN;
          }
          case '.': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_COMMA;
          }
          case ',': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_DOT;
          }
          case ';': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_SEMI;
          }
          case '?': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_QUESTION;
          }
          case '[': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LSQUARE;
          }
          case ']': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_RSQUARE;
          }
          case '{': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LBRACE;
          }
          case '}': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_RBRACE;
          }
          case '~': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_TILDE;
          }
          case ':': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_COLON;
          }
          case '!': {
            state = LS_LNOT_OP;
            info->character++;
            break;
          }
          case '%': {
            state = LS_MOD_OP;
            info->character++;
            break;
          }
          case '*': {
            state = LS_MUL_OP;
            info->character++;
            break;
          }
          case '=': {
            state = LS_ASSIGN_OP;
            info->character++;
            break;
          }
          case '^': {
            state = LS_XOR_OP;
            info->character++;
            break;
          }
          case '&': {
            state = LS_LAND_OP;
            info->character++;
            break;
          }
          case '|': {
            state = LS_LOR_OP;
            info->character++;
            break;
          }
          default: {
            if (isAlphaOrUnderscore(c)) {
              state = LS_WORD;
              info->character++;
              break;
            } else {
              info->character++;
              tokenInfo->line = line;
              tokenInfo->character = character;
              tokenInfo->data.invalidChar = c;
              return TT_INVALID;
            }
          }
        }
        break;
      }
      case LS_SEEN_CR: {
        switch (c) {
          case '\n': {
            state = LS_START;
            break;
          }
          default: {
            state = LS_START;
            fUnget(info->file);
          }
        }
        break;
      }
      case LS_COMMENT_OR_DIVIDE: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_DIVASSIGN;
          }
          case '*': {
            state = LS_BLOCK_COMMENT;
            info->character++;
            break;
          }
          case '/': {
            state = LS_LINE_COMMENT;
            info->character++;
            break;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_SLASH;
          }
        }
        break;
      }
      case LS_LNOT_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_NEQ;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_BANG;
          }
        }
        break;
      }
      case LS_MOD_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_MODASSIGN;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_PERCENT;
          }
        }
        break;
      }
      case LS_MUL_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_STAR;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_MULASSIGN;
          }
        }
        break;
      }
      case LS_ASSIGN_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_ASSIGN;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_EQ;
          }
        }
        break;
      }
      case LS_XOR_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_CARET;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_BITXORASSIGN;
          }
        }
        break;
      }
      case LS_AND_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_BITANDASSIGN;
          }
          case '&': {
            state = LS_LAND_OP;
            info->character++;
            break;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_AMPERSAND;
          }
        }
        break;
      }
      case LS_OR_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_BITORASSIGN;
          }
          case '|': {
            state = LS_LOR_OP;
            info->character++;
            break;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_PIPE;
          }
        }
        break;
      }
      case LS_LT_OP: {
        switch (c) {
          case '<': {
            state = LS_LSHIFT_OP;
            info->character++;
            break;
          }
          case '=': {
            state = LS_LTEQ_OP;
            info->character++;
            break;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LANGLE;
          }
        }
        break;
      }
      case LS_GT_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_GTEQ;
          }
          case '>': {
            state = LS_ARSHIFT_OP;
            info->character++;
            break;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_RANGLE;
          }
        }
        break;
      }
      case LS_CHARS: {
        switch (c) {}
        break;
      }
      case LS_STRINGS: {
        switch (c) {}
        break;
      }
      case LS_ZERO: {
        switch (c) {}
        break;
      }
      case LS_DECIMAL_NUM: {
        switch (c) {}
        break;
      }
      case LS_WORD: {
        switch (c) {}
        break;
      }
      case LS_LINE_COMMENT: {
        switch (c) {}
        break;
      }
      case LS_BLOCK_COMMENT: {
        switch (c) {}
        break;
      }
      case LS_LINE_COMMENT_MAYBE_ENDED: {
        switch (c) {}
        break;
      }
      case LS_BLOCK_COMMENT_MAYBE_ENDED: {
        switch (c) {}
        break;
      }
      case LS_LAND_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LANDASSIGN;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LAND;
          }
        }
        break;
      }
      case LS_LOR_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LORASSIGN;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LOR;
          }
        }
        break;
      }
      case LS_LTEQ_OP: {
        switch (c) {
          case '>': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_SPACESHIP;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LTEQ;
          }
        }
        break;
      }
      case LS_LSHIFT_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LSHIFTASSIGN;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LSHIFT;
          }
        }
        break;
      }
      case LS_ARSHIFT_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_ARSHIFTASSIGN;
          }
          case '>': {
            state = LS_LRSHIFT_OP;
            info->character++;
            break;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_ARSHIFT;
          }
        }
        break;
      }
      case LS_LRSHIFT_OP: {
        switch (c) {
          case '=': {
            info->character++;
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LRSHIFTASSIGN;
          }
          default: {
            fUnget(info->file);
            tokenInfo->line = line;
            tokenInfo->character = character;
            return TT_LRSHIFT;
          }
        }
        break;
      }
      case LS_CHAR_SINGLE: {
        switch (c) {}
        break;
      }
      case LS_CHAR_ESCAPED: {
        switch (c) {}
        break;
      }
      case LS_CHAR_HEX_DIGIT_1: {
        switch (c) {}
        break;
      }
      case LS_CHAR_HEX_DIGIT_2: {
        switch (c) {}
        break;
      }
      case LS_CHAR_WHEX_DIGIT_1: {
        switch (c) {}
        break;
      }
      case LS_CHAR_WHEX_DIGIT_2: {
        switch (c) {}
        break;
      }
      case LS_CHAR_WHEX_DIGIT_3: {
        switch (c) {}
        break;
      }
      case LS_CHAR_WHEX_DIGIT_4: {
        switch (c) {}
        break;
      }
      case LS_CHAR_WHEX_DIGIT_5: {
        switch (c) {}
        break;
      }
      case LS_CHAR_WHEX_DIGIT_6: {
        switch (c) {}
        break;
      }
      case LS_MAYBE_WCHAR: {
        switch (c) {}
        break;
      }
      case LS_STRING_ESCAPED: {
        switch (c) {}
        break;
      }
      case LS_STRING_HEX_DIGIT_1: {
        switch (c) {}
        break;
      }
      case LS_STRING_HEX_DIGIT_2: {
        switch (c) {}
        break;
      }
      case LS_STRING_WHEX_DIGIT_1: {
        switch (c) {}
        break;
      }
      case LS_STRING_WHEX_DIGIT_2: {
        switch (c) {}
        break;
      }
      case LS_STRING_WHEX_DIGIT_3: {
        switch (c) {}
        break;
      }
      case LS_STRING_WHEX_DIGIT_4: {
        switch (c) {}
        break;
      }
      case LS_STRING_WHEX_DIGIT_5: {
        switch (c) {}
        break;
      }
      case LS_STRING_WHEX_DIGIT_6: {
        switch (c) {}
        break;
      }
      case LS_MAYBE_WSTRING: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_ESCAPED: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_HEX_DIGIT_1: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_HEX_DIGIT_2: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_WHEX_DIGIT_1: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_WHEX_DIGIT_2: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_WHEX_DIGIT_3: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_WHEX_DIGIT_4: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_WHEX_DIGIT_5: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_WHEX_DIGIT_6: {
        switch (c) {}
        break;
      }
      case LS_STRING_WIDE_END: {
        switch (c) {}
        break;
      }
      case LS_BINARY_NUM: {
        switch (c) {}
        break;
      }
      case LS_OCTAL_NUM: {
        switch (c) {}
        break;
      }
      case LS_HEX_NUM: {
        switch (c) {}
        break;
      }
      case LS_FLOAT: {
        switch (c) {}
        break;
      }
      case LS_ADD: {
        switch (c) {}
        break;
      }
      case LS_SUB: {
        switch (c) {}
        break;
      }
      case LS_WORD_COLON: {
        switch (c) {}
        break;
      }
    }
  }
}