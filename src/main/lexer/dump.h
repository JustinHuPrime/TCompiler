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