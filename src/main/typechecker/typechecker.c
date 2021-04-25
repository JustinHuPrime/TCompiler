// Copyright 2019, 2021 Justin Hu
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
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "typechecker/typechecker.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileList.h"
#include "internalError.h"

/**
 * static bool type to compare with for conditionals
 */
static Type *boolType;

/**
 * complains about being unable to convert a value implicitly
 *
 * @param from from type
 * @param to to type
 */
static void errorNoImplicitConversion(FileListEntry *entry, size_t line,
                                      size_t character, Type const *from,
                                      Type const *to) {
  char *fromString = typeToString(from);
  char *toString = typeToString(to);
  fprintf(stderr,
          "%s:%zu:%zu: error: cannot implicitly convert a value of type '%s' "
          "to a value of type '%s'\n",
          entry->inputFilename, line, character, fromString, toString);
  free(fromString);
  free(toString);
  entry->errored = true;
}

/**
 * typechecks an expression
 *
 * @param exp expression to typecheck
 * @param entry entry containing this expression
 */
static Type const *typecheckExpression(Node *exp, FileListEntry *entry) {
  return NULL;  // TODO
}

/**
 * typechecks a statement
 *
 * @param stmt statement to typecheck
 * @param returnType return type
 * @param entry entry containing this statement
 */
static void typecheckStmt(Node *stmt, Type const *returnType,
                          FileListEntry *entry) {
  if (stmt == NULL) return;

  switch (stmt->type) {
    case NT_COMPOUNDSTMT: {
      Vector *stmts = stmt->data.compoundStmt.stmts;
      for (size_t idx = 0; idx < stmts->size; ++idx)
        typecheckStmt(stmts->elements[idx], returnType, entry);
      break;
    }
    case NT_IFSTMT: {
      Type const *predicateType =
          typecheckExpression(stmt->data.ifStmt.predicate, entry);
      if (predicateType != NULL &&
          !typeImplicitlyConvertable(predicateType, boolType)) {
        errorNoImplicitConversion(entry, stmt->data.ifStmt.predicate->line,
                                  stmt->data.ifStmt.predicate->character,
                                  predicateType, boolType);
      }
      typecheckStmt(stmt->data.ifStmt.consequent, returnType, entry);
      typecheckStmt(stmt->data.ifStmt.alternative, returnType, entry);
      break;
    }
    case NT_WHILESTMT: {
      Type const *conditionType =
          typecheckExpression(stmt->data.whileStmt.condition, entry);
      if (conditionType != NULL &&
          !typeImplicitlyConvertable(conditionType, boolType)) {
        errorNoImplicitConversion(entry, stmt->data.whileStmt.condition->line,
                                  stmt->data.whileStmt.condition->character,
                                  conditionType, boolType);
      }
      typecheckStmt(stmt->data.whileStmt.body, returnType, entry);
      break;
    }
    case NT_DOWHILESTMT: {
      typecheckStmt(stmt->data.doWhileStmt.body, returnType, entry);
      Type const *conditionType =
          typecheckExpression(stmt->data.doWhileStmt.condition, entry);
      if (conditionType != NULL &&
          !typeImplicitlyConvertable(conditionType, boolType)) {
        errorNoImplicitConversion(entry, stmt->data.doWhileStmt.condition->line,
                                  stmt->data.doWhileStmt.condition->character,
                                  conditionType, boolType);
      }
      break;
    }
    case NT_FORSTMT: {
      typecheckStmt(stmt->data.forStmt.initializer, returnType, entry);
      Type const *conditionType =
          typecheckExpression(stmt->data.forStmt.condition, entry);
      if (conditionType != NULL &&
          !typeImplicitlyConvertable(conditionType, boolType)) {
        errorNoImplicitConversion(entry, stmt->data.forStmt.condition->line,
                                  stmt->data.forStmt.condition->character,
                                  conditionType, boolType);
      }
      if (stmt->data.forStmt.increment != NULL)
        typecheckExpression(stmt->data.forStmt.increment, entry);
      typecheckStmt(stmt->data.forStmt.body, returnType, entry);
      break;
    }
    case NT_SWITCHSTMT: {
      Type const *conditionType =
          typecheckExpression(stmt->data.switchStmt.condition, entry);

      Vector *cases = stmt->data.switchStmt.cases;
      for (size_t caseIdx = 0; caseIdx < cases->size; ++caseIdx) {
        Node *caseNode = cases->elements[caseIdx];
        switch (caseNode->type) {
          case NT_SWITCHCASE: {
            Vector *values = caseNode->data.switchCase.values;
            for (size_t valueIdx = 0; valueIdx < values->size; ++valueIdx) {
              Node *value = values->elements[valueIdx];
              Type const *valueType = typecheckExpression(value, entry);
              if (conditionType != NULL && valueType != NULL &&
                  !typeImplicitlyConvertable(valueType, conditionType)) {
                errorNoImplicitConversion(entry, value->line, value->character,
                                          valueType, conditionType);
              }
            }
            typecheckStmt(caseNode->data.switchCase.body, returnType, entry);
            break;
          }
          case NT_SWITCHDEFAULT: {
            typecheckStmt(caseNode->data.switchDefault.body, returnType, entry);
            break;
          }
          default: {
            error(__FILE__, __LINE__, "invalid switch case type encountered");
          }
        }
      }
      break;
    }
    case NT_RETURNSTMT: {
      if (stmt->data.returnStmt.value != NULL) {
        Type const *valueType =
            typecheckExpression(stmt->data.returnStmt.value, entry);
        if (valueType != NULL &&
            !typeImplicitlyConvertable(valueType, returnType)) {
          errorNoImplicitConversion(entry, stmt->data.returnStmt.value->line,
                                    stmt->data.returnStmt.value->character,
                                    valueType, returnType);
        }
      }
      break;
    }
    case NT_VARDEFNSTMT: {
      Vector *names = stmt->data.varDefnStmt.names;
      Vector *initializers = stmt->data.varDefnStmt.initializers;
      for (size_t idx = 0; idx < names->size; ++idx) {
        Node *name = names->elements[idx];
        Node *initializer = initializers->elements[idx];
        if (initializer != NULL) {
          Type const *initializerType = typecheckExpression(initializer, entry);
          if (initializerType != NULL &&
              !typeImplicitlyConvertable(
                  initializerType, name->data.id.entry->data.variable.type)) {
            errorNoImplicitConversion(entry, initializer->line,
                                      initializer->character, initializerType,
                                      name->data.id.entry->data.variable.type);
          }
        }
      }
      break;
    }
    case NT_EXPRESSIONSTMT: {
      typecheckExpression(stmt->data.expressionStmt.expression, entry);
      break;
    }
    default: {
      break;  // nothing to check
    }
  }
}

/**
 * typechecks a code file
 *
 * @param entry entry to typecheck
 */
static void typecheckFile(FileListEntry *entry) {
  Vector *bodies = entry->ast->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; ++idx) {
    Node *body = bodies->elements[idx];
    switch (body->type) {
      case NT_VARDEFN: {
        Vector *names = body->data.varDefn.names;
        Vector *initializers = body->data.varDefn.initializers;
        for (size_t idx = 0; idx < names->size; ++idx) {
          Node *name = names->elements[idx];
          Node *initializer = initializers->elements[idx];
          if (initializer != NULL) {
            Type const *initializerType =
                typecheckExpression(initializer, entry);
            if (initializerType != NULL &&
                !typeImplicitlyConvertable(
                    initializerType, name->data.id.entry->data.variable.type)) {
              errorNoImplicitConversion(
                  entry, initializer->line, initializer->character,
                  initializerType, name->data.id.entry->data.variable.type);
            }
          }
        }
        break;
      }
      case NT_FUNDEFN: {
        typecheckStmt(
            body->data.funDefn.body,
            body->data.funDefn.name->data.id.entry->data.function.returnType,
            entry);
        break;
      }
      default: {
        break;  // nothing to check
      }
    }
  }
}

int typecheck(void) {
  bool errored = false;

  boolType = keywordTypeCreate(TK_BOOL);

  // for each code file, type check it
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    if (fileList.entries[idx].isCode) typecheckFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;
  }

  typeFree(boolType);

  if (errored) return -1;

  return 0;
}