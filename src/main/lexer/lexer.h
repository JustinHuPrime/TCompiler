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

/**
 * @file
 * lexer
 */

#ifndef TLC_LEXER_LEXER_H_
#define TLC_LEXER_LEXER_H_

#include <stdbool.h>
#include <stddef.h>

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
  TT_ASM,
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
  TT_SLASH,
  TT_PERCENT,
  TT_LSHIFT,
  TT_ARSHIFT,
  TT_LRSHIFT,
  TT_SPACESHIP,
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
  TT_ANDASSIGN,
  TT_XORASSIGN,
  TT_ORASSIGN,
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
  TT_LIT_INT_0,
  TT_LIT_INT_B,
  TT_LIT_INT_O,
  TT_LIT_INT_D,
  TT_LIT_INT_H,
  TT_LIT_FLOAT,
} TokenType;

/** a token */
typedef struct {
  TokenType type;
  size_t line;
  size_t character;
  char *string; /**< optional, depends on Token#type */
} Token;

/**
 * uninitializes the token
 *
 * @param token token to deinitialize
 */
void tokenUninit(Token *token);

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
 * @param entry entry to lex from
 * @param token token to write into
 * @returns status code (0 = OK)
 */
int lex(FileListEntry *entry, Token *token);
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