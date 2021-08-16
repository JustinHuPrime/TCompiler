// Copyright 2021 Justin Hu
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
 * translation into IR
 */

#ifndef TLC_TRANSLATION_TRANSLATION_H_
#define TLC_TRANSLATION_TRANSLATION_H_

#include <stddef.h>

#include "ast/symbolTable.h"

typedef struct FileListEntry FileListEntry;

/**
 * generate a fresh numeric identifier
 *
 * @param file numeric IDs are unique within files
 */
size_t fresh(FileListEntry *file);

/**
 * get the mangled identifier for a global symbol
 */
char *getMangledName(SymbolTableEntry *entry);

/**
 * translates all of the files in the file list into IR
 *
 * must have valid typechecked ASTs, should always succeed
 */
void translate(void);

/**
 * trace-schedules all of the files' IR
 *
 * translate must have been called first
 */
void traceSchedule(void);

#endif  // TLC_TRANSLATION_TRANSLATION_H_