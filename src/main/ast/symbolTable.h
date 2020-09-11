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

/**
 * @file
 * symbol table definition
 */

#ifndef TLC_AST_SYMBOLTABLE_H_
#define TLC_AST_SYMBOLTABLE_H_

#include <stdbool.h>

#include "util/container/hashMap.h"
#include "util/container/vector.h"

typedef struct FileListEntry FileListEntry;

/**
 * deinits a symbol table
 *
 * @param t table to deinit
 */
void stabUninit(HashMap *t);

/** A keyword type */
typedef enum {
  TK_VOID,
  TK_UBYTE,
  TK_BYTE,
  TK_CHAR,
  TK_USHORT,
  TK_SHORT,
  TK_UINT,
  TK_INT,
  TK_WCHAR,
  TK_ULONG,
  TK_LONG,
  TK_FLOAT,
  TK_DOUBLE,
  TK_BOOL,
} TypeKeyword;

/** the kind of a simple type modifier */
typedef enum {
  TM_CONST,
  TM_VOLATILE,
  TM_POINTER,
} TypeModifier;

/** the kind of a type */
typedef enum {
  TK_KEYWORD,
  TK_MODIFIED,
  TK_ARRAY,
  TK_FUNPTR,
  TK_AGGREGATE,
  TK_REFERENCE,
} TypeKind;

struct SymbolTableEntry;
/** the type of a variable or value */
typedef struct Type {
  TypeKind kind;
  union {
    struct {
      TypeKeyword keyword;
    } keyword;
    struct {
      TypeModifier modifier;
      struct Type *modified;
    } modified; /**< canonical form for CV qualification is constness first,
                   volatility second */
    struct {
      uint64_t length;
      struct Type *type;
    } array;
    struct {
      Vector argTypes; /**< vector of Type */
      struct Type *returnType;
    } funPtr;
    struct {
      Vector types; /**< vector of Type */
    } aggregate;
    struct {
      struct SymbolTableEntry *entry;
    } reference;
  } data;
} Type;

/**
 * create a keyword type
 */
Type *keywordTypeCreate(TypeKeyword keyword);
/**
 * create a modified type
 */
Type *modifiedTypeCreate(TypeModifier modifier, Type *modified);
/**
 * create an array type
 */
Type *arrayTypeCreate(uint64_t length, Type *type);
/**
 * create a function pointer type
 *
 * argTypes is initialized as the empty vector
 */
Type *funPtrTypeCreate(Type *returnType);
/**
 * create a aggregate init type
 *
 * types is initialized as the empty vector
 */
Type *aggregateTypeCreate(void);
/**
 * create a reference type
 */
Type *referenceTypeCreate(struct SymbolTableEntry *entry);
/**
 * deep copies a type
 */
Type *typeCopy(Type *);

/**
 * deinitializes a type
 *
 * @param t type to uninit
 */
void typeFree(Type *t);

/** an entry in a function overload set */
typedef struct {
  Type *returnType;
  Vector argumentTypes; /**< vector of Type */
  size_t numOptional;
  bool defined;
} OverloadSetEntry;

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
      Vector fieldNames; /**< vector of char * (owning) */
      Vector fieldTypes; /**< vector of types */
    } structType;
    struct {
      Vector optionNames; /**< vector of char * (owning) */
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
      Vector overloadSet; /**< vector of overload set entries */
    } function;
  } data;
  FileListEntry *file;
  size_t line;
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