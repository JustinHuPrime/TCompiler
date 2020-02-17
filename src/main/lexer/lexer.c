// Copyright 2020 Justin Hu
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

#include "lexer/lexer.h"

#include "constants.h"
#include "fileList.h"
#include "util/container/stringBuilder.h"
#include "util/conversions.h"
#include "util/format.h"
#include "util/functional.h"
#include "util/internalError.h"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * initializes a token
 *
 * @param state LexerState to draw line and character from
 * @param token token to initialize
 * @param type type of token
 * @param string additional data, may be null, depends on type
 */
static void tokenInit(LexerState *state, Token *token, TokenType type,
                      char *string) {
  token->type = type;
  token->line = state->line;
  token->character = state->character;
  token->string = string;

  // consistency check
  assert(token->string == NULL ||
         (token->type >= TT_ID && token->type <= TT_LIT_FLOAT));
}

void tokenUninit(Token *token) { free(token->string); }

/** keyword map */
HashMap keywordMap;
char const *const keywordStrings[] = {
    "module",  "import", "opaque",   "struct", "union",    "enum",   "typedef",
    "if",      "else",   "while",    "do",     "for",      "switch", "case",
    "default", "break",  "continue", "return", "asm",      "cast",   "sizeof",
    "true",    "false",  "null",     "void",   "ubyte",    "byte",   "char",
    "ushort",  "short",  "uint",     "int",    "wchar",    "ulong",  "long",
    "float",   "double", "bool",     "const",  "volatile",
};
TokenType const keywordTokens[] = {
    TT_MODULE,  TT_IMPORT, TT_OPAQUE,  TT_STRUCT,   TT_UNION,    TT_ENUM,
    TT_TYPEDEF, TT_IF,     TT_ELSE,    TT_WHILE,    TT_DO,       TT_FOR,
    TT_SWITCH,  TT_CASE,   TT_DEFAULT, TT_BREAK,    TT_CONTINUE, TT_RETURN,
    TT_ASM,     TT_CAST,   TT_SIZEOF,  TT_TRUE,     TT_FALSE,    TT_NULL,
    TT_VOID,    TT_UBYTE,  TT_BYTE,    TT_CHAR,     TT_USHORT,   TT_SHORT,
    TT_UINT,    TT_INT,    TT_WCHAR,   TT_ULONG,    TT_LONG,     TT_FLOAT,
    TT_DOUBLE,  TT_BOOL,   TT_CONST,   TT_VOLATILE,
};

/** magic token map */
HashMap magicMap;
char const *const magicStrings[] = {
    "__FILE__",
    "__LINE__",
    "__VERSION__",
};
typedef enum {
  MTT_FILE,
  MTT_LINE,
  MTT_VERSION,
} MagicTokenType;
MagicTokenType const magicTokens[] = {
    MTT_FILE,
    MTT_LINE,
    MTT_VERSION,
};

void lexerInitMaps(void) {
  hashMapInit(&keywordMap);
  for (size_t idx = 0; idx < 40; idx++) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    hashMapSet(&keywordMap, keywordStrings[idx], (void *)&keywordTokens[idx]);
#pragma GCC diagnostic pop
  }
  hashMapInit(&magicMap);
  for (size_t idx = 0; idx < 3; idx++) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    hashMapSet(&magicMap, magicStrings[idx], (void *)&magicTokens[idx]);
#pragma GCC diagnostic pop
  }
}

void lexerUninitMaps(void) {
  hashMapUninit(&keywordMap, nullDtor);
  hashMapUninit(&magicMap, nullDtor);
}

int lexerStateInit(FileListEntry *entry) {
  LexerState *state = &entry->lexerState;
  state->character = 0;
  state->line = 0;
  state->pushedBack = 0;

  // try to map the file
  int fd = open(entry->inputFile, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    return -1;
  }
  struct stat statbuf;
  if (fstat(fd, &statbuf) != 0) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    close(fd);
    return -1;
  }
  state->current = state->map =
      mmap(NULL, (size_t)statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (state->map == (void *)-1) {
    fprintf(stderr, "%s: error: cannot open file\n", entry->inputFile);
    return -1;
  }

  state->length = (size_t)statbuf.st_size;

  return 0;
}

/**
 * gets a character from the lexer, returns '\x04' if end of file
 */
static char get(LexerState *state) {
  if (state->current >= state->map + state->length)
    return '\x04';
  else
    return *state->current++;
}
/**
 * returns n characters to the lexer
 * must match with a get - i.e. may not put before beginning
 */
