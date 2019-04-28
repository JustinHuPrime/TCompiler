// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of lexer for T

#include "lexer/lexer.h"

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
  LS_NUMBER_ADD_OR_SUB,
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

  // NUMBER_ADD_OR_SUB states
  LS_BINARY_NUM,
  LS_OCTAL_NUM,
  LS_HEX_NUM,
  LS_FLOAT,

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

TokenType lex(LexerInfo *info, TokenInfo *tokenInfo) {
  LexerState state = LS_START;
  StringBuilder *buffer = stringBuilderCreate();
  bool expectWide = false;
  size_t line = info->line;
  size_t character = info->character;

  // implemented as augmented DFA
  while (true) {
    // look at a character
    char c = fGet(info->file);

    switch (state) {
      case LS_START: {
        switch (c) {}
        break;
      }
      case LS_COMMENT_OR_DIVIDE: {
        switch (c) {}
        break;
      }
      case LS_LNOT_OP: {
        switch (c) {}
        break;
      }
      case LS_MOD_OP: {
        switch (c) {}
        break;
      }
      case LS_MUL_OP: {
        switch (c) {}
        break;
      }
      case LS_ASSIGN_OP: {
        switch (c) {}
        break;
      }
      case LS_XOR_OP: {
        switch (c) {}
        break;
      }
      case LS_AND_OP: {
        switch (c) {}
        break;
      }
      case LS_OR_OP: {
        switch (c) {}
        break;
      }
      case LS_LT_OP: {
        switch (c) {}
        break;
      }
      case LS_GT_OP: {
        switch (c) {}
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
      case LS_NUMBER_ADD_OR_SUB: {
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
        switch (c) {}
        break;
      }
      case LS_LOR_OP: {
        switch (c) {}
        break;
      }
      case LS_LTEQ_OP: {
        switch (c) {}
        break;
      }
      case LS_LSHIFT_OP: {
        switch (c) {}
        break;
      }
      case LS_ARSHIFT_OP: {
        switch (c) {}
        break;
      }
      case LS_LRSHIFT_OP: {
        switch (c) {}
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
    }
  }
}