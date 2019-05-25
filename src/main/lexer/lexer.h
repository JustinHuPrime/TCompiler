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

// The lexer for T

#ifndef TLC_LEXER_LEXER_H_
#define TLC_LEXER_LEXER_H_

#include "util/container/hashMap.h"
#include "util/errorReport.h"
#include "util/file.h"
#include "util/fileList.h"

// the type of a token
typedef enum {
  // special conditions (file conditions)
  TT_ERR,
  TT_EOF,

  // errors
  TT_INVALID,         // invalid character seen (e.g. $)
  TT_EMPTY_SQUOTE,    // empty char ('')
  TT_INVALID_ESCAPE,  // bad escape (\$)
  TT_NOT_WIDE,        // expected wchar/wstring, got char/string
  TT_MULTICHAR_CHAR,  // multiple chars in char ('ab')

  // keywords
  TT_MODULE,
  TT_USING,
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
  TT_TRUE,
  TT_FALSE,
  TT_CAST,
  TT_SIZEOF,
  TT_VOID,
  TT_UBYTE,
  TT_BYTE,
  TT_CHAR,
  TT_UINT,
  TT_INT,
  TT_WCHAR,
  TT_ULONG,
  TT_LONG,
  TT_FLOAT,
  TT_DOUBLE,
  TT_BOOL,
  TT_CONST,

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
  TT_PLUSPLUS,
  TT_MINUSMINUS,
  TT_STAR,
  TT_AMPERSAND,
  TT_PLUS,
  TT_MINUS,
  TT_BANG,
  TT_TILDE,
  TT_SLASH,
  TT_PERCENT,
  TT_LSHIFT,
  TT_LRSHIFT,
  TT_ARSHIFT,
  TT_SPACESHIP,
  TT_LANGLE,
  TT_RANGLE,
  TT_LTEQ,
  TT_GTEQ,
  TT_EQ,
  TT_NEQ,
  TT_PIPE,
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
  TT_LRSHIFTASSIGN,
  TT_ARSHIFTASSIGN,
  TT_BITANDASSIGN,
  TT_BITXORASSIGN,
  TT_BITORASSIGN,
  TT_LANDASSIGN,
  TT_LORASSIGN,

  // identifiers
  TT_ID,
  TT_SCOPED_ID,

  // literals
  TT_LITERALINT_0,
  TT_LITERALINT_B,
  TT_LITERALINT_O,
  TT_LITERALINT_D,
  TT_LITERALINT_H,
  TT_LITERALFLOAT,
  TT_LITERALSTRING,
  TT_LITERALCHAR,
  TT_LITERALWSTRING,
  TT_LITERALWCHAR,
} TokenType;
char const *tokenToName(TokenType);

// pod object, stores the result of a lex
typedef struct {
  TokenType type;
  size_t line;
  size_t character;
  union {
    char *string;
    char invalidChar;
  } data;
} TokenInfo;
// produce true if token type is an error result handled by the lexer
bool tokenInfoIsLexerError(TokenInfo *);
// dtor
void tokenInfoUninit(TokenInfo *);

typedef struct {
  size_t line;
  size_t character;
  File *file;
  HashMap const *keywords;
  char const *fileName;
  TokenInfo previous;
  bool pushedBack;
} LexerInfo;

// creates and initializes the lexer info
LexerInfo *lexerInfoCreate(char const *fileName, HashMap const *keywords);
// dtor - note that keywords is not owned by the LexerInfo
void lexerInfoDestroy(LexerInfo *);

// specialization of hashmap
typedef HashMap KeywordMap;

KeywordMap *keywordMapCreate(void);
void keywordMapInit(KeywordMap *);
TokenType const *keywordMapGet(KeywordMap const *, char const *);
void keywordMapUninit(KeywordMap *);
void keywordMapDestroy(KeywordMap *);

// takes the lexer info and reads one token from it, discarding preceeding
// whitespace recursively.
// note that the parser is responsible for figuring out the exact value of a
// literal
void lex(TokenInfo *tokenInfo, Report *report, LexerInfo *info);
// pushes a token back into the lexer info
void unLex(LexerInfo *info, TokenInfo *tokenInfo);

// dumps the tokens from all files to stdout
void lexDump(Report *report, FileList *files);

#endif  // TLC_LEXER_LEXER_H_