static void put(LexerState *state, size_t n) {
  state->current -= n;
  assert(state->current >= state->map);
}
/**
 * consumes whitespace while updating the entry
 * @param entry entry to munch from
 */
static int lexWhitespace(FileListEntry *entry) {
  LexerState *state = &entry->lexerState;
  bool whitespace = true;
  while (whitespace) {
    char c = get(state);
    switch (c) {
      case ' ':
      case '\t': {
        state->character += 1;
        break;
      }
      case '\n': {
        state->character = 1;
        state->line++;
        break;
      }
      case '\r': {
        state->character = 1;
        state->line++;
        // handle cr-lf
        char lf = get(state);
        if (lf != '\n') put(state, 1);
        break;
      }
      case '/': {
        char next = get(state);
        switch (next) {
          case '/': {
            // line comment
            state->character += 2;

            bool inComment = true;
            while (inComment) {
              char commentChar = get(state);
              switch (commentChar) {
                case '\x04': {
                  // eof
                  inComment = false;
                  put(state, 1);
                  break;
                }
                case '\n': {
                  inComment = false;
                  state->character = 1;
                  state->line += 1;
                  break;
                }
                case '\r': {
                  inComment = false;
                  state->character = 1;
                  state->line += 1;
                  // handle cr-lf
                  char lf = get(state);
                  if (lf != '\n') put(state, 1);
                  break;
                }
                default: {
                  // much that
                  state->character += 1;
                  break;
                }
              }
            }
            break;
          }
          case '*': {
            state->character += 2;

            bool inComment = true;
            while (inComment) {
              char commentChar = get(state);
              switch (commentChar) {
                case '\x04': {
                  fprintf(stderr,
                          "%s:%zu:%zu: error: unterminated block comment\n",
                          entry->inputFile, state->line, state->character);
                  return -1;
                }
                case '\n': {
                  state->character = 1;
                  state->line += 1;
                  break;
                }
                case '\r': {
                  state->character = 1;
                  state->line += 1;
                  // handle cr-lf
                  char lf = get(state);
                  if (lf != '\n') put(state, 1);
                  break;
                }
                case '*': {
                  // maybe the end?
                  char slash = get(state);
                  state->character += 2;
                  inComment = slash != '/';
                  break;
                }
                default: {
                  state->character += 1;
                  break;
                }
              }
            }
            break;
          }
          default: {
            // not a comment, return initial slash and second char
            put(state, 2);
            whitespace = false;
            break;
          }
        }
        break;
      }
      default: {
        // not whitespace anymore
        put(state, 1);
        whitespace = false;
        break;
      }
    }
  }

  return 0;
}

/** gets a clip as a token, from some starting pointer */
static void clip(LexerState *state, Token *token, char const *start,
                 TokenType type) {
  size_t length = (size_t)(state->current - start);
  char *clip = strncpy(malloc(length + 1), start, length);
  clip[length] = '\0';
  tokenInit(state, token, type, clip);
  state->character += length + 1;
}

/** creates a pretty-print string for a character */
static char *prettyPrintChar(char c) {
  if ((c >= ' ' && c <= '~')) {
    return format("'%c'", c);
  } else {
    uint8_t punned = charToU8(c);
    return format("'\\x%hhu%hhu'", (punned >> 4) & 0xf, (punned >> 0) & 0xf);
  }
}

/** creates an escaped version of a string */
static char *escape(char const *input) {
  StringBuilder sb;
  stringBuilderInit(&sb);

  for (char const *curr = input; *curr != '\0'; curr++) {
    char c = *curr;
    switch (*curr) {
      case '\n': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, 'n');
        break;
      }
      case '\r': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, 'r');
        break;
      }
      case '\t': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, 't');
        break;
      }
      case '\\': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, '\\');
        break;
      }
      case '\0': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, '0');
        break;
      }
      case '"': {
        stringBuilderPush(&sb, '\\');
        stringBuilderPush(&sb, '"');
        break;
      }
      default: {
        if (c >= ' ' && c <= '~' && c != '"' && c != '\\') {
          // plain character
          stringBuilderPush(&sb, c);
        } else {
          // hex escape
          stringBuilderPush(&sb, '\\');
          stringBuilderPush(&sb, 'x');
          uint8_t value = charToU8(*curr);
          stringBuilderPush(&sb, u8ToNybble((value >> 4) & 0xf));
          stringBuilderPush(&sb, u8ToNybble((value >> 0) & 0xf));
        }
        break;
      }
    }
  }

  char *retVal = stringBuilderData(&sb);
  stringBuilderUninit(&sb);
  return retVal;
}

