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

/**
 * the type of a symbol
 */
typedef enum {
  ST_TYPE,
  ST_FUNCTION,
  ST_VARIABLE,
} SymbolType;

/**
 * the data to attach to a symbol
 *
 * a symbol table is a hashmap between the symbol name and the data
 */
typedef struct {
  SymbolType type;
  // TODO: fill in the actual data
} SymbolTableEntry;

/**
 * deinitializes a symbol table entry
 *
 * @param e SymbolTableEntry to deinitialize - not null
 */
void stabEntryDeinit(SymbolTableEntry *e);

#endif  // TLC_AST_SYMBOLTABLE_H_