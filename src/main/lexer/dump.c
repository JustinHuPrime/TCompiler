// Copyright 2020 Justin Hu
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
    "BAD_STRING",
    "BAD_CHAR",
    "BAD_BIN",
    "BAD_HEX",
};

void lexDump(FileListEntry *entry) {
  printf("%s:\n", entry->inputFilename);
  if (lexerStateInit(entry) != 0) return;

  Token t;
  do {
    lex(entry, &t);
    if (t.type >= TT_ID && t.type <= TT_LIT_FLOAT)
      printf("%zu:%zu: %s (%s)\n", t.line, t.character, TOKEN_NAMES[t.type],
             t.string);
    else
      printf("%zu:%zu: %s\n", t.line, t.character, TOKEN_NAMES[t.type]);
  } while (t.type != TT_EOF);

  lexerStateUninit(entry);
}