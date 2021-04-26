// Copyright 2019-2021 Justin Hu
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

#include "lexer/dump.h"

#include <stdio.h>

#include "lexer/lexer.h"

void lexDump(FileListEntry *entry) {
  fprintf(stderr, "%s:\n", entry->inputFilename);
  if (lexerStateInit(entry) != 0) return;

  Token t;
  do {
    lex(entry, &t);
    if (t.type >= TT_ID && t.type <= TT_LIT_FLOAT)
      fprintf(stderr, "%zu:%zu: %s (%s)\n", t.line, t.character,
              TOKEN_NAMES[t.type], t.string);
    else
      fprintf(stderr, "%zu:%zu: %s\n", t.line, t.character,
              TOKEN_NAMES[t.type]);
  } while (t.type != TT_EOF);

  lexerStateUninit(entry);
}