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
// The T Language Compiler. If not, see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * lexer debug-dumping
 */

#ifndef TLC_LEXER_DUMP_H_
#define TLC_LEXER_DUMP_H_

typedef struct FileListEntry FileListEntry;

/**
 * Prints the lexed results of a file to stdout. Assumes that entry has not been
 * initialized for lexing. The function lexerInitMaps() must be called first.
 *
 * @param entry entry to lex, must not be initialized for lexing already
 */
void lexDump(FileListEntry *entry);

#endif  // TLC_LEXER_DUMP_H_