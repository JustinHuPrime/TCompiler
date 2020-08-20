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
  // IMPLEMENTATION NOTES:
  // Parsing and symbol table building is mixed up as a result of the syntax of
  // the language and the circular dependencies allowed. Parsing and symbol
  // table building are split into multiple passes.
  //
  // After pass 1, it is expected that everything besides function bodies is
  // parsed. Since no expressions are allowed at top level, there is no
  // ambiguity when trying to parse the top level. Function bodies are skipped
  // by matching curly braces.
  //
  // Pass 2 creates symbol table entries for every entity (variable, type, enum
  // constant), and also fills in the parent (for enum constants) and
  // (definition) for opaques. After pass 2, it should be possible to look up
  // any entity using environmentLookup for a suitablly initialized Environment.
  //
  // Per the standard (section 4.1.2) opaques from the implicit import may be
  // redefined. As such, if declaration files are processed before code files,
  // the opaque to point to a parent must already have been created, so we can
  // know exactly who should know the newly-defined type is its underlying type
  //
  // For enum constants, the parent of the enum constant contains the symbol
  // table of the enum constants, so there is no way an enum constant can ever
  // be declared outside of its parent, whose symbol table entry must already
  // have been created.
  //
  // Pass 3 prevents collisions among the scoped names imported into a module
  //
  // Pass 4 calculates the numeric value of an enumeration constant, complaining
  // if there are any circular dependencies among enumeration constants -
  // example for that case is given below:
  // enum FirstEnum {
  //   A = SecondEnum::A
  // }
  // enum SecondEnum {
  //   A = FirstEnum::A
  // }
  // Pass 4 is needed because enumeration constants are considered extended
  // integer literals, making them eligible to be the size of an array. The next
  // pass fills in the symbol table for types, which means that the size of an
  // array must be determined. If this size involves an enumeration constant,
  // the value of the enumeration constant must be known.
  //
  // Pass 5 fills in the symbol table entries for the remaining entities, using
  // the results from pass 4. At this point it's possible to check for circular
  // dependencies in types (e.g. A contains a B, B contains an A), but this can
  // be done elsewhere - the parser is complicated enough without unnecessary
  // checks!
  //
  // Pass 6 finishes parsing and building the symbol table, while pass 7
  // enforces additional requirements on the code - pass 7 could be split into a
  // separate function, but it's very naturally part of parse.

  int retval = 0;
  bool errored = false; /**< has any part of the whole thing errored */

  // pass 1 - parse top level stuff, without populating symbol tables
  lexerInitMaps();
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

  // pass 2 - resolve imports and check for scoped id collision between imports
  if (resolveImports() != 0) return -1;

  // pass 3 - populate stab for types and enums
  for (size_t idx = 0; idx < fileList.size; idx++) {
    if (!fileList.entries[idx].isCode) {
      startTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }
  for (size_t idx = 0; idx < fileList.size; idx++) {
    if (fileList.entries[idx].isCode) {
      startTopLevelStab(&fileList.entries[idx]);
      errored = errored || fileList.entries[idx].errored;
    }
  }
  if (errored) return -1;

  // pass 4 - check for scoped id collisions between imports

  // pass 5 - build and fill in stab for enums - watch out for
  // dependency loops
  if (buildTopLevelEnumStab() != 0) return -1;

  // pass 6 - fill in stab for types

  // pass 7 - parse unparsed nodes, writing the symbol table as we go -
  // entries are filled in

  // TODO: write this

  // pass 8 - check additional constraints and warnings

  // TODO: write this

  return 0;
}