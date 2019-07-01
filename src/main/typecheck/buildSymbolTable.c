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

#include <stdlib.h>

void moduleSymbolTableMapInit(ModuleSymbolTableMap *map) { hashMapInit(map); }
SymbolTable *moduleSymbolTableMapGet(ModuleSymbolTableMap const *map,
                                     char const *key) {
  return hashMapGet(map, key);
}
int moduleSymbolTableMapPut(ModuleSymbolTableMap *map, char const *key,
                            SymbolTable *value) {
  return hashMapPut(map, key, value, (void (*)(void *))symbolTableDestroy);
}
void moduleSymbolTableMapUninit(ModuleSymbolTableMap *map) {
  hashMapUninit(map, (void (*)(void *))symbolTableDestroy);
}

void moduleSymbolTableMapPairInit(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapInit(&pair->decls);
  moduleSymbolTableMapInit(&pair->codes);
}
void moduleSymbolTableMapPairUninit(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapUninit(&pair->decls);
  moduleSymbolTableMapUninit(&pair->codes);
}

void moduleEnvironmentMapInit(ModuleEnvironmentMap *map) { hashMapInit(map); }
Environment *moduleEnvironmentMapGet(ModuleEnvironmentMap *map,
                                     char const *key) {
  return hashMapGet(map, key);
}
int moduleEnvironmentMapPut(ModuleEnvironmentMap *map, char const *key,
                            Environment *value) {
  return hashMapPut(map, key, value, (void (*)(void *))environmentDestroy);
}
void moduleEnvironmentMapUninit(ModuleEnvironmentMap *map) {
  hashMapUninit(map, (void (*)(void *))environmentDestroy);
}

