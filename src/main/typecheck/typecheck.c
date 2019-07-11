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
  switch (expression->type) {}
}

// statements
static void typecheckVarDecl(Node *, Report *, Options const *, char const *);
static void typecheckStmt(Node *statement, Report *report,
                          Options const *options, char const *filename,
                          Type const *expectedReturnType) {
  if (statement == NULL) {
    return;
  } else {
    switch (statement->type) {
      case NT_COMPOUNDSTMT: {
        NodeList *statements = statement->data.compoundStmt.statements;
        for (size_t idx = 0; idx < statements->size; idx++) {
          typecheckStmt(statements->elements[idx], report, options, filename,
                        expectedReturnType);
        }
        break;
      }
      case NT_IFSTMT: {
        Type *conditionType = typecheckExpression(
            statement->data.ifStmt.condition, report, options, filename);
        if (conditionType != NULL) {
          if (!typeIsBoolean(conditionType)) {
            reportError(report,
                        "%s:%zu:%zu: error: condition in an 'if' statement "
                        "must be assignable to a boolean value",
                        filename, statement->data.ifStmt.condition->line,
                        statement->data.ifStmt.condition->character);
          }
          typeDestroy(conditionType);
        }
        typecheckStmt(statement->data.ifStmt.thenStmt, report, options,
                      filename, expectedReturnType);
        typecheckStmt(statement->data.ifStmt.elseStmt, report, options,
                      filename, expectedReturnType);
        break;
      }
      case NT_WHILESTMT: {
        Type *conditionType = typecheckExpression(
            statement->data.whileStmt.condition, report, options, filename);
        if (conditionType != NULL) {
          if (!typeIsBoolean(conditionType)) {
            reportError(report,
                        "%s:%zu:%zu: error: condition in an 'while' statement "
                        "must be assignable to a boolean value",
                        filename, statement->data.whileStmt.condition->line,
                        statement->data.whileStmt.condition->character);
          }
          typeDestroy(conditionType);
        }
        typecheckStmt(statement->data.whileStmt.body, report, options, filename,
                      expectedReturnType);
        break;
      }
      case NT_DOWHILESTMT: {
        typecheckStmt(statement->data.doWhileStmt.condition, report, options,
                      filename, expectedReturnType);
        Type *conditionType = typecheckExpression(
            statement->data.doWhileStmt.condition, report, options, filename);
        if (conditionType != NULL) {
          if (!typeIsBoolean(conditionType)) {
            reportError(report,
                        "%s:%zu:%zu: error: condition in a 'do-while' "
                        "statement must be assignable to a boolean value",
                        filename, statement->data.doWhileStmt.condition->line,
                        statement->data.doWhileStmt.condition->character);
          }
          typeDestroy(conditionType);
        }
        break;
      }
      case NT_FORSTMT: {
        if (statement->data.forStmt.initialize != NULL) {
          if (statement->data.forStmt.initialize->type == NT_VARDECL) {
            typecheckStmt(statement->data.forStmt.initialize, report, options,
                          filename, expectedReturnType);
          } else {
            Type *exprType = typecheckExpression(
                statement->data.forStmt.initialize, report, options, filename);
            if (exprType != NULL) {
              typeDestroy(exprType);
            }
          }
        }
        Type *conditionType = typecheckExpression(
            statement->data.forStmt.condition, report, options, filename);
        if (conditionType != NULL) {
          if (!typeIsBoolean(conditionType)) {
            reportError(report,
                        "%s:%zu:%zu: error: condition in an 'for' statement "
                        "must be assignable to a boolean value",
                        filename, statement->data.forStmt.condition->line,
                        statement->data.forStmt.condition->character);
          }
          typeDestroy(conditionType);
        }
        if (statement->data.forStmt.update != NULL) {
          Type *exprType = typecheckExpression(statement->data.forStmt.update,
                                               report, options, filename);
          if (exprType != NULL) {
            typeDestroy(exprType);
          }
        }
        typecheckStmt(statement->data.forStmt.body, report, options, filename,
                      expectedReturnType);
        break;
      }
      case NT_SWITCHSTMT: {
        Type *switchedOnType = typecheckExpression(
            statement->data.switchStmt.onWhat, report, options, filename);
        if (switchedOnType != NULL) {
          if (!typeIsIntegral(switchedOnType)) {
            reportError(report,
                        "%s:%zu:%zu: error: switched on value in a 'switch' "
                        "statement must be an integral type",
                        filename, statement->data.switchStmt.onWhat->line,
                        statement->data.switchStmt.onWhat->character);
          }
          typeDestroy(switchedOnType);
        }
        NodeList *cases = statement->data.switchStmt.cases;
        for (size_t idx = 0; idx < cases->size; idx++) {
          Node *switchCase = cases->elements[idx];
          typecheckStmt(switchCase->type == NT_NUMCASE
                            ? switchCase->data.numCase.body
                            : switchCase->data.defaultCase.body,
                        report, options, filename, expectedReturnType);
        }
        break;
      }
      case NT_RETURNSTMT: {
        if (statement->data.returnStmt.value != NULL) {
          Type *returnType = typecheckExpression(
              statement->data.returnStmt.value, report, options, filename);
          if (returnType != NULL) {
            if (expectedReturnType->kind == K_VOID) {
              char *returnTypeString = typeToString(returnType);
              reportError(report,
                          "%s:%zu:%zu: error: cannot return a value of type "
                          "'%s' from a void function",
                          filename, statement->data.returnStmt.value->line,
                          statement->data.returnStmt.value->character,
                          returnTypeString);
              free(returnTypeString);
            } else if (!typeAssignable(expectedReturnType, returnType)) {
              char *returnTypeString = typeToString(returnType);
              char *expectedTypeString = typeToString(expectedReturnType);
              reportError(
                  report,
                  "%s:%zu:%zu: error: cannot return a value of type '%s' from "
                  "a function declared to return a value of type '%s'",
                  filename, statement->data.returnStmt.value->line,
                  statement->data.returnStmt.value->character, returnTypeString,
                  expectedTypeString);
              free(returnTypeString);
              free(expectedTypeString);
            }
            typeDestroy(returnType);
          }
        } else if (expectedReturnType->kind != K_VOID) {
          switch (optionsGet(options, optionWVoidReturn)) {
            case O_WT_ERROR: {
              reportError(
                  report,
                  "%s:%zu:%zu: error: returning void in a non-void function",
                  filename, statement->line, statement->character);
              break;
            }
            case O_WT_WARN: {
              reportWarning(
                  report,
                  "%s:%zu:%zu: warning: returning void in a non-void function",
                  filename, statement->line, statement->character);
              break;
            }
            case O_WT_IGNORE: {
              break;
            }
          }
        }
        break;
      }
      case NT_EXPRESSIONSTMT: {
        Type *exprType =
            typecheckExpression(statement->data.expressionStmt.expression,
                                report, options, filename);
        if (exprType != NULL) {
          typeDestroy(exprType);
        }
        break;
      }
      case NT_VARDECL: {
        typecheckVarDecl(statement, report, options, filename);
        break;
      }
      default: {
        break;  // no expressions to deal with
      }
    }
  }
}

