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

#include "parser/parser.h"

#include "fileList.h"
#include "parser/buildStab.h"
#include "parser/topLevel.h"

int parse(void) {
  int retval = 0;
  bool errored = false; /**< has any part of the whole thing errored */

  lexerInitMaps();

  // pass one - parse top level stuff, without populating symbol tables
  for (size_t idx = 0; idx < fileList.size; idx++) {
    retval = lexerStateInit(&fileList.entries[idx]);
    if (retval != 0) {
      errored = true;
      continue;
    }

    fileList.entries[idx].ast = parseFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;

    lexerStateUninit(&fileList.entries[idx]);
  }

  lexerUninitMaps();

  if (errored) return -1;

  if (buildModuleMap() != 0) return -1;

  // pass two - populate symbol tables (but don't fill in entries)
  for (size_t idx = 0; idx < fileList.size; idx++) {
    if (!fileList.entries[idx].isCode) {
      parserBuildTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }
  for (size_t idx = 0; idx < fileList.size; idx++) {
    if (fileList.entries[idx].isCode) {
      parserBuildTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }

  if (errored) return -1;

  // pass three - parse unparsed nodes, populating the symbol table as we go
  // (entries still not filled in)

  // pass four - check additional constraints and warnings

  return 0;
}