/** lexes a hexadecimal number, prefix already lexed */
static int lexHex(FileListEntry *entry, Token *token, char const *start) {
  LexerState *state = &entry->lexerState;

  char c = get(state);
  if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F'))) {
    // error!
    fprintf(stderr, "%s:%zu:%zu: error: invalid hexadecimal integer literal\n",
            entry->inputFile, state->line, state->character);
    return -1;
  }

  while (true) {
    c = get(state);
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'))) {
      // end of number
      put(state, 1);
      clip(state, token, start, TT_LIT_INT_H);
      return 0;
    }
  }
}

/**
 * Lexes a decimal number, prefix already lexed.
 * Assumes number is at least one character long.
 */
static int lexDecimal(FileListEntry *entry, Token *token, char const *start) {
  LexerState *state = &entry->lexerState;

  TokenType type = TT_LIT_INT_D;
  while (true) {
    char c = get(state);
    if (!((c >= '0' && c <= '9') || c == '.')) {
      // end of decimal
      put(state, 1);
      clip(state, token, start, type);
      return 0;
    } else if (c == '.') {
      // is a float, actually
      type = TT_LIT_FLOAT;
    }
  }
}

/**
 * Lexes an octal number, prefix already lexed.
 * Assumes number is at least one character long.
 */
static int lexOctal(FileListEntry *entry, Token *token, char const *start) {
  LexerState *state = &entry->lexerState;

  while (true) {
    char c = get(state);
    if (!(c >= '0' && c <= '7')) {
      // end of number
      put(state, 1);
      clip(state, token, start, TT_LIT_INT_O);
      return 0;
    }
  }
}

/** lexes a binary number, prefix already lexed */
static int lexBinary(FileListEntry *entry, Token *token, char const *start) {
  LexerState *state = &entry->lexerState;

  char c = get(state);
  if (!(c >= '0' && c <= '1')) {
    // error!
    fprintf(stderr, "%s:%zu:%zu: error: invalid binary integer literal\n",
            entry->inputFile, state->line, state->character);
    return -1;
  }

  while (true) {
    c = get(state);
    if (!(c >= '0' && c <= '1')) {
      // end of number
      put(state, 1);
      clip(state, token, start, TT_LIT_INT_B);
      return 0;
    }
  }
}

/** lexes a number, must start with [+-][0-9] */
static int lexNumber(FileListEntry *entry, Token *token) {
  LexerState *state = &entry->lexerState;
  char const *start = state->current;

  char sign = get(state);

  char next;
  if (sign != '-' && sign != '+') {
    next = sign;
  } else {
    next = get(state);
  }

  if (next == '0') {
    // [+-]0
    // maybe hex, dec (float), oct, bin, or zero
    char second = get(state);
    switch (second) {
      case 'b': {
        // [+-]0b
        // is binary, lex it
        return lexBinary(entry, token, start);
      }
      case 'x': {
        // [+-]0x
        // is hex, lex it
        return lexHex(entry, token, start);
      }
      default: {
        if (second >= '0' && second <= '7') {
          // [+-]0[0-7]
          // is octal, return the digit and lex it
          put(state, 1);
          return lexOctal(entry, token, start);
        } else if (second == '.') {
          // [+-]0.
          // is a decimal, return the dot and the zero, and lex it
          put(state, 2);
          return lexDecimal(entry, token, start);
        } else {
          // just a zero - [+-]0
          clip(state, token, start, TT_LIT_INT_0);
          return 0;
        }
      }
    }
  } else {
    assert(next >= '1' && next <= '9');
    // [+-][1-9]
    // is decimal, return first char, and lex it
    put(state, 1);
    return lexDecimal(entry, token, start);
  }
}

/**
 * lexes an identifier, keyword, or magic token, first character must be a valid
 * first character for an identifier or a token.
 */