void moduleEnvronmentMapPairInit(ModuleEnvironmentMapPair *pair) {
  moduleEnvironmentMapInit(&pair->decls);
  moduleEnvironmentMapInit(&pair->codes);
}
void moduleEnvronmentMapPairUninit(ModuleEnvironmentMapPair *pair) {
  moduleEnvironmentMapUninit(&pair->decls);
  moduleEnvironmentMapUninit(&pair->codes);
}

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
static void buildStabFunction(Node const *function, Report *report,
                              Options const *options, Environment *env,
                              char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, function->data.function.id->data.id.id);
  // TODO: write this
}
static void buildStabFunDecl(Node const *funDecl, Report *report,
                             Options const *options, Environment *env,
                             char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, funDecl->data.funDecl.id->data.id.id);
  Type *returnType = astToType(funDecl->data.funDecl.returnType, report,
                               options, env, filename);
  if (returnType == NULL) {
    return;
  }

  if (info == NULL) {
    info = functionSymbolInfoCreate(returnType);
  } else {
    // check that returnType === info->data.function.returnType
    if (!typeEqual(returnType, info->data.function.returnType)) {
      // error - overload set w/ different return type
    }
  }
  // TODO: write this
}
static void buildStabVarDecl(Node const *varDecl, Report *report,
                             Options const *options, Environment *env,
                             char const *filename) {
  SymbolTable *table = environmentTop(env);
  bool errorReported = false;
  for (size_t idx = 0; idx < varDecl->data.varDecl.idValuePairs->size; idx++) {
    Node *id = varDecl->data.varDecl.idValuePairs->firstElements[idx];
    SymbolInfo *info = symbolTableGet(table, id->data.id.id);
    if (info != NULL) {
      reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                  filename, id->line, id->character, id->data.id.id,
                  symbolInfoToKindString(info));
      continue;
    }
    Type *type =
        astToType(varDecl->data.varDecl.type, report, options, env, filename);
    if (type == NULL) {
      return;
    } else if (typeIsIncomplete(type, env)) {
      if (!errorReported) {
        reportError(
            report,
            "%s:%zu:%zu: error: a variable may not have an incomplete type",
            filename, varDecl->line, varDecl->character);
        errorReported = true;
      }
      continue;
    }

    symbolTablePut(table, id->data.id.id, varSymbolInfoCreate(type));
  }
}
static void buildStabStructDecl(Node const *structDecl, Report *report,
                                Options const *options, Environment *env,
                                char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, structDecl->data.structDecl.id->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_STRUCT)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, structDecl->data.structDecl.id->line,
                structDecl->data.structDecl.id->character,
                structDecl->data.structDecl.id->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (!info->data.type.data.structType.incomplete) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already defined", filename,
                structDecl->data.structDecl.id->line,
                structDecl->data.structDecl.id->character,
                structDecl->data.structDecl.id->data.id.id);
    return;
  }

  if (info == NULL) {
    info = structSymbolInfoCreate();
    symbolTablePut(table, structDecl->data.structDecl.id->data.id.id, info);
  }

  bool bad = false;
  for (size_t declIdx = 0; declIdx < structDecl->data.structDecl.decls->size;
       declIdx++) {
    Node *fieldDecl = structDecl->data.structDecl.decls->elements[declIdx];
    for (size_t fieldIdx = 0; fieldIdx < fieldDecl->data.fieldDecl.ids->size;
         fieldIdx++) {
      Node const *id = fieldDecl->data.fieldDecl.ids->elements[fieldIdx];
      Type *actualFieldType = astToType(fieldDecl->data.fieldDecl.type, report,
                                        options, env, filename);
      if (actualFieldType == NULL) {
        bad = true;
        continue;
      } else if (typeIsIncomplete(actualFieldType, env)) {
        reportError(
            report,
            "%s:%zu:%zu: error: incomplete type not allowed in a struct",
            filename, fieldDecl->data.fieldDecl.type->line,
            fieldDecl->data.fieldDecl.type->character);
        typeDestroy(actualFieldType);
        bad = true;
        continue;
      }
      typeVectorInsert(&info->data.type.data.structType.fields,
                       actualFieldType);
      stringVectorInsert(&info->data.type.data.structType.names,
                         id->data.id.id);
    }
  }

  info->data.type.data.structType.incomplete = bad;
}
static void buildStabStructForwardDecl(Node const *forwardDecl, Report *report,
                                       Options const *options, Environment *env,
                                       char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, forwardDecl->data.structForwardDecl.id->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_STRUCT)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, forwardDecl->data.structForwardDecl.id->line,
                forwardDecl->data.structForwardDecl.id->character,
                forwardDecl->data.structForwardDecl.id->data.id.id,
                symbolInfoToKindString(info));
    return;
  }

  if (info != NULL) {
    switch (optionsGet(options, optionWDuplicateDeclaration)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: duplicate declaration of '%s'",
                    filename, forwardDecl->data.structForwardDecl.id->line,
                    forwardDecl->data.structForwardDecl.id->character,
                    forwardDecl->data.structForwardDecl.id->data.id.id);
        return;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: duplicate declaration of '%s'",
                      filename, forwardDecl->data.structForwardDecl.id->line,
                      forwardDecl->data.structForwardDecl.id->character,
                      forwardDecl->data.structForwardDecl.id->data.id.id);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  } else {
    symbolTablePut(table, forwardDecl->data.structForwardDecl.id->data.id.id,
                   structSymbolInfoCreate());
  }
}
static void buildStabUnionDecl(Node const *unionDecl, Report *report,
                               Options const *options, Environment *env,
                               char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, unionDecl->data.unionDecl.id->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_UNION)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, unionDecl->data.unionDecl.id->line,
                unionDecl->data.unionDecl.id->character,
                unionDecl->data.unionDecl.id->data.id.id,
                symbolInfoToKindString(info));
    return;
  } else if (!info->data.type.data.unionType.incomplete) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already defined", filename,
                unionDecl->data.unionDecl.id->line,
                unionDecl->data.unionDecl.id->character,
                unionDecl->data.unionDecl.id->data.id.id);
    return;
  }

  if (info == NULL) {
    info = unionSymbolInfoCreate();
    symbolTablePut(table, unionDecl->data.unionDecl.id->data.id.id, info);
  }

  bool bad = false;
  for (size_t declIdx = 0; declIdx < unionDecl->data.unionDecl.opts->size;
       declIdx++) {
    Node *fieldDecl = unionDecl->data.unionDecl.opts->elements[declIdx];
    for (size_t fieldIdx = 0; fieldIdx < fieldDecl->data.fieldDecl.ids->size;
         fieldIdx++) {
      Node const *id = fieldDecl->data.fieldDecl.ids->elements[fieldIdx];
      Type *actualFieldType = astToType(fieldDecl->data.fieldDecl.type, report,
                                        options, env, filename);
      if (actualFieldType == NULL) {
        bad = true;
        continue;
      } else if (typeIsIncomplete(actualFieldType, env)) {
        reportError(report,
                    "%s:%zu:%zu: error: incomplete type not allowed in a union",
                    filename, fieldDecl->data.fieldDecl.type->line,
                    fieldDecl->data.fieldDecl.type->character);
        typeDestroy(actualFieldType);
        bad = true;
        continue;
      }
      typeVectorInsert(&info->data.type.data.unionType.fields, actualFieldType);
      stringVectorInsert(&info->data.type.data.unionType.names, id->data.id.id);
    }
  }

  info->data.type.data.unionType.incomplete = bad;
}
static void buildStabUnionForwardDecl(Node const *forwardDecl, Report *report,
                                      Options const *options, Environment *env,
                                      char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, forwardDecl->data.unionForwardDecl.id->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_UNION)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, forwardDecl->data.unionForwardDecl.id->line,
                forwardDecl->data.unionForwardDecl.id->character,
                forwardDecl->data.unionForwardDecl.id->data.id.id,
                symbolInfoToKindString(info));
    return;
  }

  if (info != NULL) {
    switch (optionsGet(options, optionWDuplicateDeclaration)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: duplicate declaration of '%s'",
                    filename, forwardDecl->data.unionForwardDecl.id->line,
                    forwardDecl->data.unionForwardDecl.id->character,
                    forwardDecl->data.unionForwardDecl.id->data.id.id);
        return;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: duplicate declaration of '%s'",
                      filename, forwardDecl->data.unionForwardDecl.id->line,
                      forwardDecl->data.unionForwardDecl.id->character,
                      forwardDecl->data.unionForwardDecl.id->data.id.id);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  } else {
    symbolTablePut(table, forwardDecl->data.unionForwardDecl.id->data.id.id,
                   unionSymbolInfoCreate());
  }
}
static void buildStabEnumDecl(Node const *enumDecl, Report *report,
                              Options const *options, Environment *env,
                              char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, enumDecl->data.enumDecl.id->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_ENUM)) {
    reportError(
        report, "%s:%zu:%zu: error: '%s' is already declared as %s", filename,
        enumDecl->data.enumDecl.id->line, enumDecl->data.enumDecl.id->character,
        enumDecl->data.enumDecl.id->data.id.id, symbolInfoToKindString(info));
    return;
  } else if (!info->data.type.data.enumType.incomplete) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already defined", filename,
                enumDecl->data.enumDecl.id->line,
                enumDecl->data.enumDecl.id->character,
                enumDecl->data.enumDecl.id->data.id.id);
    return;
  }

  if (info == NULL) {
    info = enumSymbolInfoCreate();
    symbolTablePut(table, enumDecl->data.enumDecl.id->data.id.id, info);
  }

  for (size_t declIdx = 0; declIdx < enumDecl->data.enumDecl.elements->size;
       declIdx++) {
    Node *element = enumDecl->data.enumDecl.elements->elements[declIdx];
    stringVectorInsert(&info->data.type.data.enumType.fields,
                       element->data.id.id);
  }

  info->data.type.data.unionType.incomplete = false;
}
static void buildStabEnumForwardDecl(Node const *forwardDecl, Report *report,
                                     Options const *options, Environment *env,
                                     char const *filename) {
  SymbolTable *table = environmentTop(env);
  SymbolInfo *info =
      symbolTableGet(table, forwardDecl->data.enumForwardDecl.id->data.id.id);
  if (info != NULL &&
      (info->kind != SK_TYPE || info->data.type.kind != TDK_ENUM)) {
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, forwardDecl->data.enumForwardDecl.id->line,
                forwardDecl->data.enumForwardDecl.id->character,
                forwardDecl->data.enumForwardDecl.id->data.id.id,
                symbolInfoToKindString(info));
    return;
  }

  if (info != NULL) {
    switch (optionsGet(options, optionWDuplicateDeclaration)) {
      case O_WT_ERROR: {
        reportError(report, "%s:%zu:%zu: error: duplicate declaration of '%s'",
                    filename, forwardDecl->data.enumForwardDecl.id->line,
                    forwardDecl->data.enumForwardDecl.id->character,
                    forwardDecl->data.enumForwardDecl.id->data.id.id);
        return;
      }
      case O_WT_WARN: {
        reportWarning(report,
                      "%s:%zu:%zu: warning: duplicate declaration of '%s'",
                      filename, forwardDecl->data.enumForwardDecl.id->line,
                      forwardDecl->data.enumForwardDecl.id->character,
                      forwardDecl->data.enumForwardDecl.id->data.id.id);
        break;
      }
      case O_WT_IGNORE: {
        break;
      }
    }
  } else {
    symbolTablePut(table, forwardDecl->data.enumForwardDecl.id->data.id.id,
                   enumSymbolInfoCreate());
  }
}
static void buildStabTypedef(Node const *typedefDecl, Report *report,
                             Options const *options, Environment *env,
                             char const *filename) {
  SymbolTable *table = environmentTop(env);

  SymbolInfo *info =
      symbolTableGet(table, typedefDecl->data.typedefDecl.id->data.id.id);
  if (info != NULL) {
    // already exists - must be an error
    reportError(report, "%s:%zu:%zu: error: '%s' is already declared as %s",
                filename, typedefDecl->data.typedefDecl.id->line,
                typedefDecl->data.typedefDecl.id->character,
                typedefDecl->data.typedefDecl.id->data.id.id,
                symbolInfoToKindString(info));
    return;
  }

  Type *type = astToType(typedefDecl->data.typedefDecl.type, report, options,
                         env, filename);
  if (type != NULL) {
    symbolTablePut(table, typedefDecl->data.typedefDecl.id->data.id.id,
                   typedefSymbolInfoCreate(type));
  }
}
static void buildStabBody(Node const *body, Report *report,
                          Options const *options, Environment *env,
                          char const *filename) {
  switch (body->type) {
    case NT_FUNCTION: {
      buildStabFunction(body, report, options, env, filename);
      break;
    }
    case NT_FUNDECL: {
      buildStabFunDecl(body, report, options, env, filename);
      break;
    }
    case NT_VARDECL: {
      buildStabVarDecl(body, report, options, env, filename);
      break;
    }
    case NT_STRUCTDECL: {
      buildStabStructDecl(body, report, options, env, filename);
      break;
    }
    case NT_STRUCTFORWARDDECL: {
      buildStabStructForwardDecl(body, report, options, env, filename);
      break;
    }
    case NT_UNIONDECL: {
      buildStabUnionDecl(body, report, options, env, filename);
      break;
    }
    case NT_UNIONFORWARDDECL: {
      buildStabUnionForwardDecl(body, report, options, env, filename);
      break;
    }
    case NT_ENUMDECL: {
      buildStabEnumDecl(body, report, options, env, filename);
      break;
    }
    case NT_ENUMFORWARDDECL: {
      buildStabEnumForwardDecl(body, report, options, env, filename);
      break;
    }
    case NT_TYPEDEFDECL: {
      buildStabTypedef(body, report, options, env, filename);
      break;
    }
    default: {
      return;
      // error: not syntactically valid
    }
  }
}

// file level stuff
static Environment *buildStabDecl(ModuleSymbolTableMap *declStabs,
                                  Report *report, Options const *options,
                                  ModuleAstMap const *asts, Node const *ast) {
  SymbolTable *currStab = symbolTableCreate();
  char const *currModule = ast->data.file.module->data.module.id->data.id.id;
  Environment *env = environmentCreate(currStab, currModule);

  for (size_t idx = 0; idx < ast->data.file.imports->size; idx++) {
    Node *import = ast->data.file.imports->elements[idx];
    char const *importedId = import->data.import.id->data.id.id;

    SymbolTable *importedTable = moduleSymbolTableMapGet(declStabs, importedId);
    if (importedTable == NULL) {
      buildStabDecl(declStabs, report, options, asts,
                    moduleAstMapGet(asts, importedId));
      importedTable = moduleSymbolTableMapGet(declStabs, importedId);
    }
    moduleSymbolTableMapPut(&env->imports, importedId, importedTable);
  }

  for (size_t idx = 0; idx < ast->data.file.bodies->size; idx++) {
    Node *body = ast->data.file.bodies->elements[idx];
    buildStabBody(body, report, options, env, ast->data.file.filename);
  }

  moduleSymbolTableMapPut(declStabs, currModule, currStab);
  return env;
}
static Environment *buildStabCode(ModuleSymbolTableMapPair *stabs,
                                  Report *report, Options const *options,
                                  Node const *ast) {
  SymbolTable *currStab = symbolTableCreate();
  char const *currModule = ast->data.file.module->data.module.id->data.id.id;
  Environment *env = environmentCreate(currStab, currModule);

  for (size_t idx = 0; idx < ast->data.file.imports->size; idx++) {
    Node *import = ast->data.file.imports->elements[idx];
    moduleSymbolTableMapPut(
        &env->imports, import->data.import.id->data.id.id,
        moduleSymbolTableMapGet(&stabs->decls,
                                import->data.import.id->data.id.id));
  }

  for (size_t idx = 0; idx < ast->data.file.bodies->size; idx++) {
    Node *body = ast->data.file.bodies->elements[idx];
    buildStabBody(body, report, options, env, ast->data.file.filename);
  }

  moduleSymbolTableMapPut(&stabs->codes, currModule, currStab);
  return env;
}

void buildSymbolTables(ModuleSymbolTableMapPair *stabs,
                       ModuleEnvironmentMapPair *envs, Report *report,
                       Options const *options, ModuleAstMapPair const *asts) {
  moduleSymbolTableMapPairInit(stabs);

  for (size_t idx = 0; idx < asts->decls.size; idx++) {
    if (asts->decls.keys[idx] != NULL) {
      Node *ast = asts->decls.values[idx];
      moduleEnvironmentMapPut(
          &envs->decls, ast->data.file.module->data.module.id->data.id.id,
          buildStabDecl(&stabs->decls, report, options, &asts->decls, ast));
    }
  }
  for (size_t idx = 0; idx < asts->codes.size; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *ast = asts->decls.values[idx];
      moduleEnvironmentMapPut(&envs->decls,
                              ast->data.file.module->data.module.id->data.id.id,
                              buildStabCode(stabs, report, options, ast));
    }
  }
}