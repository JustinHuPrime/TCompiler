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

#include "util/functional.h"

#include <stdlib.h>

void stabUninit(HashMap *stab) {
  hashMapUninit(stab, (void (*)(void *))stabEntryFree);
}

static Type *typeCreate(TypeKind kind) {
  Type *t = malloc(sizeof(Type));
  t->kind = kind;
  return t;
}
Type *keywordTypeCreate(TypeKeyword keyword) {
  Type *t = typeCreate(TK_KEYWORD);
  t->data.keyword.keyword = keyword;
  return t;
}
Type *modifiedTypeCreate(TypeModifier modifier, Type *modified) {
  Type *t = typeCreate(TK_MODIFIED);
  t->data.modified.modifier = modifier;
  t->data.modified.modified = modified;
  return t;
}
void typeFree(Type *t) {
  switch (t->kind) {
    case TK_MODIFIED: {
      typeFree(t->data.modified.modified);
      break;
    }
    case TK_ARRAY: {
      typeFree(t->data.array.type);
      break;
    }
    case TK_FUNPTR: {
      vectorUninit(&t->data.funPtr.argTypes, (void (*)(void *))typeFree);
      typeFree(t->data.funPtr.returnType);
      break;
    }
    case TK_AGGREGATE: {
      vectorUninit(&t->data.aggregate.types, (void (*)(void *))typeFree);
    }
    default: {
      break;  // nothing to do
    }
  }
  free(t);
}

void overloadSetFree(OverloadSetEntry *e) {
  typeFree(e->returnType);
  vectorUninit(&e->argumentTypes, (void (*)(void *))typeFree);
  free(e);
}

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
void enumConstStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                            size_t character, SymbolTableEntry *parent) {
  stabEntryInit(e, file, line, character, SK_ENUMCONST);
  e->data.enumConst.parent = parent;
}
void typedefStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                          size_t character) {
  stabEntryInit(e, file, line, character, SK_TYPEDEF);
}

void stabEntryFree(SymbolTableEntry *e) {
  switch (e->kind) {
    case SK_VARIABLE: {
      typeFree(e->data.variable.type);
      break;
    }
    case SK_FUNCTION: {
      vectorUninit(&e->data.function.overloadSet,
                   (void (*)(void *))overloadSetFree);
      break;
    }
    case SK_STRUCT: {
      vectorUninit(&e->data.structType.fieldNames, free);
      vectorUninit(&e->data.structType.fieldTypes, (void (*)(void *))typeFree);
      break;
    }
    case SK_UNION: {
      vectorUninit(&e->data.unionType.optionNames, free);
      vectorUninit(&e->data.unionType.optionTypes, (void (*)(void *))typeFree);
      break;
    }
    case SK_ENUM: {
      vectorUninit(&e->data.enumType.constantNames, nullDtor);
      vectorUninit(&e->data.enumType.constantValues,
                   (void (*)(void *))stabEntryFree);
      break;
    }
    case SK_TYPEDEF: {
      typeFree(e->data.typedefType.actual);
      break;
    }
    default: { break; }
  }
  free(e);
}