static int lexId(FileListEntry *entry, Token *token) {
  LexerState *state = &entry->lexerState;
  char const *start = state->current;

  while (true) {
    char c = get(state);
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') || c == '_')) {
      // end of identifier
      put(state, 1);
      size_t length = (size_t)(state->current - start);
      char *clip = strncpy(malloc(length + 1), start, length);
      clip[length] = '\0';

      // classify the clip
      TokenType const *keywordToken = hashMapGet(&keywordMap, clip);
      if (keywordToken != NULL) {
        // this is a keyword
        tokenInit(state, token, *keywordToken, NULL);
        state->character += length + 1;
        free(clip);
        return 0;
      }
      MagicTokenType const *magicToken = hashMapGet(&magicMap, clip);
      if (magicToken != NULL) {
        // this is a magic token
        switch (*magicToken) {
          case MTT_FILE: {
            tokenInit(state, token, TT_LIT_STRING, escape(entry->inputFile));
            state->character += length + 1;
            free(clip);
            return 0;
          }
          case MTT_LINE: {
            tokenInit(state, token, TT_LIT_INT_D, format("%zu", state->line));
            state->character += length + 1;
            free(clip);
            return 0;
          }
          case MTT_VERSION: {
            tokenInit(state, token, TT_LIT_STRING, escape(VERSION_STRING));
            state->character += length + 1;
            free(clip);
            return 0;
          }
        }
      }

      // this is a regular id
      tokenInit(state, token, TT_ID, clip);
      state->character += length + 1;
      return 0;
    }
  }
}

/**
 * lexes a string or wstring literal, opening quote already lexed
 */
static int lexString(FileListEntry *entry, Token *token) {
  LexerState *state = &entry->lexerState;
  char const *start = state->current;

  TokenType type = TT_LIT_STRING;
  while (true) {
    char c = get(state);
    switch (c) {
      case '"': {
        // end of string
        size_t length = (size_t)(state->current - start - 1);
        char *clip = strncpy(malloc(length + 1), start, length);
        clip[length] = '\0';

        if (type == TT_LIT_STRING) {
          // check for w-string-ness
          char next = get(state);
          if (next != 'w') {
            put(state, 1);
          } else {
            type = TT_LIT_WSTRING;
          }
        } else {
          // check for ending w
          char next = get(state);
          if (next != 'w') {
            free(clip);
            fprintf(stderr,
                    "%s:%zu:%zu: error: wide characters in narrow string\n",
                    entry->inputFile, state->line, state->character);
            return -1;
          }
        }

        tokenInit(state, token, type, clip);
        state->character += length + (type == TT_LIT_STRING ? 2 : 3);
        return 0;
      }
      case '\\': {
        // process escape sequence
        char next = get(state);
        switch (next) {
          case 'x': {
            // \x
            for (size_t idx = 0; idx < 2; idx++) {
              char hex = get(state);
              if (!isNybble(hex)) {
                fprintf(
                    stderr,
                    "%s:%zu:%zu: error: invalid hexadecimal escape sequence\n",
                    entry->inputFile, state->line,
                    state->character +
                        ((size_t)(state->current - start) + 1 - idx));
                return -1;
              }
            }
            break;
          }
          case 'u': {
            // \u
            for (size_t idx = 0; idx < 8; idx++) {
              char hex = get(state);
              if (!isNybble(hex)) {
                fprintf(
                    stderr,
                    "%s:%zu:%zu: error: invalid hexadecimal escape sequence\n",
                    entry->inputFile, state->line,
                    state->character +
                        ((size_t)(state->current - start) + 1 - idx));
                return -1;
              }
            }
            type = TT_LIT_WSTRING;
            break;
          }
          default: {
            if (next != 'n' && next != 'r' && next != 't' && next != '0' &&
                next != '\\' && next != '"') {
              fprintf(
                  stderr, "%s:%zu:%zu: error: unrecognized escape sequence\n",
                  entry->inputFile, state->line,
                  state->character + ((size_t)(state->current - start) + 1));
              return -1;
            }
          }
        }
        break;
      }
      default: {
        if (c == '\x04') {
          fprintf(stderr, "%s:%zu:%zu: error: unterminated string literal\n",
                  entry->inputFile, state->line,
                  state->character + ((size_t)(state->current - start) + 1));
          return -1;
        } else if (!((c >= ' ' && c <= '~' && c != '"' && c != '\\') ||
                     c == '\t')) {
          fprintf(stderr,
                  "%s:%zu:%zu: error: unsupported character encountered in "
                  "string\n",
                  entry->inputFile, state->line,
                  state->character + ((size_t)(state->current - start) + 1));
          return -1;
        }
      }
    }
  }
}

/**
 * lexes a char or wchar literal, opening quote already lexed
 */
static int lexChar(FileListEntry *entry, Token *token) {
  LexerState *state = &entry->lexerState;
  return -1;  // TODO: write this
}

int lex(FileListEntry *entry, Token *token) {
  LexerState *state = &entry->lexerState;
  // munch whitespace
  if (lexWhitespace(entry) != 0) return -1;
  // return a token
  char c = get(state);
  switch (c) {
    // EOF
    case '\x04': {
      tokenInit(state, token, TT_EOF, NULL);
      return 0;
    }

    // punctuation
    case ';': {
      tokenInit(state, token, TT_SEMI, NULL);
      state->character += 1;
      return 0;
    }
    case ',': {
      tokenInit(state, token, TT_COMMA, NULL);
      state->character += 1;
      return 0;
    }
    case '(': {
      tokenInit(state, token, TT_LPAREN, NULL);
      state->character += 1;
      return 0;
    }
    case ')': {
      tokenInit(state, token, TT_RPAREN, NULL);
      state->character += 1;
      return 0;
    }
    case '[': {
      tokenInit(state, token, TT_LSQUARE, NULL);
      state->character += 1;
      return 0;
    }
    case ']': {
      tokenInit(state, token, TT_RSQUARE, NULL);
      state->character += 1;
      return 0;
    }
    case '{': {
      tokenInit(state, token, TT_LBRACE, NULL);
      state->character += 1;
      return 0;
    }
    case '}': {
      tokenInit(state, token, TT_RBRACE, NULL);
      state->character += 1;
      return 0;
    }
    case '.': {
      tokenInit(state, token, TT_DOT, NULL);
      state->character += 1;
      return 0;
    }
    case '-': {
      char next = get(state);
      switch (next) {
        case '>': {
          // ->
          tokenInit(state, token, TT_ARROW, NULL);
          state->character += 2;
          return 0;
        }
        case '-': {
          // --
          tokenInit(state, token, TT_DEC, NULL);
          state->character += 2;
          return 0;
        }
        case '=': {
          // -=
          tokenInit(state, token, TT_SUBASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          if (next >= '0' && next <= '9') {
            // number, return - and zero
            put(state, 2);
            return lexNumber(entry, token);
          } else {
            // just -
            put(state, 1);
            tokenInit(state, token, TT_MINUS, NULL);
            state->character += 1;
            return 0;
          }
        }
      }
    }
    case '+': {
      char next = get(state);
      switch (next) {
        case '+': {
          // ++
          tokenInit(state, token, TT_INC, NULL);
          state->character += 2;
          return 0;
        }
        case '=': {
          // +=
          tokenInit(state, token, TT_ADDASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          if (next >= '0' && next <= '9') {
            // number, return + and zero
            put(state, 2);
            return lexNumber(entry, token);
          } else {
            // just +
            put(state, 1);
            tokenInit(state, token, TT_PLUS, NULL);
            state->character += 1;
            return 0;
          }
        }
      }
    }
    case '*': {
      char next = get(state);
      switch (next) {
        case '=': {
          // *=
          tokenInit(state, token, TT_MULASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just *
          put(state, 1);
          tokenInit(state, token, TT_STAR, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '&': {
      char next1 = get(state);
      switch (next1) {
        case '&': {
          // &&
          char next2 = get(state);
          switch (next2) {
            case '=': {
              // &&=
              tokenInit(state, token, TT_LANDASSIGN, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just &&
              put(state, 1);
              tokenInit(state, token, TT_LAND, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        case '=': {
          // &=
          tokenInit(state, token, TT_ANDASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just &
          put(state, 1);
          tokenInit(state, token, TT_AMP, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '!': {
      char next = get(state);
      switch (next) {
        case '=': {
          // !=
          tokenInit(state, token, TT_NEQ, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just !
          put(state, 1);
          tokenInit(state, token, TT_BANG, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '~': {
      tokenInit(state, token, TT_TILDE, NULL);
      state->character += 1;
      return 0;
    }
    case '/': {
      // note - comments dealt with in whitespace
      char next = get(state);
      switch (next) {
        case '=': {
          // /=
          tokenInit(state, token, TT_DIVASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just /
          put(state, 1);
          tokenInit(state, token, TT_SLASH, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '%': {
      char next = get(state);
      switch (next) {
        case '=': {
          // %=
          tokenInit(state, token, TT_MODASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just %
          put(state, 1);
          tokenInit(state, token, TT_PERCENT, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '<': {
      char next1 = get(state);
      switch (next1) {
        case '<': {
          // <<
          char next2 = get(state);
          switch (next2) {
            case '=': {
              tokenInit(state, token, TT_LSHIFTASSIGN, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just <<
              put(state, 1);
              tokenInit(state, token, TT_LSHIFT, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        case '=': {
          // <=
          char next2 = get(state);
          switch (next2) {
            case '>': {
              // <=>
              tokenInit(state, token, TT_SPACESHIP, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just <=
              put(state, 1);
              tokenInit(state, token, TT_LTEQ, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        default: {
          // just <
          put(state, 1);
          tokenInit(state, token, TT_LANGLE, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '>': {
      char next1 = get(state);
      switch (next1) {
        case '>': {
          // >>
          char next2 = get(state);
          switch (next2) {
            case '>': {
              // >>>
              char next3 = get(state);
              switch (next3) {
                case '=': {
                  // >>>=
                  tokenInit(state, token, TT_ARSHIFTASSIGN, NULL);
                  state->character += 4;
                  return 0;
                }
                default: {
                  // just >>>
                  put(state, 1);
                  tokenInit(state, token, TT_ARSHIFT, NULL);
                  state->character += 3;
                  return 0;
                }
              }
            }
            case '=': {
              // >>=
              tokenInit(state, token, TT_ARSHIFTASSIGN, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just >>
              put(state, 1);
              tokenInit(state, token, TT_ARSHIFT, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        case '=': {
          char next = get(state);
          switch (next) {
            case '=': {
              // ==
              tokenInit(state, token, TT_EQ, NULL);
              state->character += 2;
              return 0;
            }
            default: {
              // just =
              put(state, 1);
              tokenInit(state, token, TT_ASSIGN, NULL);
              state->character += 1;
              return 0;
            }
          }
        }
        default: {
          // just >
          put(state, 1);
          tokenInit(state, token, TT_RANGLE, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '|': {
      char next1 = get(state);
      switch (next1) {
        case '|': {
          // ||
          char next2 = get(state);
          switch (next2) {
            case '=': {
              // ||=
              tokenInit(state, token, TT_LORASSIGN, NULL);
              state->character += 3;
              return 0;
            }
            default: {
              // just ||
              put(state, 1);
              tokenInit(state, token, TT_LOR, NULL);
              state->character += 2;
              return 0;
            }
          }
        }
        case '=': {
          // |=
          tokenInit(state, token, TT_ORASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just |
          put(state, 1);
          tokenInit(state, token, TT_BAR, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '^': {
      char next = get(state);
      switch (next) {
        case '=': {
          // ^=
          tokenInit(state, token, TT_XORASSIGN, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just ^
          put(state, 1);
          tokenInit(state, token, TT_CARET, NULL);
          state->character += 1;
          return 0;
        }
      }
    }
    case '?': {
      tokenInit(state, token, TT_QUESTION, NULL);
      state->character += 1;
      return 0;
    }
    case ':': {
      char next = get(state);
      switch (next) {
        case ':': {
          // ::
          tokenInit(state, token, TT_SCOPE, NULL);
          state->character += 2;
          return 0;
        }
        default: {
          // just :
          put(state, 1);
          tokenInit(state, token, TT_COLON, NULL);
          state->character += 1;
          return 0;
        }
      }
    }

    // everything else
    default: {
      if (c >= '0' && c <= '9') {
        // number
        put(state, 1);
        return lexNumber(entry, token);
      } else if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
        // id, keyword, magic token
        put(state, 1);
        return lexId(entry, token);
      } else if (c == '"') {
        // string or wstring
        return lexString(entry, token);
      } else if (c == '\'') {
        // char or wchar
        return lexChar(entry, token);
      } else {
        // error
        char *prettyString = prettyPrintChar(c);
        fprintf(stderr, "%s:%zu:%zu: error: unexpected character: %s\n",
                entry->inputFile, state->line, state->character, prettyString);
        free(prettyString);
        return -1;
      }
    }
  }
}

void unLex(FileListEntry *entry, Token const *token) {
  // only one token of lookahead is allowed
  LexerState *state = &entry->lexerState;
  assert(!state->pushedBack);
  state->pushedBack = true;
  memcpy(&state->previous, token, sizeof(Token));
}

void lexerStateUninit(FileListEntry *entry) {
  LexerState *state = &entry->lexerState;
  munmap((void *)state->map, state->length);
  if (state->pushedBack) tokenUninit(&state->previous);
}