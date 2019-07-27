// Copyright 2019 Justin Hu and Bronwyn Damm
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

#include "util/functional.h"

#include <stdlib.h>
#include <string.h>

// helpers
static bool expressionIsLvalue(Node *expression) {
  switch (expression->type) {
    case NT_SEQEXP: {
      return expressionIsLvalue(expression->data.seqExp.last);
    }
    case NT_BINOPEXP: {
      switch (expression->data.binOpExp.op) {
        case BO_ASSIGN:
        case BO_MULASSIGN:
        case BO_DIVASSIGN:
        case BO_MODASSIGN:
        case BO_ADDASSIGN:
        case BO_SUBASSIGN:
        case BO_LSHIFTASSIGN:
        case BO_LRSHIFTASSIGN:
        case BO_ARSHIFTASSIGN:
        case BO_BITANDASSIGN:
        case BO_BITXORASSIGN:
        case BO_BITORASSIGN: {
          return true;  // lhs must have been an lvalue to get here
        }
        case BO_ARRAYACCESS: {
          return expressionIsLvalue(expression->data.binOpExp.lhs);
        }
        default: { return false; }
      }
    }
    case NT_UNOPEXP: {
      switch (expression->data.unOpExp.op) {
        case UO_DEREF:
        case UO_PREINC:
        case UO_PREDEC: {
          return true;  // post produces temp value containing original value
        }
        default: { return false; }
      }
    }
    case NT_LANDASSIGNEXP:
    case NT_LORASSIGNEXP:
    case NT_STRUCTPTRACCESSEXP:
    case NT_ID: {
      return true;
    }
    case NT_STRUCTACCESSEXP: {
      return expressionIsLvalue(expression->data.structAccessExp.base);
    }
    default: { return false; }
  }
}

static Type *typecheckExpression(Node *, Report *, Options const *,
                                 char const *, TypeVector const *, bool);
static Type *typecheckPlainBinOp(Node *expression, bool (*lhsReq)(Type const *),
                                 char const *lhsReqName,
                                 bool (*rhsReq)(Type const *),
                                 char const *rhsReqName, char const *opName,
                                 Report *report, Options const *options,
                                 char const *filename) {
  Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                  options, filename, NULL, false);
  Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                  options, filename, NULL, false);
  if (lhs == NULL || rhs == NULL) {
    return NULL;
  }

  bool bad = false;
  if (!lhsReq(lhs)) {
    reportError(report,
                "%s:%zu:%zu: error: attempted to apply %s "
                "operator on non-%s value",
                filename, expression->data.binOpExp.lhs->line,
                expression->data.binOpExp.lhs->character, opName, lhsReqName);
    bad = true;
  }
  if (!rhsReq(rhs)) {
    reportError(report,
                "%s:%zu:%zu: error: attempted to apply %s "
                "operator on non-%s value",
                filename, expression->data.binOpExp.rhs->line,
                expression->data.binOpExp.rhs->character, opName, rhsReqName);
    bad = true;
  }
  if (bad) {
    return NULL;
  }

  expression->data.binOpExp.resultType = typeExpMerge(lhs, rhs);
  if (expression->data.binOpExp.resultType == NULL) {
    char *lhsString = typeToString(lhs);
    char *rhsString = typeToString(rhs);
    reportError(
        report,
        "%s:%zu:%zu: error: cannot apply %s operator to a value of type "
        "'%s' with a value of type '%s'",
        filename, expression->line, expression->character, opName, lhsString,
        rhsString);
    free(lhsString);
    free(rhsString);
    return NULL;
  }

  return expression->data.binOpExp.resultType;
}
static Type *typecheckPlainAssignBinOp(
    Node *expression, bool (*lhsReq)(Type const *), char const *lhsReqName,
    bool (*rhsReq)(Type const *), char const *rhsReqName, char const *opName,
    Report *report, Options const *options, char const *filename) {
  Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                  options, filename, NULL, false);
  Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                  options, filename, NULL, false);
  if (lhs == NULL || rhs == NULL) {
    return NULL;
  }

  if (!expressionIsLvalue(expression->data.binOpExp.lhs)) {
    reportError(report, "%s:%zu:%zu: error: attempted to assign to non-lvalue",
                filename, expression->line, expression->character);
    return NULL;
  }

  bool bad = false;
  if (!lhsReq(lhs)) {
    reportError(report,
                "%s:%zu:%zu: error: attempted to apply %s operator on "
                "non-%s value",
                filename, expression->data.binOpExp.lhs->line,
                expression->data.binOpExp.lhs->character, opName, lhsReqName);
    bad = true;
  }
  if (!rhsReq(rhs)) {
    reportError(report,
                "%s:%zu:%zu: error: attempted to apply %s operator on "
                "non-%s value",
                filename, expression->data.binOpExp.rhs->line,
                expression->data.binOpExp.rhs->character, opName, rhsReqName);
    bad = true;
  }
  if (bad) {
    return NULL;
  }

  Type *resultType = typeExpMerge(lhs, rhs);
  if (resultType == NULL) {
    char *lhsString = typeToString(lhs);
    char *rhsString = typeToString(rhs);
    reportError(report,
                "%s:%zu:%zu: error: cannot apply %s operator to a value of "
                "type '%s' with a value of type '%s'",
                filename, expression->line, expression->character, opName,
                lhsString, rhsString);
    free(lhsString);
    free(rhsString);
    return NULL;
  }

  if (!typeAssignable(lhs, resultType)) {
    char *fromString = typeToString(resultType);
    char *toString = typeToString(lhs);
    reportError(report,
                "%s:%zu:%zu: error: cannot assign a value of type '%s' "
                "to a value of type '%s'",
                filename, expression->line, expression->character, fromString,
                toString);
    free(fromString);
    free(toString);
    typeDestroy(resultType);
    return NULL;
  }
  return expression->data.binOpExp.resultType = resultType;
}

// expressions
static Type *typecheckExpression(Node *expression, Report *report,
                                 Options const *options, char const *filename,
                                 TypeVector const *argTypes, bool directCall) {
  switch (expression->type) {
    case NT_SEQEXP: {
      typecheckExpression(expression->data.seqExp.prefix, report, options,
                          filename, NULL, false);
      return expression->data.seqExp.resultType =
                 typecheckExpression(expression->data.seqExp.last, report,
                                     options, filename, argTypes, false);
    }
    case NT_BINOPEXP: {
      switch (expression->data.binOpExp.op) {
        case BO_ASSIGN: {
          Type *to = typecheckExpression(expression->data.binOpExp.lhs, report,
                                         options, filename, NULL, false);
          if (to == NULL) {
            return NULL;
          }

          Type *from = typecheckExpression(
              expression->data.binOpExp.rhs, report, options, filename,
              typeIsFunctionPointer(to)
                  ? typeGetNonConst(to)->data.functionPtr.argumentTypes
                  : NULL,
              false);
          if (from == NULL) {
            return NULL;
          }
          if (!expressionIsLvalue(expression->data.binOpExp.lhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to assign to non-lvalue",
                        filename, expression->line, expression->character);
            return NULL;
          }
          if (!typeAssignable(to, from)) {
            char *fromString = typeToString(from);
            char *toString = typeToString(to);
            reportError(report,
                        "%s:%zu:%zu: error: cannot assign a value of type '%s' "
                        "to a value of type '%s'",
                        filename, expression->line, expression->character,
                        fromString, toString);
            free(fromString);
            free(toString);
            return NULL;
          }
          return expression->data.binOpExp.resultType = typeCopy(to);
        }
        case BO_MULASSIGN: {
          return typecheckPlainAssignBinOp(
              expression, typeIsNumeric, "numeric", typeIsNumeric, "numeric",
              "compound multiplication and assignment", report, options,
              filename);
        }
        case BO_DIVASSIGN: {
          return typecheckPlainAssignBinOp(
              expression, typeIsNumeric, "numeric", typeIsNumeric, "numeric",
              "compound division and assignment", report, options, filename);
        }
        case BO_MODASSIGN: {
          return typecheckPlainAssignBinOp(
              expression, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound modulo and assignment", report, options,
              filename);
        }
        case BO_ADDASSIGN:
        case BO_SUBASSIGN: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          if (!expressionIsLvalue(expression->data.binOpExp.lhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to assign to non-lvalue",
                        filename, expression->line, expression->character);
            return NULL;
          }

          if (typeIsNumeric(lhs) && typeIsNumeric(rhs)) {
            Type *resultType = typeExpMerge(lhs, rhs);
            if (expression->data.binOpExp.resultType == NULL) {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              reportError(report,
                          "%s:%zu:%zu: error: cannot apply compound %s and "
                          "assignment operator to a "
                          "value of type "
                          "'%s' with a value of type '%s'",
                          filename, expression->line, expression->character,
                          expression->data.binOpExp.op == BO_ADDASSIGN
                              ? "addition"
                              : "subtraction",
                          lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              return NULL;
            } else if (!typeAssignable(lhs, resultType)) {
              char *fromString = typeToString(resultType);
              char *toString = typeToString(lhs);
              reportError(
                  report,
                  "%s:%zu:%zu: error: cannot assign a value of type '%s' "
                  "to a value of type '%s'",
                  filename, expression->line, expression->character, fromString,
                  toString);
              free(fromString);
              free(toString);
              typeDestroy(resultType);
              return NULL;
            } else {
              return expression->data.binOpExp.resultType = resultType;
            }
          } else {
            if (typeIsPointer(lhs) && typeIsIntegral(rhs)) {
              return expression->data.binOpExp.resultType = typeCopy(lhs);
            } else {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              reportError(report,
                          "%s:%zu:%zu: error: cannot apply compound %s and "
                          "assignment operator to a "
                          "value of type '%s' and a value of type '%s'",
                          filename, expression->line, expression->character,
                          expression->data.binOpExp.op == BO_ADDASSIGN
                              ? "addition"
                              : "subtraction",
                          lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              return NULL;
            }
          }
        }
        case BO_LSHIFTASSIGN:
        case BO_LRSHIFTASSIGN: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          if (!expressionIsLvalue(expression->data.binOpExp.lhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to assign to non-lvalue",
                        filename, expression->line, expression->character);
            return NULL;
          }

          bool bad = false;
          if (!typeIsIntegral(lhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to apply compound %s "
                        "shift and assignment operator on non-integral value",
                        filename, expression->data.binOpExp.lhs->line,
                        expression->data.binOpExp.lhs->character,
                        expression->data.binOpExp.op == BO_LSHIFT
                            ? "left"
                            : "logical right");
            bad = true;
          }
          if (!typeIsIntegral(rhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to apply compound %s "
                        "shift and assignment operator on non-integral value",
                        filename, expression->data.binOpExp.rhs->line,
                        expression->data.binOpExp.rhs->character,
                        expression->data.binOpExp.op == BO_LSHIFT
                            ? "left"
                            : "logical right");
            bad = true;
          }
          if (bad) {
            return NULL;
          }
          return expression->data.binOpExp.resultType = typeCopy(lhs);
        }
        case BO_ARSHIFTASSIGN: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          if (!expressionIsLvalue(expression->data.binOpExp.lhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to assign to non-lvalue",
                        filename, expression->line, expression->character);
            return NULL;
          }

          bool bad = false;
          if (!typeIsSignedIntegral(lhs)) {
            reportError(
                report,
                "%s:%zu:%zu: error: attempted to apply compound arithmetic "
                "right shift and assignemt operator on non-signed integral "
                "value",
                filename, expression->data.binOpExp.lhs->line,
                expression->data.binOpExp.lhs->character);
            bad = true;
          }
          if (!typeIsIntegral(rhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to apply compound "
                        "arithmetic right shift and assignemt operator on "
                        "non-integral value",
                        filename, expression->data.binOpExp.rhs->line,
                        expression->data.binOpExp.rhs->character);
            bad = true;
          }
          if (bad) {
            return NULL;
          }
          return expression->data.binOpExp.resultType = typeCopy(lhs);
        }
        case BO_BITANDASSIGN: {
          return typecheckPlainAssignBinOp(
              expression, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound bitwise and and assignment", report,
              options, filename);
        }
        case BO_BITXORASSIGN: {
          return typecheckPlainAssignBinOp(
              expression, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound bitwise exclusive or and assignment",
              report, options, filename);
        }
        case BO_BITORASSIGN: {
          return typecheckPlainAssignBinOp(
              expression, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "compound bitwise or and assignment", report, options,
              filename);
        }
        case BO_BITAND: {
          return typecheckPlainBinOp(expression, typeIsIntegral, "integral",
                                     typeIsIntegral, "integral", "bitwise and",
                                     report, options, filename);
        }
        case BO_BITOR: {
          return typecheckPlainBinOp(expression, typeIsIntegral, "integral",
                                     typeIsIntegral, "integral", "bitwise or",
                                     report, options, filename);
        }
        case BO_BITXOR: {
          return typecheckPlainBinOp(
              expression, typeIsIntegral, "integral", typeIsIntegral,
              "integral", "bitwise exclusive or", report, options, filename);
        }
        case BO_SPACESHIP: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          if (!typeComparable(lhs, rhs)) {
            char *lhsString = typeToString(lhs);
            char *rhsString = typeToString(rhs);
            reportError(
                report,
                "%s:%zu:%zu: error: cannot compare a value of type '%s' "
                "with a value of type '%s'",
                filename, expression->line, expression->character, lhsString,
                rhsString);
            free(lhsString);
            free(rhsString);
            return NULL;
          } else {
            return expression->data.binOpExp.resultType =
                       keywordTypeCreate(K_BYTE);
          }
        }
        case BO_LSHIFT:
        case BO_LRSHIFT: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          bool bad = false;
          if (!typeIsIntegral(lhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to apply %s shift "
                        "operator on non-integral value",
                        filename, expression->data.binOpExp.lhs->line,
                        expression->data.binOpExp.lhs->character,
                        expression->data.binOpExp.op == BO_LSHIFT
                            ? "left"
                            : "logical right");
            bad = true;
          }
          if (!typeIsIntegral(rhs)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to apply %s shift "
                        "operator on non-integral value",
                        filename, expression->data.binOpExp.rhs->line,
                        expression->data.binOpExp.rhs->character,
                        expression->data.binOpExp.op == BO_LSHIFT
                            ? "left"
                            : "logical right");
            bad = true;
          }
          if (bad) {
            return NULL;
          }
          return expression->data.binOpExp.resultType = typeCopy(lhs);
        }
        case BO_ARSHIFT: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          bool bad = false;
          if (!typeIsSignedIntegral(lhs)) {
            reportError(
                report,
                "%s:%zu:%zu: error: attempted to apply arithmetic right shift "
                "operator on non-signed integral value",
                filename, expression->data.binOpExp.lhs->line,
                expression->data.binOpExp.lhs->character);
            bad = true;
          }
          if (!typeIsIntegral(rhs)) {
            reportError(
                report,
                "%s:%zu:%zu: error: attempted to apply arithmetic right shift "
                "operator on non-integral value",
                filename, expression->data.binOpExp.rhs->line,
                expression->data.binOpExp.rhs->character);
            bad = true;
          }
          if (bad) {
            return NULL;
          }
          return expression->data.binOpExp.resultType = typeCopy(lhs);
        }
        case BO_ADD:
        case BO_SUB: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          if (typeIsNumeric(lhs) && typeIsNumeric(rhs)) {
            expression->data.binOpExp.resultType = typeExpMerge(lhs, rhs);
            if (expression->data.binOpExp.resultType == NULL) {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              reportError(report,
                          "%s:%zu:%zu: error: cannot apply %s operator to a "
                          "value of type '%s' with a value of type '%s'",
                          filename, expression->line, expression->character,
                          expression->data.binOpExp.op == BO_ADD
                              ? "addition"
                              : "subtraction",
                          lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              return NULL;
            } else {
              return expression->data.binOpExp.resultType;
            }
          } else {
            if (typeIsPointer(lhs) && typeIsIntegral(rhs)) {
              return expression->data.binOpExp.resultType = typeCopy(lhs);
            } else if (typeIsIntegral(lhs) && typeIsPointer(rhs)) {
              return expression->data.binOpExp.resultType = typeCopy(rhs);
            } else {
              char *lhsString = typeToString(lhs);
              char *rhsString = typeToString(rhs);
              reportError(report,
                          "%s:%zu:%zu: error: cannot apply %s operator to a "
                          "value of type '%s' and a value of type '%s'",
                          filename, expression->line, expression->character,
                          expression->data.binOpExp.op == BO_ADD
                              ? "addition"
                              : "subtraction",
                          lhsString, rhsString);
              free(lhsString);
              free(rhsString);
              return NULL;
            }
          }
        }
        case BO_MUL: {
          return typecheckPlainBinOp(expression, typeIsNumeric, "numeric",
                                     typeIsNumeric, "numeric", "multiplication",
                                     report, options, filename);
        }
        case BO_DIV: {
          return typecheckPlainBinOp(expression, typeIsNumeric, "numeric",
                                     typeIsNumeric, "numeric", "division",
                                     report, options, filename);
        }
        case BO_MOD: {
          return typecheckPlainBinOp(expression, typeIsIntegral, "integral",
                                     typeIsIntegral, "integral", "modulo",
                                     report, options, filename);
        }
        case BO_ARRAYACCESS: {
          Type *lhs = typecheckExpression(expression->data.binOpExp.lhs, report,
                                          options, filename, NULL, false);
          Type *rhs = typecheckExpression(expression->data.binOpExp.rhs, report,
                                          options, filename, NULL, false);
          if (lhs == NULL || rhs == NULL) {
            return NULL;
          }

          bool isPtr = typeIsValuePointer(lhs);
          if (isPtr || typeIsArray(lhs)) {
            if (!typeIsIntegral(rhs)) {
              reportError(
                  report,
                  "%s:%zu:%zu: error: attempted to access a non-integral index",
                  filename, expression->line, expression->character);
              return NULL;
            }
            return isPtr ? (expression->data.binOpExp.resultType =
                                typeGetDereferenced(lhs))
                         : (expression->data.binOpExp.resultType =
                                typeGetArrayElement(lhs));
          } else {
            reportError(report,
                        "%s:%zu:%zu: error: expected an array or a pointer",
                        filename, expression->line, expression->character);
            return NULL;
          }
        }
        default: {
          return NULL;  // invalid enum
        }
      }
    }
    case NT_UNOPEXP: {
      Type *target =
          typecheckExpression(expression->data.unOpExp.target, report, options,
                              filename, NULL, false);
      if (target == NULL) {
        return NULL;
      }
      switch (expression->data.unOpExp.op) {
        case UO_DEREF: {
          // must be pointer or const pointer
          if (!typeIsValuePointer(target)) {
            reportError(
                report,
                "%s:%zu:%zu: error: attempted to dereference a non-pointer",
                filename, expression->line, expression->character);
            return NULL;
          } else if (typeGetNonConst(target->data.modifier.type)->kind ==
                     K_VOID) {
            reportError(
                report,
                "%s:%zu:%zu: error: attempted to dereference a void pointer",
                filename, expression->line, expression->character);
            return NULL;
          } else {
            return expression->data.unOpExp.resultType =
                       typeGetDereferenced(target);
          }
        }
        case UO_ADDROF: {
          if (!expressionIsLvalue(expression->data.unOpExp.target)) {
            reportError(
                report,
                "%s:%zu:%zu: error: cannot take the address of a non-lvalue",
                filename, expression->line, expression->character);
            return NULL;
          } else {
            return expression->data.unOpExp.resultType =
                       modifierTypeCreate(K_PTR, typeCopy(target));
          }
        }
        case UO_PREINC:
        case UO_PREDEC:
        case UO_POSTDEC:
        case UO_POSTINC: {
          if (!expressionIsLvalue(expression->data.unOpExp.target) ||
              target->kind == K_CONST) {
            reportError(
                report,
                "%s:%zu:%zu: error: cannot modify target of %s operator",
                filename, expression->line, expression->character,
                expression->data.unOpExp.op == UO_PREINC ||
                        expression->data.unOpExp.op == UO_POSTINC
                    ? "increment"
                    : "decrement");
            return NULL;
          } else if (!typeIsIntegral(target) && !typeIsPointer(target)) {
            char *typeString = typeToString(target);
            reportError(report,
                        "%s:%zu:%zu: error: cannot %s a value of type '%s'",
                        filename, expression->line, expression->character,
                        expression->data.unOpExp.op == UO_PREINC ||
                                expression->data.unOpExp.op == UO_POSTINC
                            ? "increment"
                            : "decrement",
                        typeString);
            free(typeString);
            return NULL;
          }
          return expression->data.unOpExp.resultType = typeCopy(target);
        }
        case UO_NEG: {
          if (!typeIsSignedIntegral(target) && !typeIsFloat(target)) {
            char *typeString = typeToString(target);
            reportError(
                report, "%s:%zu:%zu: error: cannot negate a value of type '%s'",
                filename, expression->line, expression->character, typeString);
            free(typeString);
            return NULL;
          }
          return expression->data.unOpExp.resultType = typeCopy(target);
        }
        case UO_LNOT: {
          if (!typeIsBoolean(target)) {
            reportError(report,
                        "%s:%zu:%zu: error: attempted to apply a logical not "
                        "to a non-boolean",
                        filename, expression->line, expression->character);
            return NULL;
          }
          return expression->data.unOpExp.resultType = typeCopy(target);
        }
        case UO_BITNOT: {
          if (!typeIsIntegral(target)) {
            char *typeString = typeToString(target);
            reportError(report,
                        "%s:%zu:%zu: error: may not apply a bitwise not to a "
                        "value of type '%s'",
                        filename, expression->line, expression->character,
                        typeString);
            free(typeString);
            return NULL;
          }
          return expression->data.unOpExp.resultType = typeCopy(target);
        }
        default: {
          return NULL;  // invalid enum
        }
      }
    }
    case NT_COMPOPEXP: {
      Type *lhs = typecheckExpression(expression->data.compOpExp.lhs, report,
                                      options, filename, NULL, false);
      Type *rhs = typecheckExpression(expression->data.compOpExp.rhs, report,
                                      options, filename, NULL, false);
      if (lhs == NULL || rhs == NULL) {
        return NULL;
      }

      if (!typeComparable(lhs, rhs)) {
        char *lhsString = typeToString(lhs);
        char *rhsString = typeToString(rhs);
        reportError(report,
                    "%s:%zu:%zu: error: cannot compare a value of type '%s' "
                    "with a value of type '%s'",
                    filename, expression->line, expression->character,
                    lhsString, rhsString);
        free(lhsString);
        free(rhsString);
        return NULL;
      } else {
        return expression->data.compOpExp.resultType =
                   keywordTypeCreate(K_BOOL);
      }
    }
    case NT_LANDASSIGNEXP:
    case NT_LORASSIGNEXP: {
      Node *lhs = expression->type == NT_LANDEXP ? expression->data.landExp.lhs
                                                 : expression->data.lorExp.lhs;
      Node *rhs = expression->type == NT_LANDEXP ? expression->data.landExp.rhs
                                                 : expression->data.lorExp.rhs;
      Type *lhsType =
          typecheckExpression(lhs, report, options, filename, NULL, false);
      Type *rhsType =
          typecheckExpression(rhs, report, options, filename, NULL, false);
      if (lhsType == NULL || rhsType == NULL) {
        return NULL;
      }

      if (!expressionIsLvalue(expression->data.binOpExp.lhs)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to assign to non-lvalue",
                    filename, expression->line, expression->character);
        return NULL;
      }

      bool bad = false;
      if (!typeIsBoolean(lhsType)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to apply a compound logical "
                    "%s and assignment to a non-boolean",
                    filename, lhs->line, lhs->character,
                    expression->type == NT_LANDEXP ? "and" : "or");
        bad = true;
      }
      if (!typeIsBoolean(rhsType)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to apply a compound logical "
                    "%s and assignment to a non-boolean",
                    filename, rhs->line, rhs->character,
                    expression->type == NT_LANDEXP ? "and" : "or");
        bad = true;
      }

      if (bad) {
        return NULL;
      } else {
        return expression->type == NT_LANDEXP
                   ? (expression->data.landExp.resultType = typeCopy(lhsType))
                   : (expression->data.lorExp.resultType = typeCopy(lhsType));
      }
    }
    case NT_TERNARYEXP: {
      Type *condition =
          typecheckExpression(expression->data.ternaryExp.condition, report,
                              options, filename, NULL, false);
      Type *thenExp =
          typecheckExpression(expression->data.ternaryExp.thenExp, report,
                              options, filename, argTypes, false);
      Type *elseExp =
          typecheckExpression(expression->data.ternaryExp.elseExp, report,
                              options, filename, argTypes, false);
      if (condition == NULL || thenExp == NULL || elseExp == NULL) {
        return NULL;
      }
      bool bad = false;
      if (!typeIsBoolean(condition)) {
        reportError(report,
                    "%s:%zu:%zu: error: condition in a ternary expression must "
                    "be assignable to a boolean value",
                    filename, expression->data.ternaryExp.condition->line,
                    expression->data.ternaryExp.condition->character);
        bad = true;
      }

      expression->data.ternaryExp.resultType = typeExpMerge(thenExp, elseExp);
      if (expression->data.ternaryExp.resultType == NULL) {
        reportError(report,
                    "%s:%zu:%zu: error: conflicting types in branches of "
                    "ternary expression",
                    filename, expression->line, expression->character);
        bad = true;
      }

      if (bad) {
        return NULL;
      }
      return expression->data.ternaryExp.resultType;
    }
    case NT_LANDEXP:
    case NT_LOREXP: {
      Node *lhs = expression->type == NT_LANDEXP ? expression->data.landExp.lhs
                                                 : expression->data.lorExp.lhs;
      Node *rhs = expression->type == NT_LANDEXP ? expression->data.landExp.rhs
                                                 : expression->data.lorExp.rhs;
      Type *lhsType =
          typecheckExpression(lhs, report, options, filename, NULL, false);
      Type *rhsType =
          typecheckExpression(rhs, report, options, filename, NULL, false);
      if (lhsType == NULL || rhsType == NULL) {
        return NULL;
      }
      bool bad = false;
      if (!typeIsBoolean(lhsType)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to apply a logical %s to a "
                    "non-boolean",
                    filename, lhs->line, lhs->character,
                    expression->type == NT_LANDEXP ? "and" : "or");
        bad = true;
      }
      if (!typeIsBoolean(rhsType)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to apply a logical %s to a "
                    "non-boolean",
                    filename, rhs->line, rhs->character,
                    expression->type == NT_LANDEXP ? "and" : "or");
        bad = true;
      }

      if (bad) {
        return NULL;
      } else {
        return expression->type == NT_LANDEXP
                   ? (expression->data.landExp.resultType =
                          keywordTypeCreate(K_BOOL))
                   : (expression->data.lorExp.resultType =
                          keywordTypeCreate(K_BOOL));
      }
    }
    case NT_STRUCTACCESSEXP: {
      Type *lhs = typecheckExpression(expression->data.structAccessExp.base,
                                      report, options, filename, NULL, false);
      if (lhs == NULL) {
        return NULL;
      }

      if (!typeIsCompound(lhs)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to access a field of "
                    "something that is not a struct and not a union",
                    filename, expression->line, expression->character);
        typeDestroy(lhs);
        return NULL;
      }

      SymbolInfo *definition = lhs->data.reference.referenced;
      StringVector *names = definition->data.type.kind == TDK_STRUCT
                                ? &definition->data.type.data.structType.names
                                : &definition->data.type.data.unionType.names;
      TypeVector *types = definition->data.type.kind == TDK_STRUCT
                              ? &definition->data.type.data.structType.fields
                              : &definition->data.type.data.unionType.fields;
      char const *fieldName =
          expression->data.structAccessExp.element->data.id.id;

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
        reportError(report, "%s:%zu:%zu: error: no such field", filename,
                    expression->line, expression->character);
        return NULL;
      } else {
        return expression->data.structAccessExp.resultType =
                   typeCopy(types->elements[fieldIdx]);
      }
    }
    case NT_STRUCTPTRACCESSEXP: {
      Type *lhs = typecheckExpression(expression->data.structPtrAccessExp.base,
                                      report, options, filename, NULL, false);
      if (lhs == NULL) {
        return NULL;
      }

      if (!typeIsValuePointer(lhs)) {
        reportError(report, "%s:%zu:%zu: error: not a pointer", filename,
                    expression->line, expression->character);
        return NULL;
      }
      Type *dereferenced = typeGetDereferenced(lhs);
      if (!typeIsCompound(dereferenced)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to access a field of "
                    "something that is not a struct and not a union",
                    filename, expression->line, expression->character);
        typeDestroy(dereferenced);
        return NULL;
      }

      SymbolInfo *definition =
          lhs->data.modifier.type->data.reference.referenced;
      StringVector *names = definition->data.type.kind == TDK_STRUCT
                                ? &definition->data.type.data.structType.names
                                : &definition->data.type.data.unionType.names;
      TypeVector *types = definition->data.type.kind == TDK_STRUCT
                              ? &definition->data.type.data.structType.fields
                              : &definition->data.type.data.unionType.fields;
      char const *fieldName =
          expression->data.structPtrAccessExp.element->data.id.id;

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
        reportError(report, "%s:%zu:%zu: error: no such field", filename,
                    expression->line, expression->character);
        typeDestroy(dereferenced);
        return NULL;
      } else {
        if (dereferenced->kind == K_CONST) {
          expression->data.structPtrAccessExp.resultType =
              modifierTypeCreate(K_CONST, typeCopy(types->elements[fieldIdx]));
        } else {
          expression->data.structPtrAccessExp.resultType =
              typeCopy(types->elements[fieldIdx]);
        }
        typeDestroy(dereferenced);
        return expression->data.structPtrAccessExp.resultType;
      }
    }
    case NT_FNCALLEXP: {
      NodeList *args = expression->data.fnCallExp.args;
      TypeVector *actualArgTypes = typeVectorCreate();
      for (size_t idx = 0; idx < args->size; idx++) {
        Type *argType = typecheckExpression(args->elements[idx], report,
                                            options, filename, NULL, false);
        if (argType == NULL && actualArgTypes != NULL) {
          typeVectorDestroy(actualArgTypes);
          actualArgTypes = NULL;
        }
        if (actualArgTypes != NULL) {
          typeVectorInsert(actualArgTypes, typeCopy(argType));
        }
      }

      if (actualArgTypes == NULL) {
        return NULL;  // bad args - can't proceed further
      }

      Type *fnType =
          typecheckExpression(expression->data.fnCallExp.who, report, options,
                              filename, actualArgTypes, true);
      typeVectorDestroy(actualArgTypes);
      if (fnType == NULL) {
        return NULL;  // no valid function
      } else if (!typeIsFunctionPointer(fnType)) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to make a function call on a "
                    "non-function",
                    filename, expression->line, expression->character);
        return NULL;
      }

      return expression->data.fnCallExp.resultType =
                 typeCopy(fnType->data.functionPtr.returnType);
    }
    case NT_CONSTEXP: {
      switch (expression->data.constExp.type) {
        case CT_UBYTE:
        case CT_BYTE:
        case CT_CHAR:
        case CT_USHORT:
        case CT_SHORT:
        case CT_UINT:
        case CT_INT:
        case CT_WCHAR:
        case CT_ULONG:
        case CT_LONG:
        case CT_FLOAT:
        case CT_DOUBLE:
        case CT_BOOL: {
          return expression->data.constExp.resultType = keywordTypeCreate(
                     expression->data.constExp.type - CT_UBYTE + K_BYTE);
        }
        case CT_STRING: {
          return expression->data.constExp.resultType = modifierTypeCreate(
                     K_PTR,
                     modifierTypeCreate(K_CONST, keywordTypeCreate(K_CHAR)));
        }
        case CT_WSTRING: {
          return expression->data.constExp.resultType = modifierTypeCreate(
                     K_PTR,
                     modifierTypeCreate(K_CONST, keywordTypeCreate(K_WCHAR)));
        }
        case CT_NULL: {
          return expression->data.constExp.resultType =
                     modifierTypeCreate(K_PTR, keywordTypeCreate(K_VOID));
        }
        case CT_RANGE_ERROR: {
          reportError(report,
                      "%s:%zu:%zu: error: constant out of representable range",
                      filename, expression->line, expression->character);
          return NULL;
        }
        default: {
          return NULL;  // invalid enum
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      NodeList *elements = expression->data.aggregateInitExp.elements;
      TypeVector *elementTypes = typeVectorCreate();

      for (size_t idx = 0; idx < elements->size; idx++) {
        Node *element = elements->elements[idx];
        Type *elementType = typecheckExpression(element, report, options,
                                                filename, NULL, false);
        if (elementType == NULL && elementTypes != NULL) {
          typeVectorDestroy(elementTypes);
          elementTypes = NULL;
        } else if (elementTypes != NULL) {
          typeVectorInsert(elementTypes, typeCopy(elementType));
        }
      }

      if (elementTypes != NULL) {
        return expression->data.aggregateInitExp.resultType =
                   aggregateInitTypeCreate(elementTypes);
      } else {
        return NULL;
      }
    }
    case NT_CASTEXP: {
      Type *targetType =
          typecheckExpression(expression->data.castExp.target, report, options,
                              filename, NULL, false);
      if (targetType == NULL) {
        return NULL;
      }
      if (!typeCastable(expression->data.castExp.resultType, targetType)) {
        char *toTypeName = typeToString(expression->data.castExp.resultType);
        char *fromTypeName = typeToString(targetType);
        reportError(
            report,
            "%s:%zu:%zu: error: cannot perform a cast from '%s' to '%s'",
            filename, expression->line, expression->character, fromTypeName,
            toTypeName);
        free(toTypeName);
        free(fromTypeName);
        return NULL;
      } else {
        return expression->data.castExp.resultType;
      }
    }
    case NT_SIZEOFTYPEEXP: {
      return expression->data.sizeofTypeExp.resultType =
                 keywordTypeCreate(TK_ULONG);
    }
    case NT_SIZEOFEXPEXP: {
      typecheckExpression(expression->data.sizeofExpExp.target, report, options,
                          filename, NULL, false);
      return expression->data.sizeofExpExp.resultType =
                 keywordTypeCreate(TK_ULONG);
    }
    case NT_ID: {
      SymbolInfo *info = expression->data.id.symbol;
      if (info->kind == SK_VAR) {
        return expression->data.id.resultType = typeCopy(info->data.var.type);
      } else if (info->kind == SK_FUNCTION && argTypes == NULL) {
        // ambiguous overload, maybe?
        OverloadSet *overloadSet = &info->data.function.overloadSet;
        if (overloadSet->size == 1) {
          expression->data.id.overload = overloadSet->elements[0];
          return expression->data.id.resultType = modifierTypeCreate(
                     K_CONST,
                     functionPtrTypeCreate(
                         typeCopy(expression->data.id.overload->returnType),
                         typeVectorCopy(
                             &expression->data.id.overload->argumentTypes)));
        } else {
          reportError(report,
                      "%s:%zu:%zu: error: cannot determine appropriate "
                      "overload in set to use",
                      filename, expression->line, expression->character);
          return NULL;
        }
      } else {
        OverloadSet *overloadSet = &info->data.function.overloadSet;
        if (directCall) {
          Vector *candidateCalls = overloadSetLookupCall(overloadSet, argTypes);
          if (candidateCalls->size > 1) {
            reportError(report,
                        "%s:%zu:%zu: error: function call has multiple "
                        "overload candidates",
                        filename, expression->line, expression->character);
            vectorDestroy(candidateCalls, nullDtor);
            return NULL;
          } else if (candidateCalls->size == 0) {
            reportError(report,
                        "%s:%zu:%zu: error: no function in overload set "
                        "matches given arguments",
                        filename, expression->line, expression->character);
            return NULL;
          } else {
            expression->data.id.overload = candidateCalls->elements[0];
            return expression->data.id.resultType = modifierTypeCreate(
                       K_CONST,
                       functionPtrTypeCreate(
                           typeCopy(expression->data.id.overload->returnType),
                           typeVectorCopy(
                               &expression->data.id.overload->argumentTypes)));
          }
        } else {
          expression->data.id.overload =
              overloadSetLookupDefinition(overloadSet, argTypes);
          if (expression->data.id.overload == NULL) {
            reportError(report,
                        "%s:%zu:%zu: error: no function in overload set "
                        "matches given arguments",
                        filename, expression->line, expression->character);
            return NULL;
          } else {
            return expression->data.id.resultType = modifierTypeCreate(
                       K_CONST,
                       functionPtrTypeCreate(
                           typeCopy(expression->data.id.overload->returnType),
                           typeVectorCopy(
                               &expression->data.id.overload->argumentTypes)));
          }
        }
      }
    }
    default: {
      return NULL;  // not well formed, parse-wise
    }
  }
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
        Type *conditionType =
            typecheckExpression(statement->data.ifStmt.condition, report,
                                options, filename, NULL, false);
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
        Type *conditionType =
            typecheckExpression(statement->data.whileStmt.condition, report,
                                options, filename, NULL, false);
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
        Type *conditionType =
            typecheckExpression(statement->data.doWhileStmt.condition, report,
                                options, filename, NULL, false);
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
            Type *exprType =
                typecheckExpression(statement->data.forStmt.initialize, report,
                                    options, filename, NULL, false);
            if (exprType != NULL) {
              typeDestroy(exprType);
            }
          }
        }
        Type *conditionType =
            typecheckExpression(statement->data.forStmt.condition, report,
                                options, filename, NULL, false);
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
          Type *exprType =
              typecheckExpression(statement->data.forStmt.update, report,
                                  options, filename, NULL, false);
          if (exprType != NULL) {
            typeDestroy(exprType);
          }
        }
        typecheckStmt(statement->data.forStmt.body, report, options, filename,
                      expectedReturnType);
        break;
      }
      case NT_SWITCHSTMT: {
        Type *switchedOnType =
            typecheckExpression(statement->data.switchStmt.onWhat, report,
                                options, filename, NULL, false);
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
          Type *returnType =
              typecheckExpression(statement->data.returnStmt.value, report,
                                  options, filename, NULL, false);
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
                                report, options, filename, NULL, false);
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
      Type *paramType = typecheckExpression(defaultArg, report, options,
                                            filename, NULL, false);
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
      Type *paramType = typecheckExpression(defaultArg, report, options,
                                            filename, NULL, false);
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
      Type *initType = typecheckExpression(initValue, report, options, filename,
                                           NULL, false);
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