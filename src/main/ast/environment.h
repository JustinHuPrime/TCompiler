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
  FileListEntry
      *currentModuleFile;  /**< FileListEntry reference to current module */
  Vector importFiles;      /**< Vector of FileListEntry, non-owning */
  HashMap *implicitImport; /**< symbol table for the implicit import in code
                              modules */
  Vector scopes; /**< vector of temporarily owning references to the current
                    scope (vector of symbol tables) */
} Environment;

/**
 * initialize an environment
 *
 * automatically fills in the currentModule and implicitImport pointers,
 * leaves importFiles, importTables, and scopes as empty vectors
 *
 * @param env environment to initialize
 * @param currentModuleFile FileListEntry of the current module
 */
void environmentInit(Environment *env, FileListEntry *currentModuleFile);

/**
 * looks up a symbol
 *
 * returns NULL if an error condition was hit
 *
 * @param env environment to look in
 * @param name id or scoped id node to look up
 * @param quiet boolean flag - if true, do not complain on error conditions
 */
SymbolTableEntry *environmentLookup(Environment *env, Node *name, bool quiet);

/**
 * deinitialize an environment
 *
 * @param env environment to deinitialize
 */
void environmentUninit(Environment *env);

#endif  // TLC_AST_ENVIRONMENT_H_