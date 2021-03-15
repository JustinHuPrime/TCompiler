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

#include "ast/typeManip.h"
#include "fileList.h"
#include "internalError.h"

/**
 * produce true of expression has an address
 *
 * @param exp expression to check
 * @returns if expression is an lvalue
 */
static bool expressionIsLvalue(Node *exp) {
  switch (exp->type) {
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_PTRFIELD: {
          return true;
        }
        case BO_ARRAY:
        case BO_FIELD: {
          return expressionIsLvalue(exp->data.binOpExp.lhs);
        }
        default: {
          return false;
        }
      }
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF: {
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

static Type const *typecheckExpression(FileListEntry *, Node *);

/**
 * type check a plain binary operation
 * @param expression expression to typecheck
 * @param lhsReq predicate for lhs validity
 * @param lhsReqName human-readable name of the lhs predicate
 * @param rhsReq predicate for rhs validity
 * @param rhsReqName human-readable name of the rhs predicate
 * @param opName human-readable name of the operation
 * @returns resulting type
 */
static Type const *typecheckPlainBinOp(FileListEntry *entry, Node *exp,
                                       bool (*lhsReq)(Type const *),
                                       char const *lhsReqName,
                                       bool (*rhsReq)(Type const *),
                                       char const *rhsReqName,
                                       char const *opName) {
  Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
  Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

  bool bad = false;
  if (lhs != NULL && !lhsReq(lhs)) {
    fprintf(
        stderr,
        "%s:%zu:%zu: error: attempted to apply %s operator on non-%s value\n",
        entry->inputFilename, exp->data.binOpExp.lhs->line,
        exp->data.binOpExp.lhs->character, opName, lhsReqName);
    bad = true;
  }
  if (rhs != NULL && !rhsReq(rhs)) {
    fprintf(
        stderr,
        "%s:%zu:%zu: error: attempted to apply %s operator on non-%s value\n",
        entry->inputFilename, exp->data.binOpExp.rhs->line,
        exp->data.binOpExp.rhs->character, opName, rhsReqName);
    bad = true;
  }

  if (bad || lhs == NULL || rhs == NULL) {
    entry->errored = true;
    return NULL;
  }

  exp->data.binOpExp.type = typeMerge(lhs, rhs);
  if (exp->data.binOpExp.type == NULL) {
    char *lhsString = typeToString(lhs);
    char *rhsString = typeToString(rhs);
    fprintf(stderr,
            "%s:%zu:%zu: error: cannot apply %s operator to a value of type "
            "'%s' with a value of type '%s'\n",
            entry->inputFilename, exp->line, exp->character, opName, lhsString,
            rhsString);
    free(lhsString);
    free(rhsString);
    entry->errored = true;
    return NULL;
  }

  return exp->data.binOpExp.type;
}

/**
 * type check a plain compound assignment operation
 * @param expression expression to typecheck
 * @param lhsReq predicate for lhs validity
 * @param lhsReqName human-readable name of the lhs predicate
 * @param rhsReq predicate for rhs validity
 * @param rhsReqName human-readable name of the rhs predicate
 * @param opName human-readable name of the operation
 */
static Type *typecheckPlainAssignBinOp(FileListEntry *entry, Node *expression,
                                       bool (*lhsReq)(Type const *),
                                       char const *lhsReqName,
                                       bool (*rhsReq)(Type const *),
                                       char const *rhsReqName,
                                       char const *opName) {
  Type const *lhs = typecheckExpression(entry, expression->data.binOpExp.lhs);
  Type const *rhs = typecheckExpression(entry, expression->data.binOpExp.rhs);

  if (!expressionIsLvalue(expression->data.binOpExp.lhs)) {
    fprintf(stderr,
            "%s:%zu:%zu: error: cannot assign to a non-lvalue expression\n",
            entry->inputFilename, expression->data.binOpExp.lhs->line,
            expression->data.binOpExp.lhs->character);
    entry->errored = true;
    return NULL;
  }

  bool bad = false;
  if (lhs != NULL && !lhsReq(lhs)) {
    fprintf(
        stderr,
        "%s:%zu:%zu: error: attempted to apply %s operator on non-%s value\n",
        entry->inputFilename, expression->data.binOpExp.lhs->line,
        expression->data.binOpExp.lhs->character, opName, lhsReqName);
    bad = true;
  }

  if (rhs != NULL && !rhsReq(rhs)) {
    fprintf(
        stderr,
        "%s:%zu:%zu: error: attempted to apply %s operator on non-%s value\n",
        entry->inputFilename, expression->data.binOpExp.rhs->line,
        expression->data.binOpExp.rhs->character, opName, rhsReqName);
    bad = true;
  }

  if (bad || lhs == NULL || rhs == NULL) {
    entry->errored = true;
    return NULL;
  }

  Type *resultType = typeMerge(lhs, rhs);
  if (resultType == NULL) {
    char *lhsString = typeToString(lhs);
    char *rhsString = typeToString(rhs);
    fprintf(stderr,
            "%s:%zu:%zu: error: cannot apply %s operator to a value of "
            "type '%s' with a value of type '%s'\n",
            entry->inputFilename, expression->line, expression->character,
            opName, lhsString, rhsString);
    free(lhsString);
    free(rhsString);
    entry->errored = true;
    return NULL;
  }

  if (!typeIsAssignable(lhs, resultType)) {
    char *fromString = typeToString(resultType);
    char *toString = typeToString(lhs);
    fprintf(stderr,
            "%s:%zu:%zu: error: cannot assign a value of type '%s' to a "
            "value of type '%s'\n",
            entry->inputFilename, expression->line, expression->character,
            fromString, toString);
    free(fromString);
    free(toString);
    typeFree(resultType);
    entry->errored = true;
    return NULL;
  }

  return expression->data.binOpExp.type = resultType;
}

/**
 * type checks an expression
 *
 * @param entry file containing this expression
 * @param exp expression to type check
 * @returns type of the checked expression
 */
static Type const *typecheckExpression(FileListEntry *entry, Node *exp) {
  if (exp == NULL) return NULL;

  switch (exp->type) {
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_SEQ: {
          typecheckExpression(entry, exp->data.binOpExp.lhs);
          return exp->data.binOpExp.type = typeCopy(
                     typecheckExpression(entry, exp->data.binOpExp.rhs));
        }
        case BO_ASSIGN: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (!expressionIsLvalue(exp->data.binOpExp.lhs)) {
            fprintf(
                stderr,
                "%s:%zu:%zu: error: cannot assign to a non-lvalue expression\n",
                entry->inputFilename, exp->data.binOpExp.lhs->line,
                exp->data.binOpExp.lhs->character);
            entry->errored = true;
            return NULL;
          }

          if (lhs != NULL && !typeIsAssignable(lhs, rhs)) {
            char *fromString = typeToString(rhs);
            char *toString = typeToString(lhs);
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot assign a value of type '%s' to "
                    "a value of type '%s'\n",
                    entry->inputFilename, exp->line, exp->character, fromString,
                    toString);
            entry->errored = true;
            free(toString);
            free(fromString);
          }

          if (lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = typeCopy(lhs);
        }
        case BO_MULASSIGN: {
          return typecheckPlainAssignBinOp(
              entry, exp, typeIsNumeric, "numeric", typeIsNumeric, "numeric",
              "compound multiplication and assignment");
        }
        case BO_DIVASSIGN: {
          return typecheckPlainAssignBinOp(entry, exp, typeIsNumeric, "numeric",
                                           typeIsNumeric, "numeric",
                                           "compound division and assignment");
        }
        case BO_MODASSIGN: {
          return typecheckPlainAssignBinOp(
              entry, exp, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound modulo and assignment");
        }
        case BO_ADDASSIGN:
        case BO_SUBASSIGN: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (!expressionIsLvalue(exp->data.binOpExp.lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to assign to non-lvalue\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
            return NULL;
          }

          if (lhs != NULL || rhs != NULL) {
            entry->errored = true;
            return NULL;
          }

          if (typeIsNumeric(lhs) && typeIsNumeric(rhs)) {
            Type *resultType = typeMerge(lhs, rhs);
            if (resultType == NULL) {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot apply compound %s and "
                      "assignment operator to a value of type '%s' with a "
                      "value of type '%s'\n",
                      entry->inputFilename, exp->line, exp->character,
                      exp->data.binOpExp.op == BO_ADDASSIGN ? "addition"
                                                            : "subtraction",
                      lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              entry->errored = true;
              return NULL;
            } else if (!typeIsAssignable(lhs, resultType)) {
              char *fromString = typeToString(resultType);
              char *toString = typeToString(lhs);
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot assign a value of type '%s' "
                      "to a value of type '%s'\n",
                      entry->inputFilename, exp->line, exp->character,
                      fromString, toString);
              free(fromString);
              free(toString);
              typeFree(resultType);
              entry->errored = true;
              return NULL;
            } else {
              return exp->data.binOpExp.type = resultType;
            }
          } else {
            if (typeIsValuePointer(lhs) && typeIsIntegral(rhs)) {
              return exp->data.binOpExp.type = typeCopy(lhs);
            } else {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              fprintf(
                  stderr,
                  "%s:%zu:%zu: error: cannot apply compound %s and "
                  "assignment "
                  "operator to a value of type '%s' and a value of type '%s'",
                  entry->inputFilename, exp->line, exp->character,
                  exp->data.binOpExp.op == BO_ADDASSIGN ? "addition"
                                                        : "subtraction",
                  lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              entry->errored = true;
              return NULL;
            }
          }
        }
        case BO_LSHIFTASSIGN:
        case BO_LRSHIFTASSIGN: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (!expressionIsLvalue(exp->data.binOpExp.lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to assign to non-lvalue\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
            return NULL;
          }

          bool bad = false;
          if (lhs != NULL && !typeIsIntegral(lhs)) {
            fprintf(
                stderr,
                "%s:%zu:%zu: error: attempted to apply compound %s shift and "
                "assignment operator on non-integral value\n",
                entry->inputFilename, exp->data.binOpExp.lhs->line,
                exp->data.binOpExp.lhs->character,
                exp->data.binOpExp.op == BO_LSHIFT ? "left" : "logical right");
            bad = true;
          }
          if (rhs != NULL && !typeIsUnsignedIntegral(rhs)) {
            fprintf(
                stderr,
                "%s:%zu:%zu: error: attempted to apply compound %s shift and "
                "assignment operator on non-unsigned integral value",
                entry->inputFilename, exp->data.binOpExp.rhs->line,
                exp->data.binOpExp.rhs->character,
                exp->data.binOpExp.op == BO_LSHIFT ? "left" : "logical right");
            bad = true;
          }
          if (bad || lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = typeCopy(lhs);
        }
        case BO_ARSHIFTASSIGN: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (!expressionIsLvalue(exp->data.binOpExp.lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to assign to non-lvalue\n",
                    entry->inputFilename, exp->line, exp->character);
            return NULL;
          }

          bool bad = false;
          if (lhs != NULL && !typeIsSignedIntegral(lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply compound arithmetic "
                    "right shift and assignemt operator on non-signed integral "
                    "value\n",
                    entry->inputFilename, exp->data.binOpExp.lhs->line,
                    exp->data.binOpExp.lhs->character);
            bad = true;
          }
          if (rhs != NULL && !typeIsUnsignedIntegral(rhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply compound arithmetic "
                    "right shift and assignemt operator on non-unsigned "
                    "integral value",
                    entry->inputFilename, exp->data.binOpExp.rhs->line,
                    exp->data.binOpExp.rhs->character);
            bad = true;
          }

          if (bad || lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = typeCopy(lhs);
        }
        case BO_BITANDASSIGN: {
          return typecheckPlainAssignBinOp(
              entry, exp, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound bitwise and and assignment");
        }
        case BO_BITXORASSIGN: {
          return typecheckPlainAssignBinOp(
              entry, exp, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound bitwise exclusive or and assignment");
        }
        case BO_BITORASSIGN: {
          return typecheckPlainAssignBinOp(
              entry, exp, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound bitwise or and assignment");
        }
        case BO_LANDASSIGN:
        case BO_LORASSIGN: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.lhs);

          if (!expressionIsLvalue(exp->data.binOpExp.lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to assign to non-lvalue\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
            return NULL;
          }

          bool bad = false;
          if (lhs != NULL && !typeIsBoolean(lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply a compound logical "
                    "%s and assignment to a non-boolean",
                    entry->inputFilename, exp->data.binOpExp.lhs->line,
                    exp->data.binOpExp.lhs->character,
                    exp->data.binOpExp.op == BO_LANDASSIGN ? "and" : "or");
            bad = true;
          }
          if (rhs != NULL && !typeIsBoolean(rhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply a compound logical "
                    "%s and assignment to a non-boolean\n",
                    entry->inputFilename, exp->data.binOpExp.rhs->line,
                    exp->data.binOpExp.rhs->character,
                    exp->data.binOpExp.op == BO_LANDASSIGN ? "and" : "or");
            bad = true;
          }

          if (bad || lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = typeCopy(lhs);
        }
        case BO_LAND:
        case BO_LOR: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          bool bad = false;
          if (lhs != NULL && !typeIsBoolean(lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply a logical %s to a "
                    "non-boolean\n",
                    entry->inputFilename, exp->data.binOpExp.lhs->line,
                    exp->data.binOpExp.lhs->character,
                    exp->data.binOpExp.op == BO_LAND ? "and" : "or");
            bad = true;
          }
          if (rhs != NULL && !typeIsBoolean(rhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply a logical %s to a "
                    "non-boolean",
                    entry->inputFilename, exp->data.binOpExp.rhs->line,
                    exp->data.binOpExp.rhs->character,
                    exp->data.binOpExp.op == BO_LAND ? "and" : "or");
            bad = true;
          }

          if (bad || lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = keywordTypeCreate(TK_BOOL);
        }
        case BO_BITAND: {
          return typecheckPlainBinOp(entry, exp, typeIsIntegral, "integral",
                                     typeIsIntegral, "integral", "bitwise and");
        }
        case BO_BITOR: {
          return typecheckPlainBinOp(entry, exp, typeIsIntegral, "integral",
                                     typeIsIntegral, "integral", "bitwise or");
        }
        case BO_BITXOR: {
          return typecheckPlainBinOp(entry, exp, typeIsIntegral, "integral",
                                     typeIsIntegral, "integral",
                                     "bitwise exclusive or");
        }
        case BO_EQ:
        case BO_NEQ:
        case BO_LT:
        case BO_GT:
        case BO_LTEQ:
        case BO_GTEQ: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          if (!typeIsComparable(lhs, rhs)) {
            char *lhsString = typeToString(lhs);
            char *rhsString = typeToString(rhs);
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot compare a value of type '%s' "
                    "with a value of type '%s'\n",
                    entry->inputFilename, exp->line, exp->character, lhsString,
                    rhsString);
            free(lhsString);
            free(rhsString);
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = keywordTypeCreate(TK_BOOL);
        }
        case BO_SPACESHIP: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          if (!typeIsComparable(lhs, rhs)) {
            char *lhsString = typeToString(lhs);
            char *rhsString = typeToString(rhs);
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot compare a value of type '%s' "
                    "with a value of type '%s'\n",
                    entry->inputFilename, exp->line, exp->character, lhsString,
                    rhsString);
            free(lhsString);
            free(rhsString);
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = keywordTypeCreate(TK_BYTE);
        }
        case BO_LSHIFT:
        case BO_LRSHIFT: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          bool bad = false;
          if (lhs != NULL && !typeIsIntegral(lhs)) {
            fprintf(
                stderr,
                "%s:%zu:%zu: error: attempted to apply %s shift operator on "
                "non-integral value\n",
                entry->inputFilename, exp->data.binOpExp.lhs->line,
                exp->data.binOpExp.lhs->character,
                exp->data.binOpExp.op == BO_LSHIFT ? "left" : "logical right");
            bad = true;
          }
          if (rhs != NULL && !typeIsUnsignedIntegral(rhs)) {
            fprintf(
                stderr,
                "%s:%zu:%zu: error: attempted to apply %s shift operator on "
                "non-integral value\n",
                entry->inputFilename, exp->data.binOpExp.rhs->line,
                exp->data.binOpExp.rhs->character,
                exp->data.binOpExp.op == BO_LSHIFT ? "left" : "logical right");
            bad = true;
          }

          if (bad || lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = typeCopy(lhs);
        }
        case BO_ARSHIFT: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          bool bad = false;
          if (lhs != NULL && !typeIsSignedIntegral(lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply arithmetic right "
                    "shift operator on non-signed integral value\n",
                    entry->inputFilename, exp->data.binOpExp.lhs->line,
                    exp->data.binOpExp.lhs->character);
            bad = true;
          }
          if (rhs != NULL && !typeIsUnsignedIntegral(rhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to apply arithmetic right "
                    "shift operator on non-integral value\n",
                    entry->inputFilename, exp->data.binOpExp.rhs->line,
                    exp->data.binOpExp.rhs->character);
            bad = true;
          }

          if (bad || lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = typeCopy(lhs);
        }
        case BO_ADD: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          if (typeIsNumeric(lhs) && typeIsNumeric(rhs)) {
            exp->data.binOpExp.type = typeMerge(lhs, rhs);
            if (exp->data.binOpExp.type == NULL) {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot apply addition operator to a "
                      "value of type '%s' with a value of type '%s'\n",
                      entry->inputFilename, exp->line, exp->character,
                      lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              entry->errored = true;
              return NULL;
            } else {
              return exp->data.binOpExp.type;
            }
          } else {
            if (typeIsValuePointer(lhs) && typeIsIntegral(rhs)) {
              return exp->data.binOpExp.type = typeCopy(lhs);
            } else if (typeIsIntegral(lhs) && typeIsValuePointer(rhs)) {
              return exp->data.binOpExp.type = typeCopy(rhs);
            } else {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot apply addition operator to a "
                      "value of type '%s' and a value of type '%s'\n",
                      entry->inputFilename, exp->line, exp->character,
                      lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              return NULL;
            }
          }
        }
        case BO_SUB: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          if (lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          if (typeIsNumeric(lhs) && typeIsNumeric(rhs)) {
            exp->data.binOpExp.type = typeMerge(lhs, rhs);
            if (exp->data.binOpExp.type == NULL) {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot apply subtraction operator to "
                      "a value of type '%s' with a value of type '%s'\n",
                      entry->inputFilename, exp->line, exp->character,
                      lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              entry->errored = true;
              return NULL;
            } else {
              return exp->data.binOpExp.type;
            }
          } else {
            if (typeIsValuePointer(lhs) && typeIsIntegral(rhs)) {
              return exp->data.binOpExp.type = typeCopy(lhs);
            } else {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot apply subtraction operator to "
                      "a value of type '%s' and a value of type '%s'\n",
                      entry->inputFilename, exp->line, exp->character,
                      lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              entry->errored = true;
              return NULL;
            }
          }
        }
        case BO_MUL: {
          return typecheckPlainBinOp(entry, exp, typeIsNumeric, "numeric",
                                     typeIsNumeric, "numeric",
                                     "multiplication");
        }
        case BO_DIV: {
          return typecheckPlainBinOp(entry, exp, typeIsNumeric, "numeric",
                                     typeIsNumeric, "numeric", "division");
        }
        case BO_MOD: {
          return typecheckPlainBinOp(entry, exp, typeIsIntegral, "integral",
                                     typeIsIntegral, "integral", "modulo");
        }
        case BO_FIELD: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          if (lhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          if (!typeIsCompound(lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to access a field or option "
                    "of something that is not a struct and not a union\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
            return NULL;
          }

          SymbolTableEntry *definition = lhs->data.reference.entry;
          Vector *names = definition->kind == SK_STRUCT
                              ? &definition->data.structType.fieldNames
                              : &definition->data.unionType.optionNames;
          Vector *types = definition->kind == SK_STRUCT
                              ? &definition->data.structType.fieldTypes
                              : &definition->data.unionType.optionTypes;
          char const *fieldName = exp->data.binOpExp.rhs->data.id.id;

          size_t fieldIdx;
          bool found = false;
          for (size_t idx = 0; idx < names->size; idx++) {
            if (strcmp(names->elements[idx], fieldName) == 0) {
              found = true;
              fieldIdx = idx;
              break;
            }
          }
          if (!found) {
            fprintf(stderr, "%s:%zu:%zu: error: no such %s\n",
                    entry->inputFilename, exp->line, exp->character,
                    definition->kind == SK_STRUCT ? "field" : "option");
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type = typeCopy(types->elements[fieldIdx]);
        }
        case BO_PTRFIELD: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          if (lhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          if (!typeIsValuePointer(lhs)) {
            fprintf(stderr, "%s:%zu:%zu: error: not a pointer\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
            return NULL;
          }
          Type *dereferenced = typeGetDereferenced(lhs);
          if (!typeIsCompound(dereferenced)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: attempted to access a field of "
                    "something that is not a struct and not a union\n",
                    entry->inputFilename, exp->line, exp->character);
            typeFree(dereferenced);
            entry->errored = true;
            return NULL;
          }

          SymbolTableEntry *definition = dereferenced->data.reference.entry;
          Vector *names = definition->kind == SK_STRUCT
                              ? &definition->data.structType.fieldNames
                              : &definition->data.unionType.optionNames;
          Vector *types = definition->kind == SK_STRUCT
                              ? &definition->data.structType.fieldTypes
                              : &definition->data.unionType.optionTypes;
          char const *fieldName = exp->data.binOpExp.rhs->data.id.id;

          size_t fieldIdx;
          size_t offset = 0;
          bool found = false;
          for (size_t idx = 0; idx < names->size; idx++) {
            if (strcmp(names->elements[idx], fieldName) == 0) {
              found = true;
              fieldIdx = idx;
              break;
            } else {
              offset += typeSizeof(types->elements[idx]);
            }
          }
          if (!found) {
            fprintf(stderr, "%s:%zu:%zu: error: no such %s",
                    entry->inputFilename, exp->line, exp->character,
                    definition->kind == SK_STRUCT ? "field" : "option");
            typeFree(dereferenced);
            return NULL;
          } else {
            exp->data.binOpExp.type =
                typeCopyCV(typeCopy(types->elements[fieldIdx]), dereferenced);
            typeFree(dereferenced);
            return exp->data.binOpExp.type;
          }
        }
        case BO_ARRAY: {
          Type const *lhs = typecheckExpression(entry, exp->data.binOpExp.lhs);
          Type const *rhs = typecheckExpression(entry, exp->data.binOpExp.rhs);

          bool bad = false;

          if (lhs != NULL && !typeIsValuePointer(lhs) && !typeIsArray(lhs)) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: expected an array or a pointer\n",
                    entry->inputFilename, exp->line, exp->character);
            bad = true;
          }

          if (!typeIsIntegral(rhs)) {
            fprintf(
                stderr,
                "%s:%zu:%zu: error: attempted to access a non-integral index\n",
                entry->inputFilename, exp->line, exp->character);
            bad = true;
          }

          if (bad || lhs == NULL || rhs == NULL) {
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type =
                     (typeIsValuePointer(lhs) ? typeGetDereferenced(lhs)
                                              : typeGetArrayElement(lhs));
        }
        case BO_CAST: {
          Type const *targetType =
              typecheckExpression(entry, exp->data.binOpExp.rhs);
          if (targetType == NULL) {
            entry->errored = true;
            return NULL;
          }

          if (!typeCastable(exp->data.binOpExp.type, targetType)) {
            char *toTypeName = typeToString(exp->data.binOpExp.type);
            char *fromTypeName = typeToString(targetType);
            fprintf(
                stderr,
                "%s:%zu:%zu: error: cannot perform a cast from '%s' to '%s'\n",
                entry->inputFilename, exp->line, exp->character, fromTypeName,
                toTypeName);
            free(toTypeName);
            free(fromTypeName);
            entry->errored = true;
            return NULL;
          }

          return exp->data.binOpExp.type;
        }
        default: {
          error(__FILE__, __LINE__, "invalid binop enum encountered");
        }
      }
    }
    case NT_TERNARYEXP: {
      Type const *predicate =
          typecheckExpression(entry, exp->data.ternaryExp.predicate);
      Type const *consequent =
          typecheckExpression(entry, exp->data.ternaryExp.consequent);
      Type const *alternative =
          typecheckExpression(entry, exp->data.ternaryExp.alternative);

      bool bad = false;
      if (predicate != NULL && !typeIsBoolean(predicate)) {
        fprintf(stderr,
                "%s:%zu:%zu: error: condition in a ternary expression must be "
                "a boolean\n",
                entry->inputFilename, exp->data.ternaryExp.predicate->line,
                exp->data.ternaryExp.predicate->character);
        bad = true;
      }

      if (bad || predicate == NULL || consequent == NULL ||
          alternative == NULL) {
        entry->errored = true;
        return NULL;
      }

      exp->data.ternaryExp.type = typeMerge(consequent, alternative);
      if (exp->data.ternaryExp.type == NULL) {
        fprintf(stderr,
                "%s:%zu:%zu: error: conflicting types in branches of ternary "
                "expression\n",
                entry->inputFilename, exp->line, exp->character);
        entry->errored = true;
        return NULL;
      }

      return exp->data.ternaryExp.type;
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF: {
          return NULL;  // TODO
        }
        case UO_ADDROF: {
          return NULL;  // TODO
        }
        case UO_PREINC:
        case UO_PREDEC:
        case UO_POSTINC:
        case UO_POSTDEC: {
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
          return typecheckExpression(entry, exp->data.unOpExp.target);
        }
        default: {
          error(__FILE__, __LINE__, "invalid unopexp enum encountered");
        }
      }
    }
    case NT_FUNCALLEXP: {
      return NULL;  // TODO
    }
    case NT_ID:
    case NT_SCOPEDID: {
      SymbolTableEntry *entry =
          exp->type == NT_ID ? exp->data.id.entry : exp->data.scopedId.entry;
      Type *type;
      switch (entry->kind) {
        case SK_VARIABLE: {
          type = typeCopy(entry->data.variable.type);
          break;
        }
        case SK_FUNCTION: {
          type = funPtrTypeCreate(typeCopy(entry->data.function.returnType));
          for (size_t idx = 0; idx < entry->data.function.argumentTypes.size;
               ++idx)
            vectorInsert(
                &type->data.funPtr.argTypes,
                typeCopy(entry->data.function.argumentTypes.elements[idx]));
          break;
        }
        case SK_ENUMCONST: {
          Node *base = malloc(sizeof(Node));
          base->type = NT_SCOPEDID;
          base->line = exp->line;
          base->character = exp->character;
          base->data.scopedId.entry = entry->data.enumConst.parent;
          base->data.scopedId.type = NULL;
          base->data.scopedId.components->elements =
              exp->data.scopedId.components->elements;
          base->data.scopedId.components->capacity = 0;
          base->data.scopedId.components->size =
              exp->data.scopedId.components->size - 1;
          type = referenceTypeCreate(entry->data.enumConst.parent,
                                     stringifyId(base));
          free(base);
          break;
        }
        default: {
          error(__FILE__, __LINE__,
                "type name given to typecheckExpression - parser should "
                "prevent this from happening");
        }
      }
      if (exp->type == NT_ID) {
        return exp->data.id.type = type;
      } else {
        return exp->data.scopedId.type = type;
      }
    }
    default: {
      error(__FILE__, __LINE__, "non-expression given to typecheckExpression");
    }
  }
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
        if (!typeIsSwitchable(conditionType)) {
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
          fprintf(
              stderr,
              "%s:%zu:%zu: error: may not return a value of type '%s' from a "
              "function returning '%s'\n",
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
                    "'%s' using a value of type '%s'\n",
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