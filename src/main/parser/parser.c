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

#include "parser/parser.h"
#include "ast/ast.h"
#include "lexer/lexer.h"

#include "fileList.h"
#include "options.h"

#include <stdlib.h>

static int parseFile(FileListEntry *entry) {
  int retval = 0;

  Node *module = malloc(sizeof(Node));

  return 0;
}

int parse(void) {
  int retval = 0;
  size_t badIdx; /**< set for uninitialization in pass one if retval != 0 */

  lexerInitMaps();

  // pass one - parse and gather top-level names, leaving some nodes as unparsed
  for (size_t idx = 0; idx < fileList.size; idx++) {
    retval = lexerStateInit(&fileList.entries[idx]);
    if (retval != 0) {
      badIdx = idx;
      continue;
    }

    retval = parseFile(&fileList.entries[idx]);
    if (retval != 0) {
      badIdx = idx;
      lexerStateUninit(&fileList.entries[idx]);
      continue;
    }

    lexerStateUninit(&fileList.entries[idx]);
  }

  if (retval != 0) {
    for (size_t idx = 0; idx < badIdx; idx++)
      nodeDeinit(&fileList.entries[idx].program);
    return retval;
  }

  lexerUninitMaps();

  // pass two - resolve imports and parse unparsed nodes
  for (size_t idx = 0; idx < fileList.size; idx++) {
    // TODO: write this
  }

  if (retval != 0) {
    for (size_t idx = 0; idx < fileList.size; idx++)
      nodeDeinit(&fileList.entries[idx].program);
    return retval;
  }

  return 0;
}