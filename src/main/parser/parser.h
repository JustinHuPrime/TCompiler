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
 * parser
 */

#ifndef TLC_PARSER_PARSER_H_
#define TLC_PARSER_PARSER_H_

#include "parser/ast.h"

typedef struct FileListEntry FileListEntry;

/**
 * parses a file
 *
 * expects entry to have an uninitialized FileListEntry#lexerState, and an empty
 * FileListEntry#program
 *
 * @param entry entry to parse
 * @returns status code (0 = OK, -1 = fatal error)
 */
int parse(FileListEntry *entry);

#endif  // TLC_PARSER_PARSER_H_