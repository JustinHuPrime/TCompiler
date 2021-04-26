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
#include "util/internalError.h"

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
 * is the given expression an lvalue
 *
 * @param exp expression to check
 */
static bool isLvalue(Node const *exp) {
  switch (exp->type) {
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_SEQ: {
          return isLvalue(exp->data.binOpExp.rhs);
        }
        case BO_ASSIGN:
        case BO_MULASSIGN:
        case BO_DIVASSIGN:
        case BO_MODASSIGN:
        case BO_ADDASSIGN:
        case BO_SUBASSIGN:
        case BO_LSHIFTASSIGN:
        case BO_ARSHIFTASSIGN:
        case BO_LRSHIFTASSIGN:
        case BO_BITANDASSIGN:
        case BO_BITXORASSIGN:
        case BO_BITORASSIGN:
        case BO_LANDASSIGN:
        case BO_LORASSIGN: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case NT_TERNARYEXP: {
      return isLvalue(exp->data.ternaryExp.consequent) &&
             isLvalue(exp->data.ternaryExp.alternative);
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF:
        case UO_PREINC:
        case UO_PREDEC:
        case UO_NEGASSIGN:
        case UO_LNOTASSIGN:
        case UO_BITNOTASSIGN: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case NT_ID:
    case NT_SCOPEDID: {
      return true;
    }
    default: {
      return false;
    }
  }
}

/**
 * typechecks an expression
 *
 * @param exp expression to typecheck
 * @param entry entry containing this expression
 */
static Type const *typecheckExpression(Node *exp, FileListEntry *entry) {
  if (exp == NULL) return NULL;
  switch (exp->type) {
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_SEQ: {
          typecheckExpression(exp->data.binOpExp.lhs, entry);
          return exp->data.binOpExp.type = typeCopy(
                     typecheckExpression(exp->data.binOpExp.rhs, entry));
        }
        case BO_ASSIGN: {
          Type const *to = typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *from = typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (to != NULL && from != NULL &&
              !typeImplicitlyConvertable(from, to))
            errorNoImplicitConversion(entry, exp->line, exp->character, from,
                                      to);
          if (!isLvalue(exp->data.binOpExp.lhs)) {
            fprintf(
                stderr,
                "%s:%zu:%zu: error: cannot assign a value to an non-lvalue\n",
                entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
          } else if (to != NULL && to->kind == TK_QUALIFIED &&
                     to->data.qualified.constQual) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
          }

          return exp->data.binOpExp.type = typeCopy(to);
        }
        case BO_MULASSIGN: {
          return NULL;  // TODO
        }
        case BO_DIVASSIGN: {
          return NULL;  // TODO
        }
        case BO_MODASSIGN: {
          return NULL;  // TODO
        }
        case BO_ADDASSIGN: {
          return NULL;  // TODO
        }
        case BO_SUBASSIGN: {
          return NULL;  // TODO
        }
        case BO_LSHIFTASSIGN: {
          return NULL;  // TODO
        }
        case BO_ARSHIFTASSIGN: {
          return NULL;  // TODO
        }
        case BO_LRSHIFTASSIGN: {
          return NULL;  // TODO
        }
        case BO_BITANDASSIGN: {
          return NULL;  // TODO
        }
        case BO_BITXORASSIGN: {
          return NULL;  // TODO
        }
        case BO_BITORASSIGN: {
          return NULL;  // TODO
        }
        case BO_LANDASSIGN: {
          return NULL;  // TODO
        }
        case BO_LORASSIGN: {
          return NULL;  // TODO
        }
        case BO_LAND:
        case BO_LOR: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL &&
              !typeImplicitlyConvertable(lhsType, boolType)) {
            errorNoImplicitConversion(entry, exp->data.binOpExp.lhs->line,
                                      exp->data.binOpExp.lhs->character,
                                      lhsType, boolType);
          }

          if (rhsType != NULL &&
              !typeImplicitlyConvertable(rhsType, boolType)) {
            errorNoImplicitConversion(entry, exp->data.binOpExp.rhs->line,
                                      exp->data.binOpExp.rhs->character,
                                      rhsType, boolType);
          }

          return exp->data.binOpExp.type = keywordTypeCreate(TK_BOOL);
        }
        case BO_BITAND:
        case BO_BITOR:
        case BO_BITXOR: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          bool bad = false;
          if (lhsType != NULL && !typeIntegral(lhsType)) {
            char *typeString = typeToString(lhsType);
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot perform a bitwise operation on "
                    "a value of type %s\n",
                    entry->inputFilename, exp->line, exp->character,
                    typeString);
            bad = entry->errored = true;
            free(typeString);
          }
          if (rhsType != NULL && !typeIntegral(rhsType)) {
            char *typeString = typeToString(rhsType);
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot perform a bitwise operation on "
                    "a value of type %s\n",
                    entry->inputFilename, exp->line, exp->character,
                    typeString);
            bad = entry->errored = true;
            free(typeString);
          }

          if (lhsType != NULL && rhsType != NULL && !bad) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              char *lhsString = typeToString(lhsType);
              char *rhsString = typeToString(rhsType);
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot perform a bitwise operation "
                      "with a value of type %s and a value of type %s\n",
                      entry->inputFilename, exp->line, exp->character,
                      lhsString, rhsString);
              entry->errored = true;
              free(lhsString);
              free(rhsString);
            }
            return exp->data.binOpExp.type = merged;
          } else {
            return exp->data.binOpExp.type = NULL;
          }
        }
        case BO_EQ:
        case BO_NEQ:
        case BO_LT:
        case BO_GT:
        case BO_LTEQ:
        case BO_GTEQ:
        case BO_SPACESHIP: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          bool bad = false;
          if (lhsType != NULL && !typeComparable(lhsType)) {
            char *typeString = typeToString(lhsType);
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot perform a comparison operation "
                    "on a value of type %s\n",
                    entry->inputFilename, exp->line, exp->character,
                    typeString);
            bad = entry->errored = true;
            free(typeString);
          }
          if (rhsType != NULL && !typeComparable(rhsType)) {
            char *typeString = typeToString(rhsType);
            fprintf(
                stderr,
                "%s:%zu:%zu: error: cannot perform a comparison operation on "
                "a value of type %s\n",
                entry->inputFilename, exp->line, exp->character, typeString);
            bad = entry->errored = true;
            free(typeString);
          }

          if (lhsType != NULL && rhsType != NULL && !bad) {
            Type *merged = comparisonTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              char *lhsString = typeToString(lhsType);
              char *rhsString = typeToString(rhsType);
              fprintf(
                  stderr,
                  "%s:%zu:%zu: error: cannot perform a comparison operation "
                  "with a value of type %s and a value of type %s\n",
                  entry->inputFilename, exp->line, exp->character, lhsString,
                  rhsString);
              entry->errored = true;
              free(lhsString);
              free(rhsString);
            }
            exp->data.binOpExp.comparisonType = merged;
          }

          return exp->data.binOpExp.type = exp->data.binOpExp.op == BO_SPACESHIP
                                               ? keywordTypeCreate(TK_BYTE)
                                               : keywordTypeCreate(TK_BOOL);
        }
        case BO_LSHIFT: {
          return NULL;  // TODO
        }
        case BO_ARSHIFT: {
          return NULL;  // TODO
        }
        case BO_LRSHIFT: {
          return NULL;  // TODO
        }
        case BO_ADD: {
          return NULL;  // TODO
        }
        case BO_SUB: {
          return NULL;  // TODO
        }
        case BO_MUL: {
          return NULL;  // TODO
        }
        case BO_DIV: {
          return NULL;  // TODO
        }
        case BO_MOD: {
          return NULL;  // TODO
        }
        case BO_FIELD: {
          return NULL;  // TODO
        }
        case BO_PTRFIELD: {
          return NULL;  // TODO
        }
        case BO_ARRAY: {
          return NULL;  // TODO
        }
        case BO_CAST: {
          return NULL;  // TODO}
        }
        default: {
          error(__FILE__, __LINE__, "invalid binop encountered");
        }
      }
    }
    case NT_TERNARYEXP: {
      Type const *predicateType =
          typecheckExpression(exp->data.ternaryExp.predicate, entry);
      if (predicateType != NULL &&
          !typeImplicitlyConvertable(predicateType, boolType)) {
        errorNoImplicitConversion(entry, exp->data.ternaryExp.predicate->line,
                                  exp->data.ternaryExp.predicate->character,
                                  predicateType, boolType);
      }

      Type const *consequentType =
          typecheckExpression(exp->data.ternaryExp.consequent, entry);
      Type const *alternativeType =
          typecheckExpression(exp->data.ternaryExp.alternative, entry);

      Type *merged = ternaryTypeMerge(consequentType, alternativeType);
      if (consequentType != NULL && alternativeType != NULL && merged == NULL) {
        char *consequentString = typeToString(consequentType);
        char *alternativeString = typeToString(alternativeType);
        fprintf(stderr,
                "%s:%zu:%zu: error: type mismatch in ternary expression - "
                "cannot find common type between %s and %s\n",
                entry->inputFilename, exp->line, exp->character,
                consequentString, alternativeString);
        entry->errored = true;
        free(consequentString);
        free(alternativeString);
      }

      return exp->data.binOpExp.type = merged;
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF: {
          return NULL;  // TODO
        }
        case UO_ADDROF: {
          return NULL;  // TODO
        }
        case UO_PREINC: {
          return NULL;  // TODO
        }
        case UO_PREDEC: {
          return NULL;  // TODO
        }
        case UO_NEG: {
          return NULL;  // TODO
        }
        case UO_LNOT: {
          return NULL;  // TODO
        }
        case UO_BITNOT: {
          return NULL;  // TODO
        }
        case UO_POSTINC: {
          return NULL;  // TODO
        }
        case UO_POSTDEC: {
          return NULL;  // TODO
        }
        case UO_NEGASSIGN: {
          return NULL;  // TODO
        }
        case UO_LNOTASSIGN: {
          return NULL;  // TODO
        }
        case UO_BITNOTASSIGN: {
          return NULL;  // TODO
        }
        case UO_SIZEOFEXP: {
          return NULL;  // TODO
        }
        case UO_SIZEOFTYPE: {
          return NULL;  // TODO
        }
        case UO_PARENS: {
          return NULL;  // TODO
        }
        default: {
          error(__FILE__, __LINE__, "invalid unop encountered");
        }
      }
    }
    case NT_FUNCALLEXP: {
      return NULL;  // TODO
    }
    case NT_LITERAL: {
      switch (exp->data.literal.literalType) {
        case LT_UBYTE: {
          return NULL;  // TODO
        }
        case LT_BYTE: {
          return NULL;  // TODO
        }
        case LT_USHORT: {
          return NULL;  // TODO
        }
        case LT_SHORT: {
          return NULL;  // TODO
        }
        case LT_UINT: {
          return NULL;  // TODO
        }
        case LT_INT: {
          return NULL;  // TODO
        }
        case LT_ULONG: {
          return NULL;  // TODO
        }
        case LT_LONG: {
          return NULL;  // TODO
        }
        case LT_FLOAT: {
          return NULL;  // TODO
        }
        case LT_DOUBLE: {
          return NULL;  // TODO
        }
        case LT_STRING: {
          return NULL;  // TODO
        }
        case LT_CHAR: {
          return NULL;  // TODO
        }
        case LT_WSTRING: {
          return NULL;  // TODO
        }
        case LT_WCHAR: {
          return NULL;  // TODO
        }
        case LT_BOOL: {
          return NULL;  // TODO
        }
        case LT_NULL: {
          return NULL;  // TODO
        }
        case LT_AGGREGATEINIT: {
          return NULL;  // TODO
        }
        default: {
          error(__FILE__, __LINE__, "invalid literal type encountered");
        }
      }
    }
    case NT_SCOPEDID: {
      return exp->data.scopedId.type =
                 typeCopy(exp->data.scopedId.entry->data.variable.type);
    }
    case NT_ID: {
      return exp->data.id.type =
                 typeCopy(exp->data.id.entry->data.variable.type);
    }
    default: {
      // not an expression
      error(__FILE__, __LINE__, "invalid expression encountered");
    }
  }
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