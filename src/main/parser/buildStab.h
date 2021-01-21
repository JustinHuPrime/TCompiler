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
 * build top-level parser-use symbol table in parser
 */

#ifndef TLC_PARSER_BUILDSTAB_H_
#define TLC_PARSER_BUILDSTAB_H_

#include "fileList.h"

/**
 * builds the module map, checking for any errors, and links the import
 * statements
 *
 * @returns 0 if OK, -1 if a fatal error happened
 */
int resolveImports(void);

/**
 * starts symbol table for types at the top level of the file
 *
 * Does not fill in entries (except for opaques, where it provides the link to
 * the definition), sets entry->errored if an error happened. Expects to be
 * called on code files after corresponding decl file
 *
 * @param entry entry to process
 */
void startTopLevelStab(FileListEntry *entry);

/**
 * completes the symbol table for enums at the top level
 *
 * @returns 0 if everything is OK, -1 otherwise
 */
int buildTopLevelEnumStab(void);

/**
 * checks the imports for scoped id collisions among imports
 */
void checkScopedIdCollisions(FileListEntry *entry);

/**
 * completes the symbol table for a struct
 *
 * @param entry entry containing this declaration
 * @param body declaration
 * @param stabEntry symbol table entry to fill
 * @param env enviroment to use
 */
void finishStructStab(FileListEntry *entry, Node *body,
                      SymbolTableEntry *stabEntry, Environment *env);
/**
 * completes the symbol table for a union
 *
 * @param entry entry containing this declaration
 * @param body declaration
 * @param stabEntry symbol table entry to fill
 * @param env enviroment to use
 */
void finishUnionStab(FileListEntry *entry, Node *body,
                     SymbolTableEntry *stabEntry, Environment *env);
/**
 * completes the symbol table for an enum
 *
 * only for use by functionBody.c
 *
 * @param entry entry containing this declaration
 * @param body declaration
 * @param stabEntry symbol table entry to fill
 * @param env environment to use
 */
void finishEnumStab(FileListEntry *entry, Node *body,
                    SymbolTableEntry *stabEntry, Environment *env);
/**
 * completes the symbol table for a typedef
 *
 * @param entry entry containing this declaration
 * @param body declaration
 * @param stabEntry symbol table entry to fill
 * @param env enviroment to use
 */
void finishTypedefStab(FileListEntry *entry, Node *body,
                       SymbolTableEntry *stabEntry, Environment *env);
/**
 * completes the symbol table for
 * entries at the top level
 *
 * @param entry entry to prcess
 */
void finishTopLevelStab(FileListEntry *entry);

#endif  // TLC_PARSER_BUILDSTAB_H_