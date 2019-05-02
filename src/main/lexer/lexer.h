// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// The lexer for T

#ifndef TLC_LEXER_LEXER_H_
#define TLC_LEXER_LEXER_H_

#include "util/errorReport.h"
#include "util/file.h"
#include "util/hashSet.h"

typedef enum {
  // special conditions (file conditions)
  TT_ERR,
  TT_EOF,

  // errors
  TT_INVALID,
  TT_EMPTY_SQUOTE,
  TT_INVALID_ESCAPE,

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
  TT_UINT,
  TT_INT,
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

typedef struct {
  size_t line;
  size_t character;
  union {
    char *string;
    char invalidChar;
  } data;
} TokenInfo;

typedef struct {
  size_t line;
  size_t character;
  File *file;
  HashSet const *keywords;
} LexerInfo;

HashSet *keywordSetCreate(void);
void keywordSetDestroy(HashSet *);

// creates and initializes the lexer info
LexerInfo *lexerInfoCreate(char const *fileName, HashSet const *keywords);
// dtor - note that keywords is not owned by the LexerInfo
void lexerInfoDestroy(LexerInfo *);

// takes the lexer info and reads one token from it, discarding preceeding
// whitespace recursively.
// puts auxiliary information into tokenInfo.
// note that the parser is responsible for figuring out the exact value of a
// literal
TokenType lex(Report *report, LexerInfo *info, TokenInfo *tokenInfo);

#endif  // TLC_LEXER_LEXER_H_