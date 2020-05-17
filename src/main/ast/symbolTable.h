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

#include "util/container/hashMap.h"
#include "util/container/vector.h"

#include <stdbool.h>

/** the type of a type keword */
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
/**
 * the type of a type (i.e. kind)
 */
typedef enum {
  K_BASE,
  K_CV,
  K_POINTER,
  K_ARRAY,
  K_FUNPTR,
  K_REFERENCE,
} TypeType;
struct SymbolTableEntry;
/**
 * a type, as used by a symbol table
 */
typedef struct Type {
  TypeType type;
  union {
    struct {
      TypeKeyword keyword;
    } base; /**< basic type */
    struct {
      bool isConst;
      bool isVolatile;
    } cv; /**< add const or volatile qualification to a type */
    struct {
      struct Type *base;
    } pointer; /**< form a pointer to a base type */
    struct {
      struct Type *base;
      size_t size;
    } array; /**< form an array of size base types */
    struct {
      struct Type *returnType;
      Vector argumentTypes; /**< vector of Types */
    } funPtr;               /**< function pointer */
    struct {
      struct SymbolTableEntry const *referenced;
    } reference; /**< reference to a user-defined type */
  } data;
} Type;

/**
 * deinitializes a type
 *
 * @param t type to deinitialize
 */
void typeUninit(Type *t);

/**
 * An element of a function overload set
 */
typedef struct {
  Type returnType;
  Vector argumentTypes;
  size_t numOptional;
  bool defined;
} OverloadSetElement;

/**
 * destroys an overload set element
 *
 * @param e element to destroy
 */
void overloadSetElementDestroy(OverloadSetElement *e);

/**
 * the type of a symbol
 */
typedef enum {
  ST_OPAQUE,
  ST_STRUCT,
  ST_UNION,
  ST_ENUM,
  ST_TYPEDEF,
  ST_FUNCTION,
  ST_VARIABLE,
} SymbolType;
/**
 * the data to attach to a symbol
 *
 * a symbol table is a hashmap between the symbol name and the data
 */
typedef struct SymbolTableEntry {
  SymbolType type;
  union {
    struct {
      Vector fields; /**< vector of Types */
      Vector names;  /**< vector of c-strings */
    } structDecl;
    struct {
      Vector fields; /**< vector of Types */
      Vector names;  /**< vector of c-strings */
    } unionDecl;
    struct {
      HashMap constants; /**< map between constant name and pointer to constant
                            value */ // FIXME: how do I represent this
    } enumDecl;
    struct {
      Type actual;
    } typedefDecl;
    struct {
      Vector overloadSet; /**< vector of OverloadSetElement */
    } function;
    struct {
      Type type;    /**< type of this variable */
      bool defined; /**< has this variable been defined in this context? */
    } variable;
  } data;
  char const *file;
  size_t line;
  size_t character;
} SymbolTableEntry;

/**
 * create an empty symbol table entry of the specified type
 *
 * @returns symbol table entry
 */
SymbolTableEntry *stabEntryCreate(SymbolType type, char const *file,
                                  size_t line, size_t character);

/**
 * deinitializes a symbol table entry
 *
 * @param e SymbolTableEntry to deinitialize - not null
 */
void stabEntryDestroy(SymbolTableEntry *e);

#endif  // TLC_AST_SYMBOLTABLE_H_