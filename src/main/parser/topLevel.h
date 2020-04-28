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
 * top-level parsing
 */

#ifndef TLC_PARSER_TOPLEVEL_H_
#define TLC_PARSER_TOPLEVEL_H_

#include "ast/ast.h"
#include "fileList.h"

/**
 * parses a file's top level, leaving function bodies unparsed
 *
 * @param entry entry to lex from
 * @returns AST node or NULL if error happened
 */
Node *parseFile(FileListEntry *entry);

#endif  // TLC_PARSER_TOPLEVEL_H_