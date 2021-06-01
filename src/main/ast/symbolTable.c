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

// symbol table implementation

#include "ast/symbolTable.h"

#include <stdlib.h>

#include "fileList.h"
#include "util/functional.h"

void stabFree(HashMap *stab) {
  if (stab != NULL) {
    hashMapUninit(stab, (void (*)(void *))stabEntryFree);
    free(stab);
  }
}

static char const *const SYMBOL_KIND_NAMES[] = {
    "a variable",     "a function",
    "an opaque type", "a structure type",
    "a union type",   "an enumeration type",
    "a type alias",   "an enumeration constant",
};
char const *symbolKindToString(SymbolKind kind) {
  return SYMBOL_KIND_NAMES[kind];
}

/**
 * initializes a symbol table entry
 */
static SymbolTableEntry *stabEntryCreate(FileListEntry *file, size_t line,
                                         size_t character, char const *id,
                                         SymbolKind kind) {
  SymbolTableEntry *e = malloc(sizeof(SymbolTableEntry));
  e->kind = kind;
  e->file = file;
  e->line = line;
  e->character = character;
  e->id = id;
  return e;
}

SymbolTableEntry *opaqueStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character, char const *id) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, id, SK_OPAQUE);
  e->data.opaqueType.definition = NULL;
  return e;
}
SymbolTableEntry *structStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character, char const *id) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, id, SK_STRUCT);
  vectorInit(&e->data.structType.fieldNames);
  vectorInit(&e->data.structType.fieldTypes);
  return e;
}
SymbolTableEntry *unionStabEntryCreate(FileListEntry *file, size_t line,
                                       size_t character, char const *id) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, id, SK_UNION);
  vectorInit(&e->data.unionType.optionNames);
  vectorInit(&e->data.unionType.optionTypes);
  return e;
}
SymbolTableEntry *enumStabEntryCreate(FileListEntry *file, size_t line,
                                      size_t character, char const *id) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, id, SK_ENUM);
  vectorInit(&e->data.enumType.constantNames);
  vectorInit(&e->data.enumType.constantValues);
  e->data.enumType.backingType = NULL;
  return e;
}
SymbolTableEntry *enumConstStabEntryCreate(FileListEntry *file, size_t line,
                                           size_t character, char const *id,
                                           SymbolTableEntry *parent) {
  SymbolTableEntry *e =
      stabEntryCreate(file, line, character, id, SK_ENUMCONST);
  e->data.enumConst.parent = parent;
  return e;
}
SymbolTableEntry *typedefStabEntryCreate(FileListEntry *file, size_t line,
                                         size_t character, char const *id) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, id, SK_TYPEDEF);
  e->data.typedefType.actual = NULL;
  return e;
}
SymbolTableEntry *variableStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character, char const *id) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, id, SK_VARIABLE);
  e->data.variable.type = NULL;
  e->data.variable.temp = 0;
  return e;
}
SymbolTableEntry *functionStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character, char const *id) {
  SymbolTableEntry *e = stabEntryCreate(file, line, character, id, SK_FUNCTION);
  e->data.function.returnType = NULL;
  vectorInit(&e->data.function.argumentTypes);
  return e;
}

Type *structLookupField(SymbolTableEntry *structEntry, char const *field) {
  for (size_t idx = 0; idx < structEntry->data.structType.fieldNames.size;
       ++idx) {
    if (strcmp(structEntry->data.structType.fieldNames.elements[idx], field) ==
        0)
      return structEntry->data.structType.fieldTypes.elements[idx];
  }
  return NULL;
}
Type *unionLookupOption(SymbolTableEntry *unionEntry, char const *option) {
  for (size_t idx = 0; idx < unionEntry->data.unionType.optionNames.size;
       ++idx) {
    if (strcmp(unionEntry->data.unionType.optionNames.elements[idx], option) ==
        0)
      return unionEntry->data.unionType.optionTypes.elements[idx];
  }
  return NULL;
}
SymbolTableEntry *enumLookupEnumConst(SymbolTableEntry *enumEntry,
                                      char const *name) {
  for (size_t idx = 0; idx < enumEntry->data.enumType.constantNames.size;
       ++idx) {
    if (strcmp(enumEntry->data.enumType.constantNames.elements[idx], name) == 0)
      return enumEntry->data.enumType.constantValues.elements[idx];
  }
  return NULL;
}

void stabEntryFree(SymbolTableEntry *e) {
  switch (e->kind) {
    case SK_STRUCT: {
      vectorUninit(&e->data.structType.fieldNames, nullDtor);
      vectorUninit(&e->data.structType.fieldTypes, (void (*)(void *))typeFree);
      break;
    }
    case SK_UNION: {
      vectorUninit(&e->data.unionType.optionNames, nullDtor);
      vectorUninit(&e->data.unionType.optionTypes, (void (*)(void *))typeFree);
      break;
    }
    case SK_ENUM: {
      vectorUninit(&e->data.enumType.constantNames, nullDtor);
      vectorUninit(&e->data.enumType.constantValues,
                   (void (*)(void *))stabEntryFree);
      typeFree(e->data.enumType.backingType);
      break;
    }
    case SK_TYPEDEF: {
      typeFree(e->data.typedefType.actual);
      break;
    }
    case SK_VARIABLE: {
      typeFree(e->data.variable.type);
      break;
    }
    case SK_FUNCTION: {
      typeFree(e->data.function.returnType);
      vectorUninit(&e->data.function.argumentTypes, (void (*)(void *))typeFree);
      break;
    }
    default: {
      break;
    }
  }
  free(e);
}