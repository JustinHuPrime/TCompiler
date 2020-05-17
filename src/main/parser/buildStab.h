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
 * builds the module map, checking for any errors
 *
 * @returns 0 if OK, -1 if a fatal error happened
 */
int buildModuleMap(void);

/**
 * builds symbol table as used by the parser for the top level of the file
 *
 * sets entry->errored if an error happened.
 *
 * @param entry entry to process
 */
void parserBuildTopLevelStab(FileListEntry *entry);

#endif  // TLC_PARSER_BUILDSTAB_H_