// top level
static void typecheckFnDecl(Node *fnDecl, Report *report,
                            Options const *options, char const *filename) {
  NodePairList *params = fnDecl->data.fnDecl.params;
  OverloadSetElement *overload = fnDecl->data.fnDecl.id->data.id.overload;
  if (overload->returnType->kind == K_CONST) {
    // const return type
    switch (optionsGet(options, optionWConstReturn)) {
      case O_WT_ERROR: {
        reportError(
            report,
            "%s:%zu:%zu: error: function declared as returing a constant value",
            filename, fnDecl->data.fnDecl.returnType->line,
            fnDecl->data.fnDecl.returnType->character);
        break;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: function declared as returing a "
                      "constant value",
                      filename, fnDecl->data.fnDecl.returnType->line,
                      fnDecl->data.fnDecl.returnType->character);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  }
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
                    "%s:%zu:%zu: error: cannot initialize an argument of type "
                    "'%s' with a value of type '%s'",
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
  NodeTripleList *params = function->data.function.formals;
  OverloadSetElement *overload = function->data.function.id->data.id.overload;
  if (overload->returnType->kind == K_CONST) {
    // const return type
    switch (optionsGet(options, optionWConstReturn)) {
      case O_WT_ERROR: {
        reportError(
            report,
            "%s:%zu:%zu: error: function declared as returing a constant value",
            filename, function->data.function.returnType->line,
            function->data.function.returnType->character);
        break;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: function declared as returing a "
                      "constant value",
                      filename, function->data.function.returnType->line,
                      function->data.function.returnType->character);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  }
  for (size_t idx = 0; idx < params->size; idx++) {
    Node *defaultArg = params->thirdElements[idx];
    if (defaultArg != NULL) {
      Type *paramType =
          typecheckExpression(defaultArg, report, options, filename);
      if (paramType == NULL) {
        continue;
      } else if (!typeAssignable(overload->argumentTypes.elements[idx],
                                 paramType)) {
        char *lhsType = typeToString(overload->argumentTypes.elements[idx]);
        char *rhsType = typeToString(paramType);
        Node *param = params->secondElements[idx];
        reportError(report,
                    "%s:%zu:%zu: error: cannot initialize '%s' (%s) with a "
                    "value of type '%s'",
                    filename, defaultArg->line, defaultArg->character,
                    param->data.id.id, lhsType, rhsType);
        free(lhsType);
        free(rhsType);
      }

      typeDestroy(paramType);
    }
  }

  typecheckStmt(function->data.function.body, report, options, filename,
                overload->returnType);
}
static void typecheckVarDecl(Node *varDecl, Report *report,
                             Options const *options, char const *filename) {
  NodePairList *idValuePairs = varDecl->data.varDecl.idValuePairs;
  for (size_t idx = 0; idx < idValuePairs->size; idx++) {
    Node *initValue = idValuePairs->secondElements[idx];
    if (initValue != NULL) {
      Node *name = idValuePairs->firstElements[idx];
      SymbolInfo *info = name->data.id.symbol;
      Type *initType =
          typecheckExpression(initValue, report, options, filename);
      if (initType == NULL) {
        continue;
      } else if (!typeAssignable(info->data.var.type, initType)) {
        char *lhsType = typeToString(info->data.var.type);
        char *rhsType = typeToString(initType);
        reportError(report,
                    "%s:%zu:%zu: error: cannot initialize '%s' (%s) with a "
                    "value of type '%s'",
                    filename, initValue->line, initValue->character,
                    name->data.id.id, lhsType, rhsType);
        free(lhsType);
        free(rhsType);
      }
      typeDestroy(initType);
    }
  }
}

// file level stuff
static void typecheckDecl(Node *ast, Report *report, Options const *options) {
  NodeList *bodies = ast->data.file.bodies;
  char const *filename = ast->data.file.filename;

  for (size_t idx = 0; idx < bodies->size; idx++) {
    Node *body = bodies->elements[idx];
    if (body->type == NT_FNDECL) {
      typecheckFnDecl(body, report, options, filename);
    }
  }
}
static void typecheckCode(Node *ast, Report *report, Options const *options) {
  NodeList *bodies = ast->data.file.bodies;
  char const *filename = ast->data.file.filename;

  for (size_t idx = 0; idx < bodies->size; idx++) {
    Node *body = bodies->elements[idx];
    if (body->type == NT_FNDECL) {
      typecheckFnDecl(body, report, options, filename);
    } else if (body->type == NT_FUNCTION) {
      typecheckFunction(body, report, options, filename);
    } else if (body->type == NT_VARDECL) {
      typecheckVarDecl(body, report, options, filename);
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