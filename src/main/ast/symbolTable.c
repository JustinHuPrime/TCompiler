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

// symbol table implementation

#include "ast/symbolTable.h"

#include <stdlib.h>

/**
 * initializes a symbol table entry
 */
static void stabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                          size_t character, SymbolKind kind) {
  e->kind = kind;
  e->file = file;
  e->line = line;
  e->character = character;
}

void opaqueStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                         size_t character) {
  stabEntryInit(e, file, line, character, SK_OPAQUE);
  e->data.opaqueType.definition = NULL;
}
void structStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                         size_t character) {
  stabEntryInit(e, file, line, character, SK_STRUCT);
  vectorInit(&e->data.structType.fieldNames);
  vectorInit(&e->data.structType.fieldTypes);
}
void unionStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                        size_t character) {
  stabEntryInit(e, file, line, character, SK_UNION);
  vectorInit(&e->data.unionType.optionNames);
  vectorInit(&e->data.unionType.optionTypes);
}
void enumStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                       size_t character) {
  stabEntryInit(e, file, line, character, SK_ENUM);
  vectorInit(&e->data.enumType.constantNames);
  vectorInit(&e->data.enumType.constantValues);
}
void typedefStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                          size_t character) {
  stabEntryInit(e, file, line, character, SK_TYPEDEF);
}

void stabEntryUninit(SymbolTableEntry *e) {
  // TODO: write this
}