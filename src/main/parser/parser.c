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

int parse(void) {
  int retval = 0;

  lexerInitMaps();

  // pass one - parse and gather top-level names, leaving some nodes as unparsed
  for (size_t idx = 0; idx < fileList.size; idx++) {
    retval = lexerStateInit(&fileList.entries[idx]);
    if (retval != 0) {
      retval = -1;
      continue;
    }

    // parseFile(&fileList.entries[idx]);

    lexerStateUninit(&fileList.entries[idx]);
  }

  if (retval != 0) return retval;

  lexerUninitMaps();

  // pass two - resolve imports and parse unparsed nodes
  for (size_t idx = 0; idx < fileList.size; idx++) {
  }

  return 0;
}