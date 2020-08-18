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
 * lexical scoping environment for AST traversal
 */

#ifndef TLC_AST_ENVIRONMENT_H_
#define TLC_AST_ENVIRONMENT_H_

#include "ast/symbolTable.h"

typedef struct Node Node;

typedef struct {
  Vector importNames;      /**< Vector of NT_ID or NT_SCOPED_ID, non-owning */
  Vector importTables;     /**< Vector of symbol tables, non-owning */
  Node *currentModuleName; /**< NT_ID or NT_SCOPED_ID of the current module,
                              non-owning */
  HashMap *currentModule; /**< symbol table for the current module - this is the
                             file scope, non-owning */
  HashMap *implicitImport; /**< symbol table for the implicit import in code
                              modules */
  Vector scopes; /**< vector of temporarily owning references to the current
                    scope (vector of symbol tables) */
} Environment;

/**
 * initialize an environment
 *
 * @param env environment to initialize
 * @param currentModuleName id or scoped id of the current module
 * @param currentModule module to root the environment in
 * @param implicitImport implicit import module (null when not in a code file)
 */
void environmentInit(Environment *env, Node *currentModuleName,
                     HashMap *currentModule, HashMap *implicitImport);

/**
 * looks up a symbol
 *
 * @param env environment to look in
 * @param name id or scoped id node to look up
 */
SymbolTableEntry *environmentLookup(Environment *env, Node *name);

/**
 * deinitialize an environment
 *
 * @param env environment to deinitialize
 */
void environmentUninit(Environment *env);

#endif  // TLC_AST_ENVIRONMENT_H_