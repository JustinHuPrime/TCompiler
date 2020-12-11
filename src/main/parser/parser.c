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
  // IMPLEMENTATION NOTES
  //
  // Since type information is needed to disambiguate variable declarations, the
  // parse and symbol table builder are merged together.
  //
  // Pass one parses everything but function bodies - so the AST exists, but may
  // contain unparsed nodes.
  //
  // Pass two resolves imports, by first making sure each decl file uniquely
  // names an import, then linking each import with it's referenced
  // FileListEntry
  //
  // Pass three allocates symbol table entries (but doesn't fill them out
  // (mostly)) for types, and fills out the references for opaque type entries
  //
  // Pass four looks through the identifiers imported to make sure that each
  // imported identifier is always accessible (see function for detailed
  // criteria for when an identifier is inaccessible)
  //
  // Pass five fills in the symbol table entry for enum types, checking for
  // circular dependencies among enum entries (a separate pass is required
  // because enum constants may be used as compile time constants for things
  // like array lengths)
  //
  // Pass six fills in the symbol table entry for everything else, checking for
  // collisions among the entries (i.e. are all variables and functions defined
  // the same as they are declared?)
  //
  // TODO: finish writing descriptions for other passes
  int retval = 0;
  bool errored = false; /**< has any part of the whole thing errored */

  // pass 1 - parse top level stuff, without populating symbol tables
  lexerInitMaps();
  for (size_t idx = 0; idx < fileList.size; ++idx) {
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

  // pass 2 - resolve imports and check for scoped id collision between imports
  if (resolveImports() != 0) return -1;

  // pass 3 - populate stab
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    if (!fileList.entries[idx].isCode) {
      startTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    if (fileList.entries[idx].isCode) {
      startTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }
  if (errored) return -1;

  // pass 4 - check for scoped id collisions between imports
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    checkScopedIdCollisions(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;
  }
  if (errored) return -1;

  // pass 5 - build and fill in stab for enums - watch out for
  // dependency loops
  if (buildTopLevelEnumStab() != 0) return -1;

  // pass 6 - fill in stab for everything else
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    if (!fileList.entries[idx].isCode) {
      finishTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    if (fileList.entries[idx].isCode) {
      finishTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }
  if (errored) return -1;

  // pass 7 - parse unparsed nodes, writing the symbol table as we go -
  // entries are filled in

  // TODO: write this

  // pass 8 - check additional constraints and warnings

  // TODO: write this

  return 0;
}