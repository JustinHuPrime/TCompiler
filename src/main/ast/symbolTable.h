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
  TK_FNPTR,
  TK_AGGREGATE,
} TypeKind;

/** the type of a variable or value */
typedef struct {
  TypeKind kind;
  union {
    struct {
      TypeKeyword keyword;
    } keyword;
  } data;
} Type;

/** the kind of a symbol */
typedef enum {
  SK_VARIABLE,
  SK_FUNCTION,
  SK_TYPE,
  SK_ENUMCONST,
} SymbolKind;

/** a symbol */
typedef struct {
  SymbolKind kind;
  union {
    struct {
      Type type;
    } variable;
    struct {
      Vector overloadSet;
    } function;
  } data;
  char const *file;
  size_t line;
  size_t character;
} SymbolTableEntry;

/**
 * initialize a function symbol table entry
 *
 * @param e entry to initialize
 * @param file entry location info
 * @param line entry location info
 * @param character entry location info
 */
void functionStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                           size_t character);
/**
 * initialize a variable symbol table entry
 *
 * @param e entry to initialize
 * @param file entry location info
 * @param line entry location info
 * @param character entry location info
 */
void variableStabEntryInit(SymbolTableEntry *e, char const *file, size_t line,
                           size_t character);
/**
 * deinitializes a symbol table entry
 *
 * @param e entry to deinitialize
 */
void stabEntryUninit(SymbolTableEntry *e);

#endif  // TLC_AST_SYMBOLTABLE_H_