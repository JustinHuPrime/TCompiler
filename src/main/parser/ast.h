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
 * abstract syntax tree definition
 */

#ifndef TLC_PARSER_AST_H_
#define TLC_PARSER_AST_H_

#include <stddef.h>

#include "util/container/vector.h"

typedef enum {
  NT_PROGRAM,
} NodeType;
typedef struct Node {
  NodeType type;
  size_t line;
  size_t character;
  union {
    struct {
      struct Node *module;
      Vector imports; /**< vector of Nodes */
      Vector body;    /**< vector of Nodes */
    } program;
  } data;
} Node;

#endif  // TLC_PARSER_AST_H_