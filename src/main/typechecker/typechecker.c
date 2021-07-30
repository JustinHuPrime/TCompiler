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
 * complaints about being unable to perform a binop
 */
static void errorNoBinOp(FileListEntry *entry, size_t line, size_t character,
                         char const *op, Type const *lhsType,
                         Type const *rhsType) {
  char *lhsString = typeToString(lhsType);
  char *rhsString = typeToString(rhsType);
  fprintf(stderr,
          "%s:%zu:%zu: error: cannot perform %s on a value of type '%s' and a "
          "value of type '%s'\n",
          entry->inputFilename, line, character, op, lhsString, rhsString);
  entry->errored = true;
  free(lhsString);
  free(rhsString);
}
/**
 * complaints about being unable to perform an unop
 */
static void errorNoUnOp(FileListEntry *entry, size_t line, size_t character,
                        char const *op, Type const *target) {
  char *typeString = typeToString(target);
  fprintf(stderr,
          "%s:%zu:%zu: error: cannot perform %s on a value of type '%s'\n",
          entry->inputFilename, line, character, op, typeString);
  entry->errored = true;
  free(typeString);
}
/**
 * complains about there being no member with the given name
 */
static void errorNoMember(FileListEntry *entry, size_t line, size_t character,
                          char const *member, Type const *type) {
  char *typeString = typeToString(type);
  fprintf(stderr,
          "%s:%zu:%zu: error: no member named '%s' on a value of "
          "type '%s'\n",
          entry->inputFilename, line, character, member, typeString);
  entry->errored = true;
  free(typeString);
}
/**
 * complains about there being no members
 */
static void errorNoMembers(FileListEntry *entry, size_t line, size_t character,
                           Type const *type) {
  char *typeString = typeToString(type);
  fprintf(stderr,
          "%s:%zu:%zu: error: cannot access members on a value of "
          "type '%s'\n",
          entry->inputFilename, line, character, typeString);
  entry->errored = true;
  free(typeString);
}
/**
 * complains about expression not being an lvalue
 */
static void errorNotLvalue(FileListEntry *entry, size_t line, size_t character,
                           char const *op) {
  fprintf(stderr, "%s:%zu:%zu: error: cannot %s a non-lvalue\n",
          entry->inputFilename, line, character, op);
  entry->errored = true;
}
/**
 * complains about a type being incomplete
 */
static void errorIncompleteType(FileListEntry *entry, size_t line,
                                size_t character, Type const *t) {
  char *typeString = typeToString(t);
  fprintf(stderr,
          "%s:%zu:%zu: error: values of type '%s' do not exist; the type is "
          "incomplete\n",
          entry->inputFilename, line, character, typeString);
  free(typeString);
  entry->errored = true;
}
/**
 * complains about a type being recursive
 */
static void errorRecursiveDecl(FileListEntry *entry, size_t line,
                               size_t character, char const *what,
                               char const *name) {
  fprintf(stderr, "%s:%zu:%zu: error: the %s '%s' may not contain itself\n",
          entry->inputFilename, line, character, what, name);
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
        case BO_LORASSIGN:
        case BO_PTRFIELD: {
          return true;
        }
        case BO_FIELD:
        case BO_ARRAY: {
          return isLvalue(exp->data.binOpExp.lhs);
        }
        default: {
          return false;
        }
      }
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF:
        case UO_PREINC:
        case UO_PREDEC: {
          return true;
        }
        case UO_PARENS: {
          return isLvalue(exp->data.unOpExp.target);
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
 * marks the given expression as needing to be stored in memory
 */
static void markEscapes(Node *exp) {
  switch (exp->type) {
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_SEQ: {
          markEscapes(exp->data.binOpExp.rhs);
          break;
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
          markEscapes(exp->data.binOpExp.lhs);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "tried to mark a non-lvalue as escaping");
        }
      }
      break;
    }
    case NT_TERNARYEXP: {
      markEscapes(exp->data.ternaryExp.consequent);
      markEscapes(exp->data.ternaryExp.alternative);
      break;
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF:
        case UO_PREINC:
        case UO_PREDEC: {
          markEscapes(exp->data.unOpExp.target);
          break;
        }
        default: {
          error(__FILE__, __LINE__, "tried to mark a non-lvalue as escaping");
        }
      }
      break;
    }
    case NT_ID: {
      exp->data.id.entry->data.variable.escapes = true;
      break;
    }
    case NT_SCOPEDID: {
      exp->data.scopedId.entry->data.variable.escapes = true;
      break;
    }
    default: {
      error(__FILE__, __LINE__, "tried to mark a non-lvalue as escaping");
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
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL &&
              !typeImplicitlyConvertable(rhsType, lhsType))
            errorNoImplicitConversion(entry, exp->line, exp->character, rhsType,
                                      lhsType);

          if (!isLvalue(exp->data.binOpExp.lhs)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "assign a value to");
          } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                     lhsType->data.qualified.constQual) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_MULASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a multiplication operation", lhsType, rhsType);
            }

            if (lhsType != NULL && merged != NULL &&
                !typeImplicitlyConvertable(merged, lhsType))
              errorNoImplicitConversion(entry, exp->line, exp->character,
                                        merged, lhsType);
            typeFree(merged);

            if (!isLvalue(exp->data.binOpExp.lhs)) {
              errorNotLvalue(entry, exp->line, exp->character,
                             "assign a value to");
            } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                       lhsType->data.qualified.constQual) {
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot assign a value to a constant "
                      "variable\n",
                      entry->inputFilename, exp->line, exp->character);
              entry->errored = true;
            }
          }
          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_DIVASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a division operation", lhsType, rhsType);
            }

            if (lhsType != NULL && merged != NULL &&
                !typeImplicitlyConvertable(merged, lhsType))
              errorNoImplicitConversion(entry, exp->line, exp->character,
                                        merged, lhsType);
            typeFree(merged);

            if (!isLvalue(exp->data.binOpExp.lhs)) {
              errorNotLvalue(entry, exp->line, exp->character,
                             "assign a value to");
            } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                       lhsType->data.qualified.constQual) {
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot assign a value to a constant "
                      "variable\n",
                      entry->inputFilename, exp->line, exp->character);
              entry->errored = true;
            }
          }
          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_MODASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a modulo operation", lhsType, rhsType);
            }

            if (lhsType != NULL && merged != NULL &&
                !typeImplicitlyConvertable(merged, lhsType))
              errorNoImplicitConversion(entry, exp->line, exp->character,
                                        merged, lhsType);
            typeFree(merged);

            if (!isLvalue(exp->data.binOpExp.lhs)) {
              errorNotLvalue(entry, exp->line, exp->character,
                             "assign a value to");
            } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                       lhsType->data.qualified.constQual) {
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot assign a value to a constant "
                      "variable\n",
                      entry->inputFilename, exp->line, exp->character);
              entry->errored = true;
            }
          }
          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_ADDASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            if (typeNumeric(lhsType) && typeNumeric(rhsType)) {
              Type *merged = arithmeticTypeMerge(lhsType, rhsType);
              if (merged == NULL) {
                errorNoBinOp(entry, exp->line, exp->character,
                             "an addition operation", lhsType, rhsType);
              }

              if (lhsType != NULL && merged != NULL &&
                  !typeImplicitlyConvertable(merged, lhsType))
                errorNoImplicitConversion(entry, exp->line, exp->character,
                                          merged, lhsType);
              typeFree(merged);

              if (!isLvalue(exp->data.binOpExp.lhs)) {
                errorNotLvalue(entry, exp->line, exp->character,
                               "assign a value to");
              } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                         lhsType->data.qualified.constQual) {
                fprintf(
                    stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
                entry->errored = true;
              }
            } else if (typePointer(lhsType) && typeIntegral(rhsType)) {
              if (!isLvalue(exp->data.binOpExp.lhs)) {
                errorNotLvalue(entry, exp->line, exp->character,
                               "assign a value to");
              } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                         lhsType->data.qualified.constQual) {
                fprintf(
                    stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
                entry->errored = true;
              }
            } else {
              errorNoBinOp(entry, exp->line, exp->character,
                           "an additon operation", lhsType, rhsType);
            }
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_SUBASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            if (typeNumeric(lhsType) && typeNumeric(rhsType)) {
              Type *merged = arithmeticTypeMerge(lhsType, rhsType);
              if (merged == NULL) {
                errorNoBinOp(entry, exp->line, exp->character,
                             "a subtraction operation", lhsType, rhsType);
              }

              if (lhsType != NULL && merged != NULL &&
                  !typeImplicitlyConvertable(merged, lhsType))
                errorNoImplicitConversion(entry, exp->line, exp->character,
                                          merged, lhsType);
              typeFree(merged);

              if (!isLvalue(exp->data.binOpExp.lhs)) {
                errorNotLvalue(entry, exp->line, exp->character,
                               "assign a value to");
              } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                         lhsType->data.qualified.constQual) {
                fprintf(
                    stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
                entry->errored = true;
              }
            } else if (typePointer(lhsType) && typeIntegral(rhsType)) {
              if (!isLvalue(exp->data.binOpExp.lhs)) {
                errorNotLvalue(entry, exp->line, exp->character,
                               "assign a value to");
              } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                         lhsType->data.qualified.constQual) {
                fprintf(
                    stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
                entry->errored = true;
              }
            } else {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a subtraction operation", lhsType, rhsType);
            }
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_LSHIFTASSIGN:
        case BO_LRSHIFTASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL &&
              ((!typeIntegral(lhsType) && !typePointer(lhsType)) ||
               !typeUnsignedIntegral(rhsType))) {
            errorNoBinOp(entry, exp->line, exp->character, "a shift operation",
                         lhsType, rhsType);
          }

          if (!isLvalue(exp->data.binOpExp.lhs)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "assign a value to");
          } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                     lhsType->data.qualified.constQual) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_ARSHIFTASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL &&
              (!typeSignedIntegral(lhsType) ||
               !typeUnsignedIntegral(rhsType))) {
            errorNoBinOp(entry, exp->line, exp->character,
                         "an arithmetic shift operation", lhsType, rhsType);
          }

          if (!isLvalue(exp->data.binOpExp.lhs)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "assign a value to");
          } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                     lhsType->data.qualified.constQual) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_BITANDASSIGN:
        case BO_BITXORASSIGN:
        case BO_BITORASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a bitwise operation", lhsType, rhsType);
            }

            if (lhsType != NULL && merged != NULL &&
                !typeImplicitlyConvertable(merged, lhsType))
              errorNoImplicitConversion(entry, exp->line, exp->character,
                                        merged, lhsType);
            typeFree(merged);

            if (!isLvalue(exp->data.binOpExp.lhs)) {
              errorNotLvalue(entry, exp->line, exp->character,
                             "assign a value to");
            } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                       lhsType->data.qualified.constQual) {
              fprintf(stderr,
                      "%s:%zu:%zu: error: cannot assign a value to a constant "
                      "variable\n",
                      entry->inputFilename, exp->line, exp->character);
              entry->errored = true;
            }
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_LANDASSIGN:
        case BO_LORASSIGN: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL &&
              (!typeImplicitlyConvertable(lhsType, boolType) ||
               !typeImplicitlyConvertable(rhsType, boolType))) {
            errorNoBinOp(entry, exp->line, exp->character,
                         "a logical operation", lhsType, rhsType);
          }

          if (!isLvalue(exp->data.binOpExp.lhs)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "assign a value to");
          } else if (lhsType != NULL && lhsType->kind == TK_QUALIFIED &&
                     lhsType->data.qualified.constQual) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot assign a value to a constant "
                    "variable\n",
                    entry->inputFilename, exp->line, exp->character);
            entry->errored = true;
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_LAND:
        case BO_LOR: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL &&
              (!typeImplicitlyConvertable(lhsType, boolType) ||
               !typeImplicitlyConvertable(rhsType, boolType))) {
            errorNoBinOp(entry, exp->line, exp->character,
                         "a logical operation", lhsType, rhsType);
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

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a bitwise operation", lhsType, rhsType);
            }
            return exp->data.binOpExp.type = merged;
          } else {
            return NULL;
          }
        }
        case BO_EQ:
        case BO_NEQ:
        case BO_LT:
        case BO_GT:
        case BO_LTEQ:
        case BO_GTEQ: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = comparisonTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a comparison operation", lhsType, rhsType);
            }
            exp->data.binOpExp.comparisonType = merged;
          }

          return exp->data.binOpExp.type = keywordTypeCreate(TK_BOOL);
        }
        case BO_LSHIFT:
        case BO_LRSHIFT: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL &&
              ((!typeIntegral(lhsType) && !typePointer(lhsType)) ||
               !typeUnsignedIntegral(rhsType))) {
            errorNoBinOp(entry, exp->line, exp->character, "a shift operation",
                         lhsType, rhsType);
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_ARSHIFT: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL &&
              (!typeSignedIntegral(lhsType) ||
               !typeUnsignedIntegral(rhsType))) {
            errorNoBinOp(entry, exp->line, exp->character,
                         "an arithmetic shift operation", lhsType, rhsType);
          }

          return exp->data.binOpExp.type = typeCopy(lhsType);
        }
        case BO_ADD: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            if (typeNumeric(lhsType) && typeNumeric(rhsType)) {
              Type *merged = arithmeticTypeMerge(lhsType, rhsType);
              if (merged == NULL) {
                errorNoBinOp(entry, exp->line, exp->character,
                             "an addition operation", lhsType, rhsType);
              }
              return exp->data.binOpExp.type = merged;
            } else if (typePointer(lhsType) && typeIntegral(rhsType)) {
              return exp->data.binOpExp.type = typeCopy(lhsType);
            } else if (typeIntegral(lhsType) && typePointer(rhsType)) {
              return exp->data.binOpExp.type = typeCopy(rhsType);
            } else {
              errorNoBinOp(entry, exp->line, exp->character,
                           "an additon operation", lhsType, rhsType);
              return NULL;
            }
          } else {
            return NULL;
          }
        }
        case BO_SUB: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            if (typeNumeric(lhsType) && typeNumeric(rhsType)) {
              Type *merged = arithmeticTypeMerge(lhsType, rhsType);
              if (merged == NULL) {
                errorNoBinOp(entry, exp->line, exp->character,
                             "a subtraction operation", lhsType, rhsType);
              }
              return exp->data.binOpExp.type = merged;
            } else if (typePointer(lhsType) && typeIntegral(rhsType)) {
              return exp->data.binOpExp.type = typeCopy(lhsType);
            } else if (typePointer(lhsType) && typePointer(rhsType) &&
                       typeEqual(stripCV(lhsType), stripCV(rhsType))) {
              return exp->data.binOpExp.type = keywordTypeCreate(TK_LONG);
            } else {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a subtraction operation", lhsType, rhsType);
              return NULL;
            }
          } else {
            return NULL;
          }
        }
        case BO_MUL: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a multiplication operation", lhsType, rhsType);
            }
            return exp->data.binOpExp.type = merged;
          } else {
            return NULL;
          }
        }
        case BO_DIV: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a division operation", lhsType, rhsType);
            }
            return exp->data.binOpExp.type = merged;
          } else {
            return NULL;
          }
        }
        case BO_MOD: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            Type *merged = arithmeticTypeMerge(lhsType, rhsType);
            if (merged == NULL) {
              errorNoBinOp(entry, exp->line, exp->character,
                           "a modulo operation", lhsType, rhsType);
            }
            return exp->data.binOpExp.type = merged;
          } else {
            return NULL;
          }
        }
        case BO_FIELD: {
          Type const *structType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          if (structType != NULL) {
            Type const *strippedType = stripCV(structType);
            if (!(strippedType->kind == TK_REFERENCE &&
                  (strippedType->data.reference.entry->kind == SK_STRUCT ||
                   strippedType->data.reference.entry->kind == SK_UNION))) {
              errorNoMembers(entry, exp->line, exp->character, structType);
            } else {
              SymbolTableEntry *ref = strippedType->data.reference.entry;
              if (ref->kind == SK_STRUCT) {
                for (size_t idx = 0; idx < ref->data.structType.fieldNames.size;
                     ++idx) {
                  if (strcmp(exp->data.binOpExp.rhs->data.id.id,
                             ref->data.structType.fieldNames.elements[idx]) ==
                      0) {
                    return exp->data.binOpExp.type = typeCopy(
                               ref->data.structType.fieldTypes.elements[idx]);
                  }
                }
              } else {
                for (size_t idx = 0; idx < ref->data.unionType.optionNames.size;
                     ++idx) {
                  if (strcmp(exp->data.binOpExp.rhs->data.id.id,
                             ref->data.unionType.optionNames.elements[idx]) ==
                      0) {
                    return exp->data.binOpExp.type = typeCopy(
                               ref->data.unionType.optionTypes.elements[idx]);
                  }
                }
              }
              errorNoMember(entry, exp->line, exp->character,
                            exp->data.binOpExp.rhs->data.id.id, structType);
            }
          }

          return NULL;
        }
        case BO_PTRFIELD: {
          Type const *pointerType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          if (pointerType != NULL) {
            Type const *structType = stripCV(pointerType)->data.pointer.base;
            Type const *strippedType = stripCV(structType);
            if (!(strippedType->kind == TK_REFERENCE &&
                  (strippedType->data.reference.entry->kind == SK_STRUCT ||
                   strippedType->data.reference.entry->kind == SK_UNION))) {
              errorNoMembers(entry, exp->line, exp->character, structType);
            } else {
              SymbolTableEntry *ref = strippedType->data.reference.entry;
              if (ref->kind == SK_STRUCT) {
                for (size_t idx = 0; idx < ref->data.structType.fieldNames.size;
                     ++idx) {
                  if (strcmp(exp->data.binOpExp.rhs->data.id.id,
                             ref->data.structType.fieldNames.elements[idx]) ==
                      0) {
                    return exp->data.binOpExp.type = typeCopy(
                               ref->data.structType.fieldTypes.elements[idx]);
                  }
                }
              } else {
                for (size_t idx = 0; idx < ref->data.unionType.optionNames.size;
                     ++idx) {
                  if (strcmp(exp->data.binOpExp.rhs->data.id.id,
                             ref->data.unionType.optionNames.elements[idx]) ==
                      0) {
                    return exp->data.binOpExp.type = typeCopy(
                               ref->data.unionType.optionTypes.elements[idx]);
                  }
                }
              }
              errorNoMember(entry, exp->line, exp->character,
                            exp->data.binOpExp.rhs->data.id.id, structType);
            }
          }
          return NULL;
        }
        case BO_ARRAY: {
          Type const *lhsType =
              typecheckExpression(exp->data.binOpExp.lhs, entry);
          Type const *rhsType =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (lhsType != NULL && rhsType != NULL) {
            if (typeArray(lhsType) && typeIntegral(rhsType)) {
              return exp->data.binOpExp.type =
                         typeCopy(stripCV(lhsType)->data.array.type);
            } else if (typePointer(lhsType) && typeIntegral(rhsType)) {
              return exp->data.binOpExp.type =
                         typeCopy(stripCV(lhsType)->data.pointer.base);
            } else {
              errorNoBinOp(entry, exp->line, exp->character,
                           "an array index operation", lhsType, rhsType);
              return NULL;
            }
          } else {
            return NULL;
          }
        }
        case BO_CAST: {
          Type const *target =
              typecheckExpression(exp->data.binOpExp.rhs, entry);

          if (target != NULL &&
              !typeExplicitlyConvertable(target, exp->data.binOpExp.type)) {
            char *fromString = typeToString(target);
            char *toString = typeToString(exp->data.binOpExp.type);
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot convert a value of type '%s' to "
                    "a value of type '%s'\n",
                    entry->inputFilename, exp->line, exp->character, fromString,
                    toString);
            free(fromString);
            free(toString);
            entry->errored = true;
          }

          return exp->data.binOpExp.type;
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
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL) {
            if (!typePointer(type)) {
              errorNoUnOp(entry, exp->line, exp->character,
                          "a pointer dereference", type);
              return NULL;
            }

            return exp->data.unOpExp.type =
                       typeCopy(stripCV(type)->data.pointer.base);
          } else {
            return NULL;
          }
        }
        case UO_ADDROF: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (!isLvalue(exp->data.unOpExp.target)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "take the address of");
          } else {
            markEscapes(exp->data.unOpExp.target);
          }

          if (type != NULL) {
            return exp->data.unOpExp.type = pointerTypeCreate(typeCopy(type));
          } else {
            return NULL;
          }
        }
        case UO_PREINC:
        case UO_POSTINC: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeNumeric(type) && !typePointer(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "an increment", type);
          }

          if (!isLvalue(exp->data.unOpExp.target)) {
            errorNotLvalue(entry, exp->line, exp->character, "increment");
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_PREDEC:
        case UO_POSTDEC: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeNumeric(type) && !typePointer(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "a decrement", type);
          }

          if (!isLvalue(exp->data.unOpExp.target)) {
            errorNotLvalue(entry, exp->line, exp->character, "decrement");
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_NEG: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeSignedIntegral(type) &&
              !typeFloating(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "a negation", type);
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_LNOT: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeBoolean(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "a logical negation",
                        type);
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_BITNOT: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeIntegral(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "a bitwise not",
                        type);
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_NEGASSIGN: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeSignedIntegral(type) &&
              !typeFloating(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "a negation", type);
          }

          if (!isLvalue(exp->data.unOpExp.target)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "assign a value to");
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_LNOTASSIGN: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeBoolean(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "a logical negation",
                        type);
          }

          if (!isLvalue(exp->data.unOpExp.target)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "assign a value to");
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_BITNOTASSIGN: {
          Type const *type =
              typecheckExpression(exp->data.unOpExp.target, entry);

          if (type != NULL && !typeIntegral(type)) {
            errorNoUnOp(entry, exp->line, exp->character, "a bitwise not",
                        type);
          }

          if (!isLvalue(exp->data.unOpExp.target)) {
            errorNotLvalue(entry, exp->line, exp->character,
                           "assign a value to");
          }

          return exp->data.unOpExp.type = typeCopy(type);
        }
        case UO_SIZEOFEXP: {
          typecheckExpression(exp->data.unOpExp.target, entry);
          return exp->data.unOpExp.type = keywordTypeCreate(TK_ULONG);
        }
        case UO_SIZEOFTYPE: {
          return exp->data.unOpExp.type = keywordTypeCreate(TK_ULONG);
        }
        case UO_PARENS: {
          return exp->data.unOpExp.type = typeCopy(
                     typecheckExpression(exp->data.unOpExp.target, entry));
        }
        default: {
          error(__FILE__, __LINE__, "invalid unop encountered");
        }
      }
    }
    case NT_FUNCALLEXP: {
      Type const *funType =
          typecheckExpression(exp->data.funCallExp.function, entry);

      if (funType != NULL) {
        Type const *stripped = stripCV(funType);
        if (stripped->kind != TK_FUNPTR) {
          errorNoUnOp(entry, exp->line, exp->character, "call", funType);
        } else {
          if (stripped->data.funPtr.argTypes.size !=
              exp->data.funCallExp.arguments->size) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: function expects %zu arguments, but "
                    "was called with %zu\n",
                    entry->inputFilename, exp->line, exp->character,
                    stripped->data.funPtr.argTypes.size,
                    exp->data.funCallExp.arguments->size);
            entry->errored = true;
          } else {
            for (size_t idx = 0; idx < exp->data.funCallExp.arguments->size;
                 ++idx) {
              Node *arg = exp->data.funCallExp.arguments->elements[idx];
              Type const *argType = typecheckExpression(arg, entry);
              if (argType != NULL &&
                  !typeImplicitlyConvertable(
                      argType, stripped->data.funPtr.argTypes.elements[idx])) {
                errorNoImplicitConversion(
                    entry, arg->line, arg->character, argType,
                    stripped->data.funPtr.argTypes.elements[idx]);
              }
            }
          }

          return exp->data.funCallExp.type =
                     typeCopy(stripped->data.funPtr.returnType);
        }
      }

      for (size_t idx = 0; idx < exp->data.funCallExp.arguments->size; ++idx)
        typecheckExpression(exp->data.funCallExp.arguments->elements[idx],
                            entry);
      return NULL;
    }
    case NT_LITERAL: {
      switch (exp->data.literal.literalType) {
        case LT_UBYTE: {
          return exp->data.literal.type = keywordTypeCreate(TK_UBYTE);
        }
        case LT_BYTE: {
          return exp->data.literal.type = keywordTypeCreate(TK_BYTE);
        }
        case LT_USHORT: {
          return exp->data.literal.type = keywordTypeCreate(TK_USHORT);
        }
        case LT_SHORT: {
          return exp->data.literal.type = keywordTypeCreate(TK_SHORT);
        }
        case LT_UINT: {
          return exp->data.literal.type = keywordTypeCreate(TK_UINT);
        }
        case LT_INT: {
          return exp->data.literal.type = keywordTypeCreate(TK_INT);
        }
        case LT_ULONG: {
          return exp->data.literal.type = keywordTypeCreate(TK_ULONG);
        }
        case LT_LONG: {
          return exp->data.literal.type = keywordTypeCreate(TK_LONG);
        }
        case LT_FLOAT: {
          return exp->data.literal.type = keywordTypeCreate(TK_FLOAT);
        }
        case LT_DOUBLE: {
          return exp->data.literal.type = keywordTypeCreate(TK_FLOAT);
        }
        case LT_STRING: {
          return exp->data.literal.type =
                     pointerTypeCreate(keywordTypeCreate(TK_CHAR));
        }
        case LT_CHAR: {
          return exp->data.literal.type = keywordTypeCreate(TK_CHAR);
        }
        case LT_WSTRING: {
          return exp->data.literal.type =
                     pointerTypeCreate(keywordTypeCreate(TK_WCHAR));
        }
        case LT_WCHAR: {
          return exp->data.literal.type = keywordTypeCreate(TK_WCHAR);
        }
        case LT_BOOL: {
          return exp->data.literal.type = keywordTypeCreate(TK_BOOL);
        }
        case LT_NULL: {
          return exp->data.literal.type =
                     pointerTypeCreate(keywordTypeCreate(TK_VOID));
        }
        case LT_AGGREGATEINIT: {
          exp->data.literal.type = aggregateTypeCreate();
          for (size_t idx = 0;
               idx < exp->data.literal.data.aggregateInitVal->size; ++idx) {
            vectorInsert(
                &exp->data.literal.type->data.aggregate.types,
                typeCopy(typecheckExpression(
                    exp->data.literal.data.aggregateInitVal->elements[idx],
                    entry)));
          }
          return exp->data.literal.type;
        }
        default: {
          error(__FILE__, __LINE__, "invalid literal type encountered");
        }
      }
    }
    case NT_SCOPEDID: {
      return exp->data.scopedId.type =
                 exp->data.scopedId.entry->kind == SK_VARIABLE
                     ? typeCopy(exp->data.scopedId.entry->data.variable.type)
                     : referenceTypeCreate(
                           exp->data.scopedId.entry->data.enumConst.parent);
    }
    case NT_ID: {
      if (exp->data.id.entry->kind == SK_VARIABLE) {
        return exp->data.id.type =
                   typeCopy(exp->data.id.entry->data.variable.type);
      } else {
        exp->data.id.type = funPtrTypeCreate(
            typeCopy(exp->data.id.entry->data.function.returnType));
        for (size_t idx = 0;
             idx < exp->data.id.entry->data.function.argumentTypes.size;
             ++idx) {
          vectorInsert(&exp->data.id.type->data.funPtr.argTypes,
                       typeCopy(exp->data.id.entry->data.function.argumentTypes
                                    .elements[idx]));
        }
        return exp->data.id.type;
      }
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
      if (!typeSwitchable(conditionType)) {
        char *typeString = typeToString(conditionType);
        fprintf(stderr,
                "%s:%zu:%zu: error: cannot switch on values of type '%s'\n",
                entry->inputFilename, stmt->data.switchStmt.condition->line,
                stmt->data.switchStmt.condition->character, typeString);
        free(typeString);
        entry->errored = true;
      }

      Vector *cases = stmt->data.switchStmt.cases;
      size_t numValues = 0;
      for (size_t caseIdx = 0; caseIdx < cases->size; ++caseIdx) {
        Node *caseNode = cases->elements[caseIdx];
        switch (caseNode->type) {
          case NT_SWITCHCASE: {
            Vector *values = caseNode->data.switchCase.values;
            numValues += values->size;
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

      bool isSigned =
          typeSignedIntegral(conditionType) ||
          (typeEnum(conditionType) &&
           typeSignedIntegral(
               stripCV(conditionType)
                   ->data.reference.entry->data.enumType.backingType));
      typedef struct {
        union {
          uint64_t unsignedVal;
          int64_t signedVal;
        } value;
        size_t line;
        size_t character;
      } SwitchCaseVal;
      SwitchCaseVal *values = malloc(sizeof(SwitchCaseVal) * numValues);
      size_t currValue = 0;

      bool seenDefault = false;
      size_t firstLine = 0;
      size_t firstCharacter = 0;
      for (size_t idx = 0; idx < cases->size; ++idx) {
        Node *c = cases->elements[idx];
        if (c->type == NT_SWITCHDEFAULT) {
          if (seenDefault) {
            fprintf(stderr,
                    "%s:%zu:%zu: error: cannot have multiple default cases in "
                    "a switch statement\n",
                    entry->inputFilename, c->line, c->character);
            fprintf(stderr, "%s:%zu:%zu: note: first seen here\n",
                    entry->inputFilename, firstLine, firstCharacter);
            entry->errored = true;
          } else {
            seenDefault = true;
            firstLine = c->line;
            firstCharacter = c->character;
          }
        } else {
          Vector *valueNodes = c->data.switchCase.values;
          for (size_t valueIdx = 0; valueIdx < valueNodes->size; ++idx) {
            Node const *valueNode = valueNodes->elements[idx];
            values[currValue].line = valueNode->line;
            values[currValue].character = valueNode->character;
            switch (valueNode->type) {
              case NT_LITERAL: {
                switch (valueNode->data.literal.literalType) {
                  case LT_UBYTE: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.ubyteVal;
                    else
                      values[currValue++].value.unsignedVal =
                          valueNode->data.literal.data.ubyteVal;
                    break;
                  }
                  case LT_BYTE: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.byteVal;
                    else
                      values[currValue++].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.byteVal;
                    break;
                  }
                  case LT_USHORT: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.ushortVal;
                    else
                      values[currValue++].value.unsignedVal =
                          valueNode->data.literal.data.ushortVal;
                    break;
                  }
                  case LT_SHORT: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.shortVal;
                    else
                      values[currValue++].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.shortVal;
                    break;
                  }
                  case LT_UINT: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.uintVal;
                    else
                      values[currValue++].value.unsignedVal =
                          valueNode->data.literal.data.uintVal;
                    break;
                  }
                  case LT_INT: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.intVal;
                    else
                      values[currValue++].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.intVal;
                    break;
                  }
                  case LT_ULONG: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          (int64_t)valueNode->data.literal.data.ulongVal;
                    else
                      values[currValue++].value.unsignedVal =
                          valueNode->data.literal.data.ulongVal;
                    break;
                  }
                  case LT_LONG: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.longVal;
                    else
                      values[currValue++].value.unsignedVal =
                          (uint64_t)valueNode->data.literal.data.longVal;
                    break;
                  }
                  case LT_CHAR: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.charVal;
                    else
                      values[currValue++].value.unsignedVal =
                          valueNode->data.literal.data.charVal;
                    break;
                  }
                  case LT_WCHAR: {
                    if (isSigned)
                      values[currValue++].value.signedVal =
                          valueNode->data.literal.data.wcharVal;
                    else
                      values[currValue++].value.unsignedVal =
                          valueNode->data.literal.data.wcharVal;
                    break;
                  }
                  default: {
                    error(__FILE__, __LINE__,
                          "can't have a switch case value of that type");
                  }
                }
                break;
              }
              case NT_SCOPEDID: {
                if (isSigned)
                  values[currValue++].value.signedVal =
                      valueNode->data.scopedId.entry->data.enumConst.data
                          .signedValue;
                else
                  values[currValue++].value.unsignedVal =
                      valueNode->data.scopedId.entry->data.enumConst.data
                          .unsignedValue;
                break;
              }
              default: {
                error(__FILE__, __LINE__,
                      "can't have a switch case value with that node");
              }
            }

            for (size_t valueIdx = 0; valueIdx < currValue - 1; ++valueIdx) {
              if ((isSigned && values[valueIdx].value.signedVal ==
                                   values[currValue - 1].value.signedVal) ||
                  (!isSigned && values[valueIdx].value.unsignedVal ==
                                    values[currValue - 1].value.unsignedVal)) {
                fprintf(stderr,
                        "%s:%zu:%zu: error: cannot have multiple cases with "
                        "the same value in a switch statement\n",
                        entry->inputFilename, values[currValue - 1].line,
                        values[currValue - 1].character);
                fprintf(stderr, "%s:%zu:%zu: note: first seen here\n",
                        entry->inputFilename, values[valueIdx].line,
                        values[valueIdx].character);
                entry->errored = true;
                break;
              }
            }
          }
        }
      }
      free(values);

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
      Node *firstName = names->elements[0];
      if (!typeComplete(firstName->data.id.entry->data.variable.type))
        errorIncompleteType(entry, stmt->data.varDefnStmt.type->line,
                            stmt->data.varDefnStmt.type->character,
                            firstName->data.id.entry->data.variable.type);
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
    case NT_STRUCTDECL: {
      if (structRecursive(stmt->data.structDecl.name->data.id.entry)) {
        errorRecursiveDecl(entry, stmt->line, stmt->character, "struct",
                           stmt->data.structDecl.name->data.id.id);
      }
      break;
    }
    case NT_UNIONDECL: {
      if (unionRecursive(stmt->data.unionDecl.name->data.id.entry)) {
        errorRecursiveDecl(entry, stmt->line, stmt->character, "union",
                           stmt->data.unionDecl.name->data.id.id);
      }
      break;
    }
    case NT_TYPEDEFDECL: {
      if (typedefRecursive(stmt->data.typedefDecl.name->data.id.entry)) {
        errorRecursiveDecl(entry, stmt->line, stmt->character, "typedef",
                           stmt->data.typedefDecl.name->data.id.id);
      }
      break;
    }
    default: {
      break;  // nothing to check
    }
  }
}

/**
 * typechecks a file
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
        Node *firstName = names->elements[0];
        if (!typeComplete(firstName->data.id.entry->data.variable.type))
          errorIncompleteType(entry, body->data.varDefn.type->line,
                              body->data.varDefn.type->character,
                              firstName->data.id.entry->data.variable.type);
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
      case NT_VARDECL: {
        Vector *names = body->data.varDecl.names;
        Node *firstName = names->elements[0];
        if (!typeComplete(firstName->data.id.entry->data.variable.type))
          errorIncompleteType(entry, body->data.varDecl.type->line,
                              body->data.varDecl.type->character,
                              firstName->data.id.entry->data.variable.type);
        break;
      }
      case NT_FUNDEFN: {
        Type const *returnType =
            body->data.funDefn.name->data.id.entry->data.function.returnType;
        if (!((returnType->kind == TK_KEYWORD &&
               returnType->data.keyword.keyword == TK_VOID) ||
              typeComplete(returnType)))
          errorIncompleteType(entry, body->data.funDefn.returnType->line,
                              body->data.funDefn.returnType->character,
                              returnType);
        Vector *argTypes = &body->data.funDefn.name->data.id.entry->data
                                .function.argumentTypes;
        for (size_t idx = 0; idx < argTypes->size; ++idx) {
          if (!typeComplete(argTypes->elements[idx])) {
            Node *typeNode = body->data.funDefn.argTypes->elements[idx];
            errorIncompleteType(entry, typeNode->line, typeNode->character,
                                argTypes->elements[idx]);
          }
        }
        typecheckStmt(body->data.funDefn.body, returnType, entry);
        break;
      }
      case NT_FUNDECL: {
        Type const *returnType =
            body->data.funDecl.name->data.id.entry->data.function.returnType;
        if (!((returnType->kind == TK_KEYWORD &&
               returnType->data.keyword.keyword == TK_VOID) ||
              typeComplete(returnType)))
          errorIncompleteType(entry, body->data.funDecl.returnType->line,
                              body->data.funDecl.returnType->character,
                              returnType);
        Vector *argTypes = &body->data.funDecl.name->data.id.entry->data
                                .function.argumentTypes;
        for (size_t idx = 0; idx < argTypes->size; ++idx) {
          if (!typeComplete(argTypes->elements[idx])) {
            Node *typeNode = body->data.funDecl.argTypes->elements[idx];
            errorIncompleteType(entry, typeNode->line, typeNode->character,
                                argTypes->elements[idx]);
          }
        }
        break;
      }
      case NT_STRUCTDECL: {
        if (structRecursive(body->data.structDecl.name->data.id.entry)) {
          errorRecursiveDecl(entry, body->line, body->character, "struct",
                             body->data.structDecl.name->data.id.id);
        }
        break;
      }
      case NT_UNIONDECL: {
        if (unionRecursive(body->data.unionDecl.name->data.id.entry)) {
          errorRecursiveDecl(entry, body->line, body->character, "union",
                             body->data.unionDecl.name->data.id.id);
        }
        break;
      }
      case NT_TYPEDEFDECL: {
        if (typedefRecursive(body->data.typedefDecl.name->data.id.entry)) {
          errorRecursiveDecl(entry, body->line, body->character, "typedef",
                             body->data.typedefDecl.name->data.id.id);
        }
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
    typecheckFile(&fileList.entries[idx]);
    errored = errored || fileList.entries[idx].errored;
  }

  typeFree(boolType);

  if (errored) return -1;

  return 0;
}