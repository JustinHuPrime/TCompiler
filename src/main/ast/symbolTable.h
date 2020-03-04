// Copyright 2020 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

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