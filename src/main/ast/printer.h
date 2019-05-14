// Copyright 2019 Justin Hu
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

// Printers for ASTs, to verify correctness

#ifndef TLC_AST_PRINTER_H_
#define TLC_AST_PRINTER_H_

#include "ast/ast.h"

// prints the structure of a node, in function format
void printNodeStructure(Node const *);

// prints the original code, modulo whitespace and ignored syntactic elements
void printNode(Node const *);

#endif  // TLC_AST_PRINTER_H_