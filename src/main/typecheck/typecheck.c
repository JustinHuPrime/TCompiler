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
#include <string.h>

// helpers
static bool expressionIsLvalue(Node *expression) {
  return false;  // TODO: write this
}

// expressions
static Type *typecheckExpression(Node *expression, Report *report,
                                 Options const *options, char const *filename,
                                 TypeVector const *argTypes, bool directCall) {
  switch (expression->type) {
    case NT_SEQEXP: {
      Type *prefix = typecheckExpression(expression->data.seqExp.prefix, report,
                                         options, filename, NULL, false);
      Type *last = typecheckExpression(expression->data.seqExp.last, report,
                                       options, filename, argTypes, false);
      if (prefix != NULL) {
        typeDestroy(prefix);
      }
      return last;
    }
    case NT_BINOPEXP: {
      // TODO: write this
      switch (expression->data.binOpExp.op) {
        case BO_ASSIGN: {
          Type *to = typecheckExpression(expression->data.binOpExp.lhs, report,
                                         options, filename, NULL, false);
          if (to == NULL) {
            return NULL;  // can't go further
          }

          Type *from = typecheckExpression(
              expression->data.binOpExp.rhs, report, options, filename,
              to->kind == K_FUNCTION_PTR ? to->data.functionPtr.argumentTypes
                                         : NULL,
              false);
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
          }
          return to;
        }
        case BO_MULASSIGN: {
          return NULL;
        }
        case BO_DIVASSIGN: {
          return NULL;
        }
        case BO_MODASSIGN: {
          return NULL;
        }
        case BO_ADDASSIGN: {
          return NULL;
        }
        case BO_SUBASSIGN: {
          return NULL;
        }
        case BO_LSHIFTASSIGN: {
          return NULL;
        }
        case BO_LRSHIFTASSIGN: {
          return NULL;
        }
        case BO_ARSHIFTASSIGN: {
          return NULL;
        }
        case BO_BITANDASSIGN: {
          return NULL;
        }
        case BO_BITXORASSIGN: {
          return NULL;
        }
        case BO_BITORASSIGN: {
          return NULL;
        }
        case BO_BITAND: {
          return NULL;
        }
        case BO_BITOR: {
          return NULL;
        }
        case BO_BITXOR: {
          return NULL;
        }
        case BO_SPACESHIP: {
          return NULL;
        }
        case BO_LSHIFT: {
          return NULL;
        }
        case BO_LRSHIFT: {
          return NULL;
        }
        case BO_ARSHIFT: {
          return NULL;
        }
        case BO_ADD: {
          return NULL;
        }
        case BO_SUB: {
          return NULL;
        }
        case BO_MUL: {
          return NULL;
        }
        case BO_DIV: {
          return NULL;
        }
        case BO_MOD: {
          return NULL;
        }
        case BO_ARRAYACCESS: {
          return NULL;
        }
        default: {
          return NULL;  // invalid enum
        }
      }
    }
    case NT_UNOPEXP: {
      // TODO: write this
      switch (expression->data.unOpExp.op) {
        case UO_DEREF: {
          Type *target =
              typecheckExpression(expression->data.unOpExp.target, report,
                                  options, filename, NULL, false);
          if (target == NULL) {
            return NULL;
          }
          // must be pointer or const pointer
          if (!(target->kind == K_PTR ||
                (target->kind == K_CONST &&
                 target->data.modifier.type->kind == K_PTR))) {
            reportError(
                report,
                "%s:%zu:%zu: error: attempted to dereference a non-pointer",
                filename, expression->line, expression->character);
            typeDestroy(target);
            return NULL;
          } else {
            if (target->kind == K_PTR) {
              Type *retVal = target->data.modifier.type;
              free(target);
              return retVal;
            } else {
              Type *retVal = target->data.modifier.type->data.modifier.type;
              free(target->data.modifier.type);
              free(target);
              return retVal;
            }
          }
        }
        case UO_ADDROF: {
          Type *target =
              typecheckExpression(expression->data.unOpExp.target, report,
                                  options, filename, NULL, false);
          if (target == NULL) {
            return NULL;
          }
          if (!expressionIsLvalue(expression->data.unOpExp.target)) {
            reportError(
                report,
                "%s:%zu:%zu: error: cannot take the address of a non-lvalue",
                filename, expression->line, expression->character);
            typeDestroy(target);
            return NULL;
          } else {
            return modifierTypeCreate(K_PTR, target);
          }
        }
        case UO_PREINC: {
          return NULL;
        }
        case UO_PREDEC: {
          return NULL;
        }
        case UO_NEG: {
          return NULL;
        }
        case UO_LNOT: {
          return NULL;
        }
        case UO_BITNOT: {
          return NULL;
        }
        case UO_POSTINC: {
          return NULL;
        }
        case UO_POSTDEC: {
          return NULL;
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
        if (lhs != NULL) {
          typeDestroy(lhs);
        }
        if (rhs != NULL) {
          typeDestroy(rhs);
        }
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
        return keywordTypeCreate(K_BOOL);
      }
    }
    case NT_LANDASSIGNEXP: {
      // TODO: write this
      return NULL;
    }
    case NT_LORASSIGNEXP: {
      // TODO: write this
      return NULL;
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
        if (condition != NULL) {
          typeDestroy(condition);
        }
        if (thenExp != NULL) {
          typeDestroy(thenExp);
        }
        if (elseExp != NULL) {
          typeDestroy(elseExp);
        }
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

      Type *expType = typeTernaryExpMerge(thenExp, elseExp);
      if (expType == NULL) {
        reportError(report,
                    "%s:%zu:%zu: error: conflicting types in branches of "
                    "ternary expression",
                    filename, expression->line, expression->character);
        bad = true;
      }
      typeDestroy(condition);
      typeDestroy(thenExp);
      typeDestroy(elseExp);

      if (bad) {
        if (expType != NULL) {
          typeDestroy(expType);
        }

        return NULL;
      }
      return expType;
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
        // get out - can't go further.
        if (lhsType != NULL) {
          typeDestroy(lhsType);
        }
        if (rhsType != NULL) {
          typeDestroy(rhsType);
        }
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
      typeDestroy(lhsType);
      typeDestroy(rhsType);
      return bad ? NULL : keywordTypeCreate(K_BOOL);
    }
    case NT_STRUCTACCESSEXP: {
      Type *lhs = typecheckExpression(expression->data.structAccessExp.base,
                                      report, options, filename, NULL, false);
      if (lhs == NULL) {
        return NULL;
      }

      if (lhs->kind != K_STRUCT && lhs->kind != K_UNION) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to access a field of "
                    "something that is not a struct and not a union",
                    filename, expression->line, expression->character);
        typeDestroy(lhs);
        return NULL;
      }

      SymbolInfo *definition = lhs->data.reference.referenced;
      typeDestroy(lhs);
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
        return typeCopy(types->elements[fieldIdx]);
      }
    }
    case NT_STRUCTPTRACCESSEXP: {
      Type *lhs = typecheckExpression(expression->data.structPtrAccessExp.base,
                                      report, options, filename, NULL, false);
      if (lhs == NULL) {
        return NULL;
      }

      if (lhs->kind != K_PTR) {
        reportError(report, "%s:%zu:%zu: error: not a pointer", filename,
                    expression->line, expression->character);
        typeDestroy(lhs);
        return NULL;
      } else if (lhs->data.modifier.type->kind != K_STRUCT &&
                 lhs->data.modifier.type->kind != K_UNION) {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to access a field of "
                    "something that is not a struct and not a union",
                    filename, expression->line, expression->character);
        typeDestroy(lhs);
        return NULL;
      }

      SymbolInfo *definition =
          lhs->data.modifier.type->data.reference.referenced;
      typeDestroy(lhs);
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
        return NULL;
      } else {
        return typeCopy(types->elements[fieldIdx]);
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
          typeVectorInsert(actualArgTypes, argType);
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
      } else if (fnType->kind != K_FUNCTION_PTR) {
        typeDestroy(fnType);
        reportError(report,
                    "%s:%zu:%zu: error: attempted to make a function call on a "
                    "non-function",
                    filename, expression->line, expression->character);
        return NULL;
      }

      return typeCopy(fnType->data.functionPtr.returnType);
    }
    case NT_CONSTEXP: {
      switch (expression->data.constExp.type) {
        case CT_UBYTE: {
          return keywordTypeCreate(K_UBYTE);
        }
        case CT_BYTE: {
          return keywordTypeCreate(K_BYTE);
        }
        case CT_CHAR: {
          return keywordTypeCreate(K_CHAR);
        }
        case CT_USHORT: {
          return keywordTypeCreate(K_USHORT);
        }
        case CT_SHORT: {
          return keywordTypeCreate(K_SHORT);
        }
        case CT_UINT: {
          return keywordTypeCreate(K_UINT);
        }
        case CT_INT: {
          return keywordTypeCreate(K_INT);
        }
        case CT_WCHAR: {
          return keywordTypeCreate(K_WCHAR);
        }
        case CT_ULONG: {
          return keywordTypeCreate(K_ULONG);
        }
        case CT_LONG: {
          return keywordTypeCreate(K_LONG);
        }
        case CT_FLOAT: {
          return keywordTypeCreate(K_FLOAT);
        }
        case CT_DOUBLE: {
          return keywordTypeCreate(K_DOUBLE);
        }
        case CT_BOOL: {
          return keywordTypeCreate(K_BOOL);
        }
        case CT_STRING: {
          return modifierTypeCreate(
              K_PTR, modifierTypeCreate(K_CONST, keywordTypeCreate(K_CHAR)));
        }
        case CT_WSTRING: {
          return modifierTypeCreate(
              K_PTR, modifierTypeCreate(K_CONST, keywordTypeCreate(K_WCHAR)));
        }
        case CT_NULL: {
          return modifierTypeCreate(K_PTR, keywordTypeCreate(K_VOID));
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
          typeVectorInsert(elementTypes, elementType);
        }
      }

      if (elementTypes != NULL) {
        return aggregateInitTypeCreate(elementTypes);
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
      if (!typeCastable(expression->data.castExp.toType, targetType)) {
        char *toTypeName = typeToString(expression->data.castExp.toType);
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
        return typeCopy(expression->data.castExp.toType);
      }
    }
    case NT_SIZEOFTYPEEXP: {
      return keywordTypeCreate(TK_ULONG);
    }
    case NT_SIZEOFEXPEXP: {
      expression->data.sizeofExpExp.targetType =
          typecheckExpression(expression->data.sizeofExpExp.target, report,
                              options, filename, NULL, false);
      return keywordTypeCreate(TK_ULONG);
    }
    case NT_ID: {
      SymbolInfo *info = expression->data.id.symbol;
      if (info->kind == SK_VAR) {
        return typeCopy(info->data.var.type);
      } else if (info->kind == SK_FUNCTION && argTypes == NULL) {
        // ambiguous overload, maybe?
        OverloadSet *overloadSet = &info->data.function.overloadSet;
        if (overloadSet->size == 1) {
          OverloadSetElement *elm = overloadSet->elements[0];
          expression->data.id.overload = elm;
          return modifierTypeCreate(
              K_CONST,
              functionPtrTypeCreate(typeCopy(elm->returnType),
                                    typeVectorCopy(&elm->argumentTypes)));
        } else {
          reportError(report,
                      "%s:%zu:%zu: error: cannot determine appropriate "
                      "overload in set to use",
                      filename, expression->line, expression->character);
          return NULL;
        }
      } else if (info->kind == SK_FUNCTION) {
        // TODO: figure out overload in set
        // use directCall - if direct, match w/ default args, else match exact
        // arg string
        return NULL;
      } else {
        return NULL;  // not a valid expression - parse error
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