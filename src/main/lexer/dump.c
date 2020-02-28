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

// lexer debug dumping

#include "lexer/dump.h"

#include "fileList.h"
#include "lexer/lexer.h"

#include <stdio.h>

static char const *const TOKEN_NAMES[] = {
    "EOF",
    "MODULE",
    "IMPORT",
    "OPAQUE",
    "STRUCT",
    "UNION",
    "ENUM",
    "TYPEDEF",
    "IF",
    "ELSE",
    "WHILE",
    "DO",
    "FOR",
    "SWITCH",
    "CASE",
    "DEFAULT",
    "BREAK",
    "CONTINUE",
    "RETURN",
    "ASM",
    "CAST",
    "SIZEOF",
    "TRUE",
    "FALSE",
    "NULL",
    "VOID",
    "UBYTE",
    "BYTE",
    "CHAR",
    "USHORT",
    "SHORT",
    "UINT",
    "INT",
    "WCHAR",
    "ULONG",
    "LONG",
    "FLOAT",
    "DOUBLE",
    "BOOL",
    "CONST",
    "VOLATILE",
    "SEMI",
    "COMMA",
    "LPAREN",
    "RPAREN",
    "LSQUARE",
    "RSQUARE",
    "LBRACE",
    "RBRACE",
    "DOT",
    "ARROW",
    "INC",
    "DEC",
    "STAR",
    "AMP",
    "PLUS",
    "MINUS",
    "BANG",
    "TILDE",
    "NEGASSIGN",
    "LNOTASSIGN",
    "NOTASSIGN",
    "SLASH",
    "PERCENT",
    "LSHIFT",
    "ARSHIFT",
    "LRSHIFT",
    "SPACESHIP",
    "LANGLE",
    "RANGLE",
    "LTEQ",
    "GTEQ",
    "EQ",
    "NEQ",
    "BAR",
    "CARET",
    "LAND",
    "LOR",
    "QUESTION",
    "COLON",
    "ASSIGN",
    "MULASSIGN",
    "DIVASSIGN",
    "MODASSIGN",
    "ADDASSIGN",
    "SUBASSIGN",
    "LSHIFTASSIGN",
    "ARSHIFTASSIGN",
    "LRSHIFTASSIGN",
    "ANDASSIGN",
    "XORASSIGN",
    "ORASSIGN",
    "LANDASSIGN",
    "LORASSIGN",
    "SCOPE",
    "ID",
    "LIT_STRING",
    "LIT_WSTRING",
    "LIT_CHAR",
    "LIT_WCHAR",
    "LIT_INT_0",
    "LIT_INT_B",
    "LIT_INT_O",
    "LIT_INT_D",
    "LIT_INT_H",
    "LIT_DOUBLE",
    "LIT_FLOAT",
};

void lexDump(FileListEntry *entry) {
  printf("%s:\n", entry->inputFile);
  if (lexerStateInit(entry) != 0) return;

  Token t;
  int retval;
  do {
    retval = lex(entry, &t);
    if (retval == 0) {
      if (t.type >= TT_ID && t.type <= TT_LIT_FLOAT)
        printf("%zu:%zu: %s (%s)\n", t.line, t.character, TOKEN_NAMES[t.type],
               t.string);
      else
        printf("%zu:%zu: %s\n", t.line, t.character, TOKEN_NAMES[t.type]);
    }
  } while (t.type != TT_EOF && retval != -1);

  lexerStateUninit(entry);
}