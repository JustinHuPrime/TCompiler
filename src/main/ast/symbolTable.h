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

/**
 * @file
 * symbol table definition
 */

#ifndef TLC_AST_SYMBOLTABLE_H_
#define TLC_AST_SYMBOLTABLE_H_

#include "ast/type.h"
#include "util/container/hashMap.h"
#include "util/container/vector.h"

typedef struct FileListEntry FileListEntry;

/**
 * deinits and frees a symbol table
 *
 * @param t table to free
 */
void stabFree(HashMap *t);

/** the kind of a symbol */
typedef enum {
  SK_VARIABLE,
  SK_FUNCTION,
  SK_OPAQUE,
  SK_STRUCT,
  SK_UNION,
  SK_ENUM,
  SK_TYPEDEF,
  SK_ENUMCONST,
} SymbolKind;

/**
 * produce stringified form (like "a variable") suitable for error messages
 */
char const *symbolKindToString(SymbolKind kind);

/** a symbol */
typedef struct SymbolTableEntry {
  SymbolKind kind;
  union {
    // types
    struct {
      struct SymbolTableEntry
          *definition; /**< actual definition of this opaque, nullable */
    } opaqueType;
    struct {
      Vector fieldNames; /**< vector of char * (non-owning) */
      Vector fieldTypes; /**< vector of types */
    } structType;
    struct {
      Vector optionNames; /**< vector of char * (non-owning) */
      Vector optionTypes; /**< vector of types */
    } unionType;
    struct {
      Vector constantNames;  /**< vector of char * (non-owning) */
      Vector constantValues; /**< vector of SymbolTableEntry (enum consts) */
    } enumType;
    struct {
      struct SymbolTableEntry
          *parent; /**< non-owning reference to parent stab entry */
      bool signedness;
      union {
        uint64_t unsignedValue;
        int64_t signedValue;
      } data;
    } enumConst;
    struct {
      Type *actual;
    } typedefType;

    // non-types
    struct {
      Type *type;
    } variable;
    struct {
      Type *returnType;
      Vector argumentTypes; /**< vector of Type */
    } function;
  } data;
  FileListEntry *file;
  size_t line; /**< line and character of first declaration */
  size_t character;
} SymbolTableEntry;

/**
 * initialize symbol table entries
 */
SymbolTableEntry *opaqueStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character);
SymbolTableEntry *structStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character);
SymbolTableEntry *unionStabEntryCreate(FileListEntry *file, size_t line,
                                       size_t character);
SymbolTableEntry *enumStabEntryCreate(FileListEntry *file, size_t line,
                                      size_t character);
SymbolTableEntry *enumConstStabEntryCreate(FileListEntry *file, size_t line,
                                           size_t character,
                                           SymbolTableEntry *parent);
SymbolTableEntry *typedefStabEntryCreate(FileListEntry *file, size_t line,
                                         size_t character);
SymbolTableEntry *variableStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character);
SymbolTableEntry *functionStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character);

/**
 * find the type associated with a field, or return NULL
 */
Type *structLookupField(SymbolTableEntry *structEntry, char const *field);
/**
 * find the type associated with an option, or return NULL
 */
Type *unionLookupOption(SymbolTableEntry *unionEntry, char const *option);
/**
 * find the enum const associated with a name, or return NULL
 */
SymbolTableEntry *enumLookupEnumConst(SymbolTableEntry *enumEntry,
                                      char const *name);

/**
 * deinitializes a symbol table entry
 *
 * @param e entry to deinitialize
 */
void stabEntryFree(SymbolTableEntry *e);

#endif  // TLC_AST_SYMBOLTABLE_H_