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

// Implementation of type checking

#include "typecheck/typecheck.h"

static void typecheckDecl(Node *ast, Report *report, Options const *options) {
  // TODO: write this
}
static void typecheckCode(Node *ast, Report *report, Options const *options) {
  // TODO: write this
}
void typecheck(Report *report, Options const *options,
               ModuleAstMapPair const *asts) {
  for (size_t idx = 0; idx < asts->decls.size; idx++) {
    if (asts->decls.keys[idx] != NULL) {
      typecheckDecl(asts->decls.values[idx], report, options);
    }
  }
  for (size_t idx = 0; idx < asts->codes.size; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      typecheckCode(asts->codes.values[idx], report, options);
    }
  }
}