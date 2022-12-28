// Copyright 2019-2020 Justin Hu
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
 * lexer
 */

#ifndef TLC_LEXER_LEXER_H_
#define TLC_LEXER_LEXER_H_

#include <stdbool.h>
#include <stddef.h>

#include "util/container/hashMap.h"

typedef struct FileListEntry FileListEntry;

/** the type of a token */
typedef enum {
  // end of file
  TT_EOF,

  // keywords
  TT_MODULE,
  TT_IMPORT,
  TT_OPAQUE,
  TT_STRUCT,
  TT_UNION,
  TT_ENUM,
  TT_TYPEDEF,
  TT_IF,
  TT_ELSE,
  TT_WHILE,
  TT_DO,
  TT_FOR,
  TT_SWITCH,
  TT_CASE,
  TT_DEFAULT,
  TT_BREAK,
  TT_CONTINUE,
  TT_RETURN,
  TT_CAST,
  TT_SIZEOF,
  TT_TRUE,
  TT_FALSE,
  TT_NULL,
  TT_VOID,
  TT_UBYTE,
  TT_BYTE,
  TT_CHAR,
  TT_USHORT,
  TT_SHORT,
  TT_UINT,
  TT_INT,
  TT_WCHAR,
  TT_ULONG,
  TT_LONG,
  TT_FLOAT,
  TT_DOUBLE,
  TT_BOOL,
  TT_CONST,
  TT_VOLATILE,

  // punctuation
  TT_SEMI,
  TT_COMMA,
  TT_LPAREN,
  TT_RPAREN,
  TT_LSQUARE,
  TT_RSQUARE,
  TT_LBRACE,
  TT_RBRACE,
  TT_DOT,
  TT_ARROW,
  TT_INC,
  TT_DEC,
  TT_STAR,
  TT_AMP,
  TT_PLUS,
  TT_MINUS,
  TT_BANG,
  TT_TILDE,
  TT_NEGASSIGN,
  TT_LNOTASSIGN,
  TT_BITNOTASSIGN,
  TT_SLASH,
  TT_PERCENT,
  TT_LSHIFT,
  TT_ARSHIFT,
  TT_LRSHIFT,
  TT_LANGLE,
  TT_RANGLE,
  TT_LTEQ,
  TT_GTEQ,
  TT_EQ,
  TT_NEQ,
  TT_BAR,
  TT_CARET,
  TT_LAND,
  TT_LOR,
  TT_QUESTION,
  TT_COLON,
  TT_ASSIGN,
  TT_MULASSIGN,
  TT_DIVASSIGN,
  TT_MODASSIGN,
  TT_ADDASSIGN,
  TT_SUBASSIGN,
  TT_LSHIFTASSIGN,
  TT_ARSHIFTASSIGN,
  TT_LRSHIFTASSIGN,
  TT_BITANDASSIGN,
  TT_BITXORASSIGN,
  TT_BITORASSIGN,
  TT_LANDASSIGN,
  TT_LORASSIGN,
  TT_SCOPE,

  // identifier
  TT_ID,

  // literals
  TT_LIT_STRING,
  TT_LIT_WSTRING,
  TT_LIT_CHAR,
  TT_LIT_WCHAR,
  TT_LIT_INT_B,
  TT_LIT_INT_O,
  TT_LIT_INT_D,
  TT_LIT_INT_H,
  TT_LIT_DOUBLE,
  TT_LIT_FLOAT,

  // error tokens
  TT_BAD_STRING,
  TT_BAD_CHAR,
  TT_BAD_BIN,
  TT_BAD_HEX,
} TokenType;
extern char const *const TOKEN_NAMES[];

/** a token */
typedef struct {
  TokenType type;
  size_t line;
  size_t character;
  char *string; /**< optional, depends on Token#type. For ids, contains the
                   string of the id. For strings and chars, contains the data
                   between the quotes (quotes excluded), for numbers, contains
                   the whole number (sign and prefix included) */
} Token;

/**
 * uninitializes the token
 *
 * only needs to be called if token has a string that isn't used (i.e. token is
 * in the range TT_ID to TT_LIT_FLOAT and Token#string was ignored)
 *
 * @param token token to deinitialize
 */
void tokenUninit(Token *token);

/**
 * Initializes keyword and magic token maps - must be called before any lexing
 * is done
 */
void lexerInitMaps(void);

/**
 * Deinitializes keyword and magic token maps - may be called after all lexing
 * is done
 */
void lexerUninitMaps(void);

/** internal state for a lexer for some file */
typedef struct {
  char *map;           /**< mmap of file */
  size_t length;       /**< length of file */
  char const *current; /**< character about to be read */

  size_t line;
  size_t character;

  Token previous;
  bool pushedBack;
} LexerState;

/**
 * Initializes the internal lexer state for a file entry
 *
 * @param entry entry to initialize
 * @returns status code (0 = OK)
 */
int lexerStateInit(FileListEntry *entry);

/**
 * lexes one token
 *
 * @param entry entry to lex from - may set error flag on this entry
 * @param token token to write into
 */
void lex(FileListEntry *entry, Token *token);
/**
 * pushes back one token. Must not call this twice in a row.
 *
 * @param entry entry to return token to
 * @param token token to return
 */
void unLex(FileListEntry *entry, Token const *token);

/**
 * Uninitializes internal lexer state for the entry
 *
 * @param entry entry to uninitialize
 */
void lexerStateUninit(FileListEntry *entry);

#endif  // TLC_LEXER_LEXER_H_
