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

#include "fileList.h"
#include "internalError.h"
#include "typechecker/typePredicates.h"

/**
 * type checks an expression
 *
 * @param entry file containing this expression
 * @param exp expression to type check
 * @returns type of the checked expression
 */
static Type const *typecheckExpression(FileListEntry *entry, Node *exp) {
  if (exp == NULL) return NULL;

  return NULL;  // TODO
}

/**
 * type checks a statement
 *
 * @param entry file containing this statement
 * @param thisFunction function stab containing this statement
 * @param stmt statement to type check
 */
static void typecheckStmt(FileListEntry *entry, SymbolTableEntry *thisFunction,
                          Node *stmt) {
  if (stmt == NULL) return;

  switch (stmt->type) {
    case NT_COMPOUNDSTMT: {
      Vector *stmts = stmt->data.compoundStmt.stmts;
      for (size_t idx = 0; idx < stmts->size; ++idx)
        typecheckStmt(entry, thisFunction, stmts->elements[idx]);
      break;
    }
    case NT_IFSTMT: {
      Type const *conditionType =
          typecheckExpression(entry, stmt->data.ifStmt.predicate);
      if (conditionType != NULL) {
        if (!typeIsBoolean(conditionType)) {
          fprintf(stderr,
                  "%s:%zu:%zu: error: condition in an 'if' statement must be a "
                  "boolean\n",
                  entry->inputFilename, stmt->data.ifStmt.predicate->line,
                  stmt->data.ifStmt.predicate->character);
          entry->errored = true;
        }
      }
      typecheckStmt(entry, thisFunction, stmt->data.ifStmt.consequent);
      typecheckStmt(entry, thisFunction, stmt->data.ifStmt.alternative);
      break;
    }
    case NT_WHILESTMT: {
      Type const *conditionType =
          typecheckExpression(entry, stmt->data.whileStmt.condition);
      if (conditionType != NULL) {
        if (!typeIsBoolean(conditionType)) {
          fprintf(stderr,
                  "%s:%zu:%zu: error: condition in a while statement must be a "
                  "boolean\n",
                  entry->inputFilename, stmt->data.whileStmt.condition->line,
                  stmt->data.whileStmt.condition->character);
          entry->errored = true;
        }
      }
      typecheckStmt(entry, thisFunction, stmt->data.whileStmt.body);
      break;
    }
    case NT_DOWHILESTMT: {
      Type const *conditionType =
          typecheckExpression(entry, stmt->data.doWhileStmt.condition);
      if (conditionType != NULL) {
        if (!typeIsBoolean(conditionType)) {
          fprintf(stderr,
                  "%s:%zu:%zu: error: condition in a do-while statement must "
                  "be a boolean\n",
                  entry->inputFilename, stmt->data.doWhileStmt.condition->line,
                  stmt->data.doWhileStmt.condition->character);
          entry->errored = true;
        }
      }
      typecheckStmt(entry, thisFunction, stmt->data.doWhileStmt.body);
      break;
    }
    case NT_FORSTMT: {
      typecheckStmt(entry, thisFunction, stmt->data.forStmt.initializer);
      Type const *conditionType =
          typecheckExpression(entry, stmt->data.forStmt.condition);
      if (conditionType != NULL) {
        if (!typeIsBoolean(conditionType)) {
          fprintf(stderr,
                  "%s:%zu:%zu: error: condition in a for statement must be a "
                  "boolean\n",
                  entry->inputFilename, stmt->data.forStmt.condition->line,
                  stmt->data.forStmt.condition->character);
          entry->errored = true;
        }
      }
      typecheckExpression(entry, stmt->data.forStmt.increment);
      typecheckStmt(entry, thisFunction, stmt->data.forStmt.body);
      break;
    }
    case NT_SWITCHSTMT: {
      Type const *conditionType =
          typecheckExpression(entry, stmt->data.switchStmt.condition);
      if (conditionType != NULL) {
        if (!typeIsIntegral(conditionType)) {
          fprintf(stderr,
                  "%s:%zu:%zu: error: conditition in a switch statement must "
                  "be an integral type\n",
                  entry->inputFilename, stmt->data.switchStmt.condition->line,
                  stmt->data.switchStmt.condition->character);
          entry->errored = true;
        }
      }
      Vector *cases = stmt->data.switchStmt.cases;
      for (size_t idx = 0; idx < cases->size; ++idx) {
        Node *caseNode = cases->elements[idx];
        switch (caseNode->type) {
          case NT_SWITCHCASE: {
            typecheckStmt(entry, thisFunction, caseNode->data.switchCase.body);
            break;
          }
          case NT_SWITCHDEFAULT: {
            typecheckStmt(entry, thisFunction,
                          caseNode->data.switchDefault.body);
            break;
          }
          default: {
            error(__FILE__, __LINE__, "non-switch case in switch's cases");
          }
        }
      }
      break;
    }
    case NT_RETURNSTMT: {
      Type const *valueType =
          typecheckExpression(entry, stmt->data.returnStmt.value);
      if (valueType != NULL) {
        if (!typeIsInitializable(thisFunction->data.function.returnType,
                                 valueType)) {
          char *toString = typeToString(thisFunction->data.function.returnType);
          char *fromString = typeToString(valueType);
          fprintf(stderr,
                  "%s:%zu:%zu: error: may not return a value of type %s from a "
                  "function returning %s\n",
                  entry->inputFilename, stmt->data.returnStmt.value->line,
                  stmt->data.returnStmt.value->line, fromString, toString);
          entry->errored = true;
          free(fromString);
          free(toString);
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
        Type const *initializerType = typecheckExpression(entry, initializer);
        if (initializerType != NULL) {
          if (!typeIsInitializable(name->data.id.entry->data.variable.type,
                                   initializerType)) {
            char *toString =
                typeToString(name->data.id.entry->data.variable.type);
            char *fromString = typeToString(initializerType);
            fprintf(stderr,
                    "%s:%zu:%zu: error: may not initialize a variable of type "
                    "%s using a value of type %s\n",
                    entry->inputFilename, initializer->line, initializer->line,
                    fromString, toString);
            entry->errored = true;
            free(fromString);
            free(toString);
          }
        }
      }
      break;
    }
    case NT_EXPRESSIONSTMT: {
      typecheckExpression(entry, stmt->data.expressionStmt.expression);
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
      case NT_FUNDEFN: {
        typecheckStmt(entry, body->data.funDefn.name->data.id.entry,
                      body->data.funDefn.body);
      }
      default: {
        break;  // nothing to check
      }
    }
  }
}

int typecheck(void) {
  bool errored = false;

  // for each code file, type check it
  for (size_t idx = 0; idx < fileList.size; ++idx) {
    if (fileList.entries[idx].isCode) typecheckFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;
  }

  if (errored) return -1;

  return 0;
}