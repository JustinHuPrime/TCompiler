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

// Implementation of symbol table building

#include "typecheck/buildSymbolTable.h"

#include "constants.h"

#include <string.h>

// helpers
static Type *astToType(Node const *ast, Report *report, Options const *options,
                       Environment const *env, char const *filename) {
  switch (ast->type) {
    case NT_KEYWORDTYPE: {
      return keywordTypeCreate(ast->data.keywordType.type - TK_VOID + K_VOID);
    }
    case NT_ID: {
      SymbolInfo *info = environmentLookup(env, report, ast->data.id.id,
                                           ast->line, ast->character, filename);
      return info == NULL
                 ? NULL
                 : referneceTypeCreate(
                       info->data.type.kind - TDK_STRUCT + K_STRUCT, info);
    }
    case NT_CONSTTYPE: {
      if (ast->data.constType.target->type == NT_CONSTTYPE) {
        switch (optionsGet(options, optionWDuplicateDeclSpecifier)) {
          case O_WT_ERROR: {
            reportError(report,
                        "%s:%zu:%zu: error: duplciate 'const' specifier",
                        filename, ast->line, ast->character);
            return NULL;
          }
          case O_WT_WARN: {
            reportWarning(report,
                          "%s:%zu:%zu: warning: duplciate 'const' specifier",
                          filename, ast->line, ast->character);
            return astToType(ast->data.constType.target, report, options, env,
                             filename);
          }
          case O_WT_IGNORE: {
            return astToType(ast->data.constType.target, report, options, env,
                             filename);
          }
        }
      }

      Type *subType =
          astToType(ast->data.constType.target, report, options, env, filename);
      return subType == NULL ? NULL : modifierTypeCreate(K_CONST, subType);
    }
    case NT_ARRAYTYPE: {
      Node const *sizeConst = ast->data.arrayType.size;
      switch (sizeConst->data.constExp.type) {
        case CT_UBYTE: {
          if (sizeConst->data.constExp.value.ubyteVal == 0) {
            reportError(report,
                        "%s:%zu:%zu: error: expected a non-zero array length",
                        filename, sizeConst->line, sizeConst->character);
            return NULL;
          }
          break;
        }
        case CT_USHORT:
        case CT_UINT:
        case CT_ULONG: {
          break;
        }
        default: {
          reportError(report,
                      "%s:%zu:%zu: error: expected an unsigned integer for an "
                      "array size, but found %s",
                      filename, sizeConst->line, sizeConst->character,
                      constTypeToString(sizeConst->data.constExp.type));
          return NULL;
        }
      }
      size_t size;
      switch (sizeConst->data.constExp.type) {
        case CT_UBYTE: {
          size = sizeConst->data.constExp.value.ubyteVal;
          break;
        }
        case CT_USHORT: {
          size = sizeConst->data.constExp.value.ushortVal;
          break;
        }
        case CT_UINT: {
          size = sizeConst->data.constExp.value.uintVal;
          break;
        }
        case CT_ULONG: {
          size = sizeConst->data.constExp.value.ulongVal;
          break;
        }
        default: {
          return NULL;  // error: mutation in const ptr
        }
      }
      Node *element = ast->data.arrayType.element;
      Type *subType = astToType(element, report, options, env, filename);
      if (subType->kind == K_CONST) {
        reportError(report,
                    "%s:%zu:%zu: error: may not have constant array elements, "
                    "only a constant array",
                    filename, element->line, element->character);
        typeDestroy(subType);
        return NULL;
      }
      return subType == NULL ? NULL : arrayTypeCreate(subType, size);
    }
    case NT_PTRTYPE: {
      Type *subType =
          astToType(ast->data.ptrType.target, report, options, env, filename);
      return subType == NULL ? NULL : modifierTypeCreate(K_PTR, subType);
    }
    case NT_FNPTRTYPE: {
      Type *retType = astToType(ast->data.fnPtrType.returnType, report, options,
                                env, filename);
      if (retType == NULL) {
        return NULL;
      }

      TypeVector *argTypes = typeVectorCreate();
      for (size_t idx = 0; idx < ast->data.fnPtrType.argTypes->size; idx++) {
        Type *argType = astToType(ast->data.fnPtrType.argTypes->elements[idx],
                                  report, options, env, filename);
        if (argType == NULL) {
          typeDestroy(retType);
        }

        typeVectorInsert(argTypes, argType);
      }

      return functionPtrTypeCreate(retType, argTypes);
    }
    default: {
      return NULL;  // error: not syntactically valid
    }
  }
}
static void checkId(Node *id, Report *report, Options const *options,
                    char const *filename) {
  if (strncmp(id->data.id.id, "__", 2) == 0) {
    switch (optionsGet(options, optionWReservedId)) {
      case O_WT_ERROR: {
        reportError(report,
                    "%s:%zu:%zu: error: attempted to define something using a "
                    "reserved identifier",
                    filename, id->line, id->character);
        return;
      }
      case O_WT_WARN: {
        reportWarning(
            report,
            "%s:%zu:%zu: warning: attempted to define something using a "
            "reserved identifier",
            filename, id->line, id->character);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  }
}

// expression
static void buildStabExpression(Node *expression, Report *report,
                                Options const *options, Environment *env,
                                char const *filename, char const *moduleName) {
  if (expression == NULL) {
    return;
  } else {
    switch (expression->type) {
      case NT_SEQEXP: {
        buildStabExpression(expression->data.seqExp.prefix, report, options,
                            env, filename, moduleName);
        buildStabExpression(expression->data.seqExp.last, report, options, env,
                            filename, moduleName);
        break;
      }
      case NT_BINOPEXP: {
        buildStabExpression(expression->data.binOpExp.lhs, report, options, env,
                            filename, moduleName);
        buildStabExpression(expression->data.binOpExp.rhs, report, options, env,
                            filename, moduleName);
        break;
      }
      case NT_UNOPEXP: {
        buildStabExpression(expression->data.unOpExp.target, report, options,
                            env, filename, moduleName);
        break;
      }
      case NT_COMPOPEXP: {
        buildStabExpression(expression->data.compOpExp.lhs, report, options,
                            env, filename, moduleName);
        buildStabExpression(expression->data.compOpExp.rhs, report, options,
                            env, filename, moduleName);
        break;
      }
      case NT_LANDASSIGNEXP: {
        buildStabExpression(expression->data.landAssignExp.lhs, report, options,
                            env, filename, moduleName);
        buildStabExpression(expression->data.landAssignExp.rhs, report, options,
                            env, filename, moduleName);
        break;
      }
      case NT_LORASSIGNEXP: {
        buildStabExpression(expression->data.lorAssignExp.lhs, report, options,
                            env, filename, moduleName);
        buildStabExpression(expression->data.lorAssignExp.rhs, report, options,
                            env, filename, moduleName);
        break;
      }
      case NT_TERNARYEXP: {
        buildStabExpression(expression->data.ternaryExp.condition, report,
                            options, env, filename, moduleName);
        buildStabExpression(expression->data.ternaryExp.thenExp, report,
                            options, env, filename, moduleName);
        buildStabExpression(expression->data.ternaryExp.elseExp, report,
                            options, env, filename, moduleName);
        break;
      }
      case NT_LANDEXP: {
        buildStabExpression(expression->data.landExp.lhs, report, options, env,
                            filename, moduleName);
        buildStabExpression(expression->data.landExp.rhs, report, options, env,
                            filename, moduleName);
        break;
      }
      case NT_LOREXP: {
        buildStabExpression(expression->data.lorExp.lhs, report, options, env,
                            filename, moduleName);
        buildStabExpression(expression->data.lorExp.rhs, report, options, env,
                            filename, moduleName);
        break;
      }
      case NT_STRUCTACCESSEXP: {
        buildStabExpression(expression->data.structAccessExp.base, report,
                            options, env, filename, moduleName);
        break;
      }
      case NT_STRUCTPTRACCESSEXP: {
        buildStabExpression(expression->data.structPtrAccessExp.base, report,
                            options, env, filename, moduleName);
        break;
      }
      case NT_FNCALLEXP: {
        buildStabExpression(expression->data.fnCallExp.who, report, options,
                            env, filename, moduleName);
        NodeList *args = expression->data.fnCallExp.args;
        for (size_t idx = 0; idx < args->size; idx++) {
          buildStabExpression(args->elements[idx], report, options, env,
                              filename, moduleName);
        }
        break;
      }
      case NT_AGGREGATEINITEXP: {
        NodeList *elements = expression->data.aggregateInitExp.elements;
        for (size_t idx = 0; idx < elements->size; idx++) {
          buildStabExpression(elements->elements[idx], report, options, env,
                              filename, moduleName);
        }
        break;
      }
      case NT_CASTEXP: {
        expression->data.castExp.resultType = astToType(
            expression->data.castExp.toWhat, report, options, env, filename);
        buildStabExpression(expression->data.castExp.target, report, options,
                            env, filename, moduleName);
        break;
      }
      case NT_SIZEOFTYPEEXP: {
        expression->data.sizeofTypeExp.targetType =
            astToType(expression->data.sizeofTypeExp.target, report, options,
                      env, filename);
        break;
      }
      case NT_SIZEOFEXPEXP: {
        buildStabExpression(expression->data.sizeofExpExp.target, report,
                            options, env, filename, moduleName);
        break;
      }
      case NT_ID: {
        expression->data.id.symbol = environmentLookup(
            env, report, expression->data.id.id, expression->line,
            expression->character, filename);
        break;
      }
      default: {
        break;  // nothing to build a stab for
      }
    }
  }
}

// statement
static void buildStabVarDecl(Node *, Report *, Options const *, Environment *,
                             char const *, char const *, bool);
static void buildStabStructOrUnionDecl(Node *, bool, Report *, Options const *,
                                       Environment *, char const *,
                                       char const *);
static void buildStabStructOrUnionForwardDecl(Node *, bool, Report *,
                                              Options const *, Environment *,
                                              char const *, char const *);
static void buildStabEnumDecl(Node *, Report *, Options const *, Environment *,
                              char const *, char const *);
static void buildStabEnumForwardDecl(Node *, Report *, Options const *,
                                     Environment *, char const *, char const *);
static void buildStabTypedefDecl(Node *, Report *, Options const *,
                                 Environment *, char const *, char const *);
static void buildStabStmt(Node *statement, Report *report,
                          Options const *options, Environment *env,
                          char const *filename, char const *moduleName) {
  if (statement == NULL) {
    return;
  } else {
    switch (statement->type) {
      case NT_COMPOUNDSTMT: {
        environmentPush(env);
        NodeList *statements = statement->data.compoundStmt.statements;
        for (size_t idx = 0; idx < statements->size; idx++) {
          buildStabStmt(statements->elements[idx], report, options, env,
                        filename, moduleName);
        }
        statement->data.compoundStmt.localSymbols = environmentPop(env);
        break;
      }
      case NT_IFSTMT: {
        buildStabExpression(statement->data.ifStmt.condition, report, options,
                            env, filename, moduleName);
        buildStabStmt(statement->data.ifStmt.thenStmt, report, options, env,
                      filename, moduleName);
        buildStabStmt(statement->data.ifStmt.elseStmt, report, options, env,
                      filename, moduleName);
        break;
      }
      case NT_WHILESTMT: {
        buildStabExpression(statement->data.whileStmt.condition, report,
                            options, env, filename, moduleName);
        buildStabStmt(statement->data.whileStmt.body, report, options, env,
                      filename, moduleName);
        break;
      }
      case NT_DOWHILESTMT: {
        buildStabStmt(statement->data.doWhileStmt.condition, report, options,
                      env, filename, moduleName);
        buildStabExpression(statement->data.doWhileStmt.condition, report,
                            options, env, filename, moduleName);
        break;
      }
      case NT_FORSTMT: {
        environmentPush(env);
        if (statement->data.forStmt.initialize != NULL) {
          if (statement->data.forStmt.initialize->type == NT_VARDECL) {
            buildStabVarDecl(statement->data.forStmt.initialize, report,
                             options, env, filename, moduleName, true);
          } else {
            buildStabExpression(statement->data.forStmt.initialize, report,
                                options, env, filename, moduleName);
          }
        }
        buildStabExpression(statement->data.forStmt.condition, report, options,
                            env, filename, moduleName);
        buildStabExpression(statement->data.forStmt.update, report, options,
                            env, filename, moduleName);
        buildStabStmt(statement->data.forStmt.body, report, options, env,
                      filename, moduleName);
        statement->data.forStmt.localSymbols = environmentPop(env);
        break;
      }
      case NT_SWITCHSTMT: {
        buildStabExpression(statement->data.switchStmt.onWhat, report, options,
                            env, filename, moduleName);
        environmentPush(env);
        NodeList *cases = statement->data.switchStmt.cases;
        for (size_t idx = 0; idx < cases->size; idx++) {
          Node *switchCase = cases->elements[idx];
          buildStabStmt(switchCase->type == NT_NUMCASE
                            ? switchCase->data.numCase.body
                            : switchCase->data.defaultCase.body,
                        report, options, env, filename, moduleName);
        }
        statement->data.switchStmt.localSymbols = environmentPop(env);
        break;
      }
      case NT_RETURNSTMT: {
        buildStabExpression(statement->data.returnStmt.value, report, options,
                            env, filename, moduleName);
        break;
      }
      case NT_EXPRESSIONSTMT: {
        buildStabExpression(statement->data.expressionStmt.expression, report,
                            options, env, filename, moduleName);
        break;
      }
      case NT_VARDECL: {
        buildStabVarDecl(statement, report, options, env, filename, moduleName,
                         true);
        break;
      }
      case NT_STRUCTDECL:
      case NT_UNIONDECL: {
        buildStabStructOrUnionDecl(statement, statement->type == NT_STRUCTDECL,
                                   report, options, env, filename, moduleName);
        break;
      }
      case NT_STRUCTFORWARDDECL:
      case NT_UNIONFORWARDDECL: {
        buildStabStructOrUnionForwardDecl(
            statement, statement->type == NT_STRUCTFORWARDDECL, report, options,
            env, filename, moduleName);
        break;
      }
      case NT_ENUMDECL: {
        buildStabEnumDecl(statement, report, options, env, filename,
                          moduleName);
        break;
      }
      case NT_ENUMFORWARDDECL: {
        buildStabEnumForwardDecl(statement, report, options, env, filename,
                                 moduleName);
        break;
      }
      case NT_TYPEDEFDECL: {
        buildStabTypedefDecl(statement, report, options, env, filename,
                             moduleName);
        break;
      }
      default: {
        break;  // no expressions to deal with
      }
    }
  }
}

// top level
static void buildStabParameter(Node const *type, Node *name, Report *report,
                               Options const *options, Environment *env,
                               char const *filename, char const *moduleName) {
  Type *paramType = astToType(type, report, options, env, filename);
  if (paramType == NULL) {
    return;
  }
  SymbolInfo *info = symbolTableGet(environmentTop(env), name->data.id.id);

  if (info != NULL) {
    // duplicate name - can't be anything else
    reportError(report, "%s:%zu:%zu: error: '%s' has already been declared",
                filename, name->line, name->character, name->data.id.id);
  } else {
    checkId(name, report, options, filename);
    info = varSymbolInfoCreate(moduleName, paramType, true,
                               typeIsComposite(paramType));
    name->data.id.symbol = info;
    symbolTablePut(environmentTop(env), name->data.id.id, info);
  }
}
static OverloadSetElement *astToOverloadSetElement(
    Report *report, Options const *options, Environment const *env,
    char const *filename, Node const *returnTypeNode, size_t numArgs,
    Node **argTypes, Node **argDefaults, bool defined) {
  OverloadSetElement *overload = overloadSetElementCreate();

  Type *returnType = astToType(returnTypeNode, report, options, env, filename);
  if (returnType == NULL) {
    overloadSetElementDestroy(overload);
    return NULL;
  } else if (typeIsIncomplete(returnType, env) && returnType->kind != K_VOID) {
    reportError(
        report,
        "%s:%zu:%zu: error: function declared as returning an incomplete type",
        filename, returnTypeNode->line, returnTypeNode->character);
    overloadSetElementDestroy(overload);
    return NULL;
  }
  overload->returnType = returnType;

  for (size_t idx = 0; idx < numArgs; idx++) {
    Node *arg = argTypes[idx];
    Type *argType = astToType(arg, report, options, env, filename);
    if (argType == NULL) {
      overloadSetElementDestroy(overload);
      return NULL;
    } else if (typeIsIncomplete(argType, env)) {
      reportError(report,
                  "%s:%zu:%zu: error: function declared as taking a parameter "
                  "of an incomplete type",
                  filename, arg->line, arg->character);
      overloadSetElementDestroy(overload);
      return NULL;
    }
    typeVectorInsert(&overload->argumentTypes, argType);
  }

  overload->defined = defined;
  overload->numOptional = 0;
  for (size_t idx = 0; idx < numArgs; idx++) {
    if (argDefaults[idx] != NULL) {
      overload->numOptional = numArgs - idx;
      break;
    }
  }

  return overload;
}
static void buildStabFnDefn(Node *fn, Report *report, Options const *options,
                            Environment *env, char const *filename,
                            char const *moduleName) {
  // INVARIANT: env has no scopes
  // must not be declared/defined as a non-function, must not allow a function
  // with the same input args and name to be declared/defined
  Node *name = fn->data.function.id;
  SymbolInfo *info = symbolTableGet(env->currentModule, name->data.id.id);
  OverloadSetElement *overload;
  if (info != NULL && info->kind != SK_FUNCTION) {
    // already declared/defined as a non-function - error!
    reportError(report, "%s:%zu:%zu: error: '%s' already declared as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info == NULL) {
    // not declared/defined - do that now
    overload = astToOverloadSetElement(
        report, options, env, filename, fn->data.function.returnType,
        fn->data.function.formals->size,
        fn->data.function.formals->firstElements,
        fn->data.function.formals->thirdElements, true);
    if (overload == NULL) {
      return;
    }

    info = functionSymbolInfoCreate(moduleName);
    overloadSetInsert(&info->data.function.overloadSet, overload);
    symbolTablePut(env->currentModule, name->data.id.id, info);
  } else {
    // is already declared/defined.

    // if found match, then return type must be the same, default args
    // must not be given, and function must not previously have been defined.
    // otherwise, all's well, this is a new declaration. Make sure the
    // declaration doesn't conflict (see below), and add the

    overload = astToOverloadSetElement(
        report, options, env, filename, fn->data.function.returnType,
        fn->data.function.formals->size,
        fn->data.function.formals->firstElements,
        fn->data.function.formals->thirdElements, true);
    if (overload == NULL) {
      return;
    }

    OverloadSetElement *matched = overloadSetLookupDefinition(
        &info->data.function.overloadSet, &overload->argumentTypes);

    if (matched == NULL) {
      // new declaration + definition: insert as if declared and defined
      OverloadSetElement *declMatched = overloadSetLookupCollision(
          &info->data.function.overloadSet, &overload->argumentTypes,
          overload->numOptional);
      if (declMatched == NULL) {
        overloadSetInsert(&info->data.function.overloadSet, overload);
      } else {
        // never an exact match here
        switch (optionsGet(options, optionWOverloadAmbiguity)) {
          case O_WT_ERROR: {
            reportError(report,
                        "%s:%zu:%zu: error: overload set allows ambiguous "
                        "calls through use of default arguments",
                        filename, fn->line, fn->character);
            overloadSetElementDestroy(overload);
            return;
          }
          case O_WT_WARN: {
            reportWarning(report,
                          "%s:%zu:%zu: warning: overload set allows ambiguous "
                          "calls through use of default arguments",
                          filename, fn->line, fn->character);
            break;
          }
          case O_WT_IGNORE: {
            break;
          }
        }
        overloadSetInsert(&info->data.function.overloadSet, overload);
      }
    } else if (matched->defined) {
      reportError(report, "%s:%zu:%zu: error: duplicate definition of '%s'",
                  filename, fn->line, fn->character, name->data.id.id);
      overloadSetElementDestroy(overload);
      return;
    } else if (!typeEqual(matched->returnType, overload->returnType)) {
      reportError(report,
                  "%s:%zu:%zu: error: return type of '%s' changed between "
                  "declaration and definition",
                  filename, fn->line, fn->character, name->data.id.id);
      overloadSetElementDestroy(overload);
      return;
    } else if (overload->numOptional != 0) {
      reportError(report,
                  "%s:%zu:%zu: error: may not redeclare default arguments in "
                  "function definition",
                  filename, fn->line, fn->character);
      overloadSetElementDestroy(overload);
      return;
    } else {
      overloadSetElementDestroy(overload);
      matched->defined = true;
      overload = matched;
    }
  }

  checkId(name, report, options, filename);
  name->data.id.symbol = info;
  name->data.id.overload = overload;

  environmentPush(env);
  NodeTripleList *formals = fn->data.function.formals;
  for (size_t idx = 0; idx < formals->size; idx++) {
    buildStabParameter(formals->firstElements[idx],
                       formals->secondElements[idx], report, options, env,
                       filename, moduleName);
  }
  buildStabStmt(fn->data.function.body, report, options, env, filename,
                moduleName);
  fn->data.function.localSymbols = environmentPop(env);
}
static void buildStabFnDecl(Node *fnDecl, Report *report,
                            Options const *options, Environment *env,
                            char const *filename, char const *moduleName) {
  // INVARIANT: env has no scopes
  // must not be declared as a non-function, must check if a function with the
  // same input args and name is declared/defined
  Node *name = fnDecl->data.fnDecl.id;
  SymbolInfo *info = symbolTableGet(env->currentModule, name->data.id.id);
  OverloadSetElement *overload;
  if (info != NULL && info->kind != SK_FUNCTION) {
    // already declared/defined as a non-function - error!
    reportError(report, "%s:%zu:%zu: error: '%s' already declared as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info == NULL) {
    // not declared/defined - do that now
    overload = astToOverloadSetElement(
        report, options, env, filename, fnDecl->data.fnDecl.returnType,
        fnDecl->data.fnDecl.params->size,
        fnDecl->data.fnDecl.params->firstElements,
        fnDecl->data.fnDecl.params->secondElements, false);
    if (overload == NULL) {
      return;
    }

    info = functionSymbolInfoCreate(moduleName);
    overloadSetInsert(&info->data.function.overloadSet, overload);
    symbolTablePut(env->currentModule, name->data.id.id, info);
  } else {
    // is already declared/defined.
    overload = astToOverloadSetElement(
        report, options, env, filename, fnDecl->data.fnDecl.returnType,
        fnDecl->data.fnDecl.params->size,
        fnDecl->data.fnDecl.params->firstElements,
        fnDecl->data.fnDecl.params->secondElements, false);
    if (overload == NULL) {
      return;
    }

    OverloadSetElement *matched = overloadSetLookupCollision(
        &info->data.function.overloadSet, &overload->argumentTypes,
        overload->numOptional);

    if (matched == NULL) {
      // new declaration
      overloadSetInsert(&info->data.function.overloadSet, overload);
    } else {
      bool allArgsSame =
          matched->argumentTypes.size == overload->argumentTypes.size;
      if (allArgsSame) {
        for (size_t idx = 0; idx < overload->argumentTypes.size; idx++) {
          if (!typeEqual(overload->argumentTypes.elements[idx],
                         matched->argumentTypes.elements[idx])) {
            allArgsSame = false;
            break;
          }
        }
      }
      bool exactMatch = matched->numOptional == overload->numOptional &&
                        typeEqual(matched->returnType, overload->returnType) &&
                        allArgsSame;

      // maybe a repeat, maybe a collision
      // if exact match, including return type -> repeat - check options for
      // error or no
      // else -> collision - if all argument types are different - check options
      // for error or no, else error - definite collision

      if (exactMatch) {
        overloadSetElementDestroy(overload);
        overload = matched;
        switch (optionsGet(options, optionWDuplicateDeclaration)) {
          case O_WT_ERROR: {
            reportError(
                report, "%s:%zu:%zu: error: duplicate declaration of '%s'",
                filename, fnDecl->line, fnDecl->character, name->data.id.id);
            return;
          }
          case O_WT_WARN: {
            reportWarning(
                report, "%s:%zu:%zu: warning: duplicate declaration of '%s'",
                filename, fnDecl->line, fnDecl->character, name->data.id.id);
            break;
          }
          case O_WT_IGNORE: {
            break;
          }
        }
      } else if (allArgsSame) {
        reportError(report,
                    "%s:%zu:%zu: error: return type or default argument "
                    "conflicts for duplciated declarations of '%s'",
                    filename, fnDecl->line, fnDecl->character,
                    name->data.id.id);
        overloadSetElementDestroy(overload);
        return;
      } else {
        switch (optionsGet(options, optionWOverloadAmbiguity)) {
          case O_WT_ERROR: {
            reportError(report,
                        "%s:%zu:%zu: error: overload set allows ambiguous "
                        "calls through use of default arguments",
                        filename, fnDecl->line, fnDecl->character);
            overloadSetElementDestroy(overload);
            return;
          }
          case O_WT_WARN: {
            reportWarning(report,
                          "%s:%zu:%zu: warning: overload set allows ambiguous "
                          "calls through use of default arguments",
                          filename, fnDecl->line, fnDecl->character);
            break;
          }
          case O_WT_IGNORE: {
            break;
          }
        }
        overloadSetInsert(&info->data.function.overloadSet, overload);
      }
    }
  }

  checkId(name, report, options, filename);
  name->data.id.symbol = info;
  name->data.id.overload = overload;
}
static void buildStabVarDecl(Node *varDecl, Report *report,
                             Options const *options, Environment *env,
                             char const *filename, char const *moduleName,
                             bool isDecl) {
  // must not allow a var with the same name to be defined/declared twice
  Type *varType =
      astToType(varDecl->data.varDecl.type, report, options, env, filename);
  bool escapes = typeIsComposite(varType);
  if (varType == NULL) {
    return;
  } else if (typeIsIncomplete(varType, env)) {
    reportError(report,
                "%s:%zu:%zu: error: variable of incomplete type declared",
                filename, varDecl->line, varDecl->character);
    return;
  }
  for (size_t nameIdx = 0; nameIdx < varDecl->data.varDecl.idValuePairs->size;
       nameIdx++) {
    Node *name = varDecl->data.varDecl.idValuePairs->firstElements[nameIdx];
    SymbolInfo *info = symbolTableGet(environmentTop(env), name->data.id.id);
    if (info != NULL && info->kind != SK_VAR) {
      // already declared as a non-var - error!
      reportError(report, "%s:%zu:%zu: error: '%s' already declared as %s",
                  filename, name->line, name->character, name->data.id.id,
                  symbolInfoToKindString(info));
      continue;
    } else if (info == NULL) {
      // new variable to declare
      info =
          varSymbolInfoCreate(moduleName, typeCopy(varType), !isDecl, escapes);
      symbolTablePut(environmentTop(env), name->data.id.id, info);
    } else {
      if (isDecl) {
        // redeclaration - check opts
        switch (optionsGet(options, optionWDuplicateDeclaration)) {
          case O_WT_ERROR: {
            reportError(
                report, "%s:%zu:%zu: error: duplciate declaration of '%s'",
                filename, name->line, name->character, name->data.id.id);
            continue;
          }
          case O_WT_WARN: {
            reportWarning(
                report, "%s:%zu:%zu: warning: duplciate declaration of '%s'",
                filename, name->line, name->character, name->data.id.id);
            continue;
          }
          case O_WT_IGNORE: {
            continue;
          }
        }
      } else {
        // binding declaration - only allowed if not previously bound
        if (info->data.var.bound) {
          reportError(report,
                      "%s:%zu:%zu: error: '%s' has already been declared",
                      filename, name->line, name->character, name->data.id.id);
          continue;
        }
      }
    }

    checkId(name, report, options, filename);
    name->data.id.symbol = info;
  }

  typeDestroy(varType);
  return;
}
static void buildStabStructOrUnionDecl(Node *decl, bool isStruct,
                                       Report *report, Options const *options,
                                       Environment *env, char const *filename,
                                       char const *moduleName) {
  // must not allow anything that isn't a struct with the same name to be
  // declared/defined, must not allow a struct with the same name to be defined
  Node *name = isStruct ? decl->data.structDecl.id : decl->data.unionDecl.id;
  SymbolInfo *info = symbolTableGet(environmentTop(env), name->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE ||
       info->data.type.kind != (isStruct ? TDK_STRUCT : TDK_UNION))) {
    // already declared as a non-struct - error!
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info == NULL) {
    // new struct declaration
    info = (isStruct ? structSymbolInfoCreate : unionSymbolInfoCreate)(
        moduleName, name->data.id.id);
    symbolTablePut(environmentTop(env), name->data.id.id, info);
  } else if (!(isStruct ? info->data.type.data.structType.incomplete
                        : info->data.type.data.unionType.incomplete)) {
    // already declared
    reportError(report, "%s:%zu:%zu: error: '%s' is already defined", filename,
                name->line, name->character, name->data.id.id);
    return;
  }

  NodeList *fields =
      isStruct ? decl->data.structDecl.decls : decl->data.unionDecl.opts;
  for (size_t declIdx = 0; declIdx < fields->size; declIdx++) {
    Node *field = fields->elements[declIdx];
    Type *actualFieldType =
        astToType(field->data.fieldDecl.type, report, options, env, filename);
    if (actualFieldType == NULL) {
      continue;
    } else if (typeIsIncomplete(actualFieldType, env)) {
      reportError(
          report,
          isStruct
              ? "%s:%zu:%zu: error: incomplete type not allowed in a struct"
              : "%s:%zu:%zu: error: incomplete type not allowed in a union",
          filename, field->data.fieldDecl.type->line,
          field->data.fieldDecl.type->character);
      continue;
    }
    for (size_t fieldIdx = 0; fieldIdx < field->data.fieldDecl.ids->size;
         fieldIdx++) {
      Node *id = field->data.fieldDecl.ids->elements[fieldIdx];
      typeVectorInsert(isStruct ? &info->data.type.data.structType.fields
                                : &info->data.type.data.unionType.fields,
                       typeCopy(actualFieldType));
      stringVectorInsert(isStruct ? &info->data.type.data.structType.names
                                  : &info->data.type.data.unionType.names,
                         id->data.id.id);
    }
  }

  checkId(name, report, options, filename);
  if (isStruct) {
    info->data.type.data.structType.incomplete = false;
  } else {
    info->data.type.data.unionType.incomplete = false;
  }
  name->data.id.symbol = info;
}
static void buildStabStructOrUnionForwardDecl(
    Node *forwardDecl, bool isStruct, Report *report, Options const *options,
    Environment *env, char const *filename, char const *moduleName) {
  // must not allow anything that isn't a struct with the same name to be
  // declared/defined, must check if a struct with the same name is
  // declared/defined
  Node *name = isStruct ? forwardDecl->data.structForwardDecl.id
                        : forwardDecl->data.unionForwardDecl.id;
  SymbolInfo *info = symbolTableGet(environmentTop(env), name->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE ||
       info->data.type.kind != (isStruct ? TDK_STRUCT : TDK_UNION))) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info != NULL) {
    switch (optionsGet(options, optionWDuplicateDeclaration)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: duplicate declaration of '%s'",
                    filename, name->line, name->character, name->data.id.id);
        return;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: duplicate declaration of '%s'",
                      filename, name->line, name->character, name->data.id.id);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  } else {
    info = (isStruct ? structSymbolInfoCreate : unionSymbolInfoCreate)(
        moduleName, name->data.id.id);
    symbolTablePut(environmentTop(env), name->data.id.id, info);
  }

  checkId(name, report, options, filename);
  name->data.id.symbol = info;
}
static void buildStabEnumDecl(Node *enumDecl, Report *report,
                              Options const *options, Environment *env,
                              char const *filename, char const *moduleName) {
  // must not allow anything that isn't an enum with the same name to be
  // declared/defined, must not allow an enum with the same name to be defined
  Node *name = enumDecl->data.enumDecl.id;
  SymbolInfo *info = symbolTableGet(environmentTop(env), name->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_ENUM)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info == NULL) {
    // new enum declaration
    info = enumSymbolInfoCreate(moduleName, name->data.id.id);
    symbolTablePut(environmentTop(env), name->data.id.id, info);
  } else if (!info->data.type.data.enumType.incomplete) {
    // already declared
    reportError(report, "%s:%zu:%zu: error: '%s' is already defined", filename,
                name->line, name->character, name->data.id.id);
    return;
  }

  NodeList *constants = enumDecl->data.enumDecl.elements;
  for (size_t idx = 0; idx < constants->size; idx++) {
    Node *constant = constants->elements[idx];
    stringVectorInsert(&info->data.type.data.enumType.fields,
                       constant->data.id.id);
  }

  checkId(name, report, options, filename);
  info->data.type.data.enumType.incomplete = false;
  name->data.id.symbol = info;
}
static void buildStabEnumForwardDecl(Node *enumForwardDecl, Report *report,
                                     Options const *options, Environment *env,
                                     char const *filename,
                                     char const *moduleName) {
  // must not allow anything that isn't an enum with the same name to be
  // declared/defined, must check if an enum with the same name is
  // declared/defined
  Node *name = enumForwardDecl->data.enumForwardDecl.id;
  SymbolInfo *info = symbolTableGet(environmentTop(env), name->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_ENUM)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info != NULL) {
    switch (optionsGet(options, optionWDuplicateDeclaration)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: duplicate declaration of '%s'",
                    filename, name->line, name->character, name->data.id.id);
        return;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: duplicate declaration of '%s'",
                      filename, name->line, name->character, name->data.id.id);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  } else {
    info = enumSymbolInfoCreate(moduleName, name->data.id.id);
    symbolTablePut(environmentTop(env), name->data.id.id, info);
  }

  checkId(name, report, options, filename);
  name->data.id.symbol = info;
}
static void buildStabTypedefDecl(Node *typedefDecl, Report *report,
                                 Options const *options, Environment *env,
                                 char const *filename, char const *moduleName) {
  Node *name = typedefDecl->data.typedefDecl.id;
  SymbolInfo *info = symbolTableGet(environmentTop(env), name->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_ENUM)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info != NULL) {
    // already exists - must be an error
    reportError(report, "%s:%zu:%zu: error: '%s' is already defined", filename,
                name->line, name->character, name->data.id.id);
    return;
  } else {
    Type *type = astToType(typedefDecl->data.typedefDecl.type, report, options,
                           env, filename);
    if (type == NULL) {
      return;
    }

    checkId(name, report, options, filename);
    info = typedefSymbolInfoCreate(moduleName, type, name->data.id.id);
    name->data.id.symbol = info;
    symbolTablePut(environmentTop(env), name->data.id.id, info);
  }
}
static void buildStabBody(Node *body, Report *report, Options const *options,
                          Environment *env, char const *filename,
                          char const *moduleName, bool isDecl) {
  switch (body->type) {
    case NT_FUNCTION: {
      buildStabFnDefn(body, report, options, env, filename, moduleName);
      return;
    }
    case NT_FNDECL: {
      buildStabFnDecl(body, report, options, env, filename, moduleName);
      return;
    }
    case NT_VARDECL: {
      buildStabVarDecl(body, report, options, env, filename, moduleName,
                       isDecl);
      return;
    }
    case NT_UNIONDECL:
    case NT_STRUCTDECL: {
      buildStabStructOrUnionDecl(body, body->type == NT_STRUCTDECL, report,
                                 options, env, filename, moduleName);
      return;
    }
    case NT_STRUCTFORWARDDECL:
    case NT_UNIONFORWARDDECL: {
      buildStabStructOrUnionForwardDecl(
          body, body->type == NT_STRUCTFORWARDDECL, report, options, env,
          filename, moduleName);
      return;
    }
    case NT_ENUMDECL: {
      buildStabEnumDecl(body, report, options, env, filename, moduleName);
      return;
    }
    case NT_ENUMFORWARDDECL: {
      buildStabEnumForwardDecl(body, report, options, env, filename,
                               moduleName);
      return;
    }
    case NT_TYPEDEFDECL: {
      buildStabTypedefDecl(body, report, options, env, filename, moduleName);
      return;
    }
    default: {
      return;  // not syntactically valid
    }
  }
}

// file level stuff
static void buildStabDecl(Node *ast, Report *report, Options const *options,
                          ModuleAstMap const *decls) {
  ast->data.file.symbols = symbolTableCreate();
  Environment env;
  environmentInit(&env, ast->data.file.symbols,
                  ast->data.file.module->data.module.id->data.id.id);

  // add all imports to env
  for (size_t idx = 0; idx < ast->data.file.imports->size; idx++) {
    Node *import = ast->data.file.imports->elements[idx];
    char const *importedId = import->data.import.id->data.id.id;

    Node *importedAst = moduleAstMapGet(decls, importedId);
    SymbolTable *importedTable = importedAst->data.file.symbols;
    if (importedTable == NULL) {
      buildStabDecl(importedAst, report, options, decls);
      importedTable = importedAst->data.file.symbols;
    }
    moduleTableMapPut(&env.imports, importedId, importedTable);
  }

  // traverse body
  for (size_t idx = 0; idx < ast->data.file.bodies->size; idx++) {
    Node *body = ast->data.file.bodies->elements[idx];
    buildStabBody(body, report, options, &env, ast->data.file.filename,
                  ast->data.file.module->data.module.id->data.id.id, true);
  }

  environmentUninit(&env);
}
static void buildStabCode(Node *ast, Report *report, Options const *options,
                          ModuleAstMap const *decls) {
  char const *moduleName = ast->data.file.module->data.module.id->data.id.id;
  ast->data.file.symbols =
      symbolTableCopy(moduleAstMapGet(decls, moduleName)->data.file.symbols);
  Environment env;
  environmentInit(&env, ast->data.file.symbols, moduleName);

  // add all imports to env
  for (size_t idx = 0; idx < ast->data.file.imports->size; idx++) {
    Node *import = ast->data.file.imports->elements[idx];
    char const *importedId = import->data.import.id->data.id.id;

    // import must have been typechecked to get here
    moduleTableMapPut(&env.imports, importedId,
                      moduleAstMapGet(decls, importedId)->data.file.symbols);
  }

  // traverse body
  for (size_t idx = 0; idx < ast->data.file.bodies->size; idx++) {
    Node *body = ast->data.file.bodies->elements[idx];
    buildStabBody(body, report, options, &env, ast->data.file.filename,
                  ast->data.file.module->data.module.id->data.id.id, false);
  }

  environmentUninit(&env);
}

void buildSymbolTables(Report *report, Options const *options,
                       ModuleAstMapPair const *asts) {
  for (size_t idx = 0; idx < asts->decls.size; idx++) {
    // if this is a decl, and it hasn't been processed
    if (asts->decls.keys[idx] != NULL &&
        ((Node *)asts->decls.values[idx])->data.file.symbols == NULL) {
      buildStabDecl(asts->decls.values[idx], report, options, &asts->decls);
    }
  }
  for (size_t idx = 0; idx < asts->codes.size; idx++) {
    // all codes that are valid haven't been processed by this loop.
    if (asts->codes.keys[idx] != NULL) {
      buildStabCode(asts->codes.values[idx], report, options, &asts->decls);
    }
  }
}