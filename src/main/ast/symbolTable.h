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
      Type *backingType;     /**< type used to store this enum */
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
      Type *type;   /**< type of the variable */
      size_t temp;  /**< IR temp in which it's stored (zero if global) */
      bool escapes; /**< do we ever want the address of this variable */
    } variable;
    struct {
      Type *returnType;
      Vector argumentTypes;   /**< vector of Type */
      Vector argumentEntries; /**< vector of SymbolTableEntry (non-owning) */
    } function;
  } data;
  FileListEntry *file;
  size_t line; /**< line and character of first declaration */
  size_t character;
  char const *id;
} SymbolTableEntry;

/**
 * initialize symbol table entries
 */
SymbolTableEntry *opaqueStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character, char const *id);
SymbolTableEntry *structStabEntryCreate(FileListEntry *file, size_t line,
                                        size_t character, char const *id);
SymbolTableEntry *unionStabEntryCreate(FileListEntry *file, size_t line,
                                       size_t character, char const *id);
SymbolTableEntry *enumStabEntryCreate(FileListEntry *file, size_t line,
                                      size_t character, char const *id);
SymbolTableEntry *enumConstStabEntryCreate(FileListEntry *file, size_t line,
                                           size_t character, char const *id,
                                           SymbolTableEntry *parent);
SymbolTableEntry *typedefStabEntryCreate(FileListEntry *file, size_t line,
                                         size_t character, char const *id);
SymbolTableEntry *variableStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character, char const *id);
SymbolTableEntry *functionStabEntryCreate(FileListEntry *file, size_t line,
                                          size_t character, char const *id);

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