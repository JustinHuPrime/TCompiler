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

#include "typecheck/buildSymbolTable.h"

// helpers
static Type *astToType(Node const *ast, Report *report, Options const *options,
                       Environment const *env, char const *filename) {
  switch (ast->type) {
    case NT_KEYWORDTYPE: {
      return keywordTypeCreate(ast->data.typeKeyword.type - TK_VOID + K_VOID);
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
      return subType == NULL ? NULL : modifierTypeCreate(K_PTR, subType);
    }
    case NT_ARRAYTYPE: {
      Node const *sizeConst = ast->data.arrayType.size;
      switch (sizeConst->data.constExp.type) {
        case CT_UBYTE:
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
      Type *subType = astToType(ast->data.arrayType.element, report, options,
                                env, filename);
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

// expression

// statement

// top level
static OverloadSetElement *astToOverloadSetElement(
    Report *report, Options const *options, Environment const *env,
    char const *filename, Node const *returnTypeNode, size_t numArgs,
    Node **argTypes, Node **argDefaults, bool defined) {
  OverloadSetElement *overload = overloadSetElementCreate();

  Type *returnType = astToType(returnTypeNode, report, options, env, filename);
  if (returnType == NULL) {
    overloadSetElementDestroy(overload);
    return NULL;
  }
  overload->returnType = returnType;

  for (size_t idx = 0; idx < numArgs; idx++) {
    Type *argType = astToType(argTypes[idx], report, options, env, filename);
    if (argType == NULL) {
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
static void buildStabFunDefn(Node *fn, Report *report, Options const *options,
                             Environment *env, char const *filename) {
  // INVARIANT: env has no scopes
  // must not be declared/defined as a non-function, must not allow a function
  // with the same input args and name to be declared/defined
  Node *name = fn->data.function.id;
  SymbolInfo *info = symbolTableGet(env->currentModule, name->data.id.id);
  if (info != NULL && info->kind != SK_FUNCTION) {
    // already declared/defined as a non-function - error!
    reportError(report, "%s:%zu:%zu: error: '%s' already defined as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info == NULL) {
    // not declared/defined - do that now
    OverloadSetElement *overload = astToOverloadSetElement(
        report, options, env, filename, fn->data.function.returnType,
        fn->data.function.formals->size,
        fn->data.function.formals->firstElements,
        fn->data.function.formals->thirdElements, true);
    if (overload == NULL) {
      return;
    }

    info = functionSymbolInfoCreate();
    overloadSetInsert(&info->data.function.overloadSet, overload);
    symbolTablePut(env->currentModule, name->data.id.id, info);
  } else {
    // is already declared/defined.

    // if found match, then return type must be the same, default args
    // must not be given, and function must not previously have been defined.
    // otherwise, all's well, this is a new declaration. Make sure the
    // declaration doesn't conflict (see below), and add the

    OverloadSetElement *overload = astToOverloadSetElement(
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
    }
  }

  name->data.id.symbol = info;

  environmentPush(env);
  // TODO: add local params
  // TODO: check inside of function
  fn->data.function.localSymbols = environmentPop(env);
}
static void buildStabFunDecl(Node *fnDecl, Report *report,
                             Options const *options, Environment *env,
                             char const *filename) {
  // INVARIANT: env has no scopes
  // must not be declared as a non-function, must check if a function with the
  // same input args and name is declared/defined
  Node *name = fnDecl->data.funDecl.id;
  SymbolInfo *info = symbolTableGet(env->currentModule, name->data.id.id);
  if (info != NULL && info->kind != SK_FUNCTION) {
    // already declared/defined as a non-function - error!
    reportError(report, "%s:%zu:%zu: error: '%s' already defined as %s",
                filename, name->line, name->character, name->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (info == NULL) {
    // not declared/defined - do that now
    OverloadSetElement *overload = astToOverloadSetElement(
        report, options, env, filename, fnDecl->data.funDecl.returnType,
        fnDecl->data.funDecl.params->size,
        fnDecl->data.funDecl.params->firstElements,
        fnDecl->data.funDecl.params->secondElements, false);
    if (overload == NULL) {
      return;
    }

    info = functionSymbolInfoCreate();
    overloadSetInsert(&info->data.function.overloadSet, overload);
    symbolTablePut(env->currentModule, name->data.id.id, info);
  } else {
    // is already declared/defined.
    OverloadSetElement *overload = astToOverloadSetElement(
        report, options, env, filename, fnDecl->data.funDecl.returnType,
        fnDecl->data.funDecl.params->size,
        fnDecl->data.funDecl.params->firstElements,
        fnDecl->data.funDecl.params->secondElements, false);
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
        switch (optionsGet(options, optionWDuplicateDeclaration)) {
          case O_WT_ERROR: {
            reportError(
                report, "%s:%zu:%zu: error: duplicate definition of '%s'",
                filename, fnDecl->line, fnDecl->character, name->data.id.id);
            overloadSetElementDestroy(overload);
            return;
          }
          case O_WT_WARN: {
            reportWarning(
                report, "%s:%zu:%zu: warning: duplicate definition of '%s'",
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

  name->data.id.symbol = info;
}
static void buildStabVarDecl(Node *varDecl, Report *report,
                             Options const *options, Environment *env,
                             char const *filename) {
  // mu
  // must not allow a var with the same name to be defined/declared twice
  // TODO: write this
}
static void buildStabStructDecl(Node *structDecl, Report *report,
                                Options const *options, Environment *env,
                                char const *filename) {
  // must not allow anything that isn't a struct with the same name to be
  // declared/defined, must not allow a struct with the same name to be defined
  // TODO: write this
}
static void buildStabStructForwardDecl(Node *structForwardDecl, Report *report,
                                       Options const *options, Environment *env,
                                       char const *filename) {
  // must not allow anything that isn't a struct with the same name to be
  // declared/defined, must check if a struct with the same name is
  // declared/defined
  // TODO: write this
}
static void buildStabUnionDecl(Node *unionDecl, Report *report,
                               Options const *options, Environment *env,
                               char const *filename) {
  // must not allow anything that isn't a struct with the same name to be
  // declared/defined, must not allow a union with the same name to be defined
  // TODO: write this
}
static void buildStabUnionForwardDecl(Node *unionForwardDecl, Report *report,
                                      Options const *options, Environment *env,
                                      char const *filename) {
  // must not allow anything that isn't a struct with the same name to be
  // declared/defined, must check if a struct with the same name is
  // declared/defined
  // TODO: write this
}
static void buildStabEnumDecl(Node *enumDecl, Report *report,
                              Options const *options, Environment *env,
                              char const *filename) {
  // must not allow anything that isn't an enum with the same name to be
  // declared/defined, must not allow an enum with the same name to be defined
  // TODO: write this
}
static void buildStabEnumForwardDecl(Node *enumForwardDecl, Report *report,
                                     Options const *options, Environment *env,
                                     char const *filename) {
  // must not allow anything that isn't an enum with the same name to be
  // declared/defined, must check if an enum with the same name is
  // declared/defined
  // TODO: write this
}
static void buildStabTypeDefDecl(Node *typedefDecl, Report *report,
                                 Options const *options, Environment *env,
                                 char const *filename) {
  // TODO: write this
}
static void buildStabBody(Node *body, Report *report, Options const *options,
                          Environment *env, char const *filename, bool isDecl) {
  switch (body->type) {
    case NT_FUNCTION: {
      buildStabFunDefn(body, report, options, env, filename);
      return;
    }
    case NT_FUNDECL: {
      buildStabFunDecl(body, report, options, env, filename);
      return;
    }
    case NT_VARDECL: {
      buildStabVarDecl(body, report, options, env, filename);
      return;
    }
    case NT_STRUCTDECL: {
      buildStabStructDecl(body, report, options, env, filename);
      return;
    }
    case NT_STRUCTFORWARDDECL: {
      buildStabStructForwardDecl(body, report, options, env, filename);
      return;
    }
    case NT_UNIONDECL: {
      buildStabUnionDecl(body, report, options, env, filename);
      return;
    }
    case NT_UNIONFORWARDDECL: {
      buildStabUnionForwardDecl(body, report, options, env, filename);
      return;
    }
    case NT_ENUMDECL: {
      buildStabEnumDecl(body, report, options, env, filename);
      return;
    }
    case NT_ENUMFORWARDDECL: {
      buildStabEnumForwardDecl(body, report, options, env, filename);
      return;
    }
    case NT_TYPEDEFDECL: {
      buildStabTypeDefDecl(body, report, options, env, filename);
      return;
    }
    default: {
      return;  // not syntactically valid - caught at parse time
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
    buildStabBody(body, report, options, &env, ast->data.file.filename, true);
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
    buildStabBody(body, report, options, &env, ast->data.file.filename, false);
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
      buildStabCode(asts->decls.values[idx], report, options, &asts->decls);
    }
  }
}