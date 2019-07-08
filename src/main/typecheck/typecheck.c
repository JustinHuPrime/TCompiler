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

#include <stdlib.h>

// expressions
static Type *typecheckExpression(Node *expression, Report *report,
                                 Options const *options, char const *filename) {
  // TODO: write this
  return NULL;
}

// statements
static void typecheckStmt(Node *statement, Report *report,
                          Options const *options, char const *filename) {
  // TODO: write this
}

// top level
static void typecheckFnDecl(Node *fnDecl, Report *report,
                            Options const *options, char const *filename) {
  NodePairList *params = fnDecl->data.funDecl.params;
  OverloadSetElement *overload = fnDecl->data.funDecl.overload;
  for (size_t idx = 0; idx < params->size; idx++) {
    Node *defaultArg = params->secondElements[idx];
    if (defaultArg != NULL) {
      Type *paramType =
          typecheckExpression(defaultArg, report, options, filename);
      if (paramType == NULL) {
        continue;
      } else if (!typeAssignable(overload->argumentTypes.elements[idx],
                                 paramType)) {
        char *lhsType = typeToString(overload->argumentTypes.elements[idx]);
        char *rhsType = typeToString(paramType);
        reportError(report,
                    "%s:%zu:%zu: error: cannot initialize a value of type %s "
                    "with a value of type %s",
                    filename, defaultArg->line, defaultArg->character, lhsType,
                    rhsType);
        free(lhsType);
        free(rhsType);
      }

      typeDestroy(paramType);
    }
  }
}
static void typecheckFunction(Node *function, Report *report,
                              Options const *options, char const *filename) {
  // TODO: write this
}

// file level stuff
static void typecheckDecl(Node *ast, Report *report, Options const *options) {
  NodeList *bodies = ast->data.file.bodies;
  char const *filename = ast->data.file.filename;

  for (size_t idx = 0; idx < bodies->size; idx++) {
    Node *body = bodies->elements[idx];
    if (body->type == NT_FUNDECL) {
      typecheckFnDecl(body, report, options, filename);
    }
  }
}
static void typecheckCode(Node *ast, Report *report, Options const *options) {
  NodeList *bodies = ast->data.file.bodies;
  char const *filename = ast->data.file.filename;

  for (size_t idx = 0; idx < bodies->size; idx++) {
    Node *body = bodies->elements[idx];
    if (body->type == NT_FUNDECL) {
      typecheckFnDecl(body, report, options, filename);
    } else if (body->type == NT_FUNCTION) {
      typecheckFunction(body, report, options, filename);
    }
  }
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