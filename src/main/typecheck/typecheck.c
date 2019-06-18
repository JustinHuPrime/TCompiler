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

static void typecheckFunction(Node const *function, Report *report,
                              Options const *options, Environment *env) {
  // TODO: write this
}
static void typecheckVarDecl(Node const *varDecl, Report *report,
                             Options const *options, Environment *env) {
  // TODO: write this
}
static void typecheckStructDecl(Node const *structDecl, Report *report,
                                Options const *options, Environment *env) {
  // TODO: write this
}
static void typecheckStructForwardDecl(Node const *forwardDecl, Report *report,
                                       Options const *options,
                                       Environment *env) {
  // TODO: write this
}
static void typecheckUnionDecl(Node const *unionDecl, Report *report,
                               Options const *options, Environment *env) {
  // TODO: write this
}
static void typecheckUnionForwardDecl(Node const *forwardDecl, Report *report,
                                      Options const *options,
                                      Environment *env) {
  // TODO: write this
}
static void typecheckEnumDecl(Node const *enumDecl, Report *report,
                              Options const *options, Environment *env) {
  // TODO: write this
}
static void typecheckEnumForwardDecl(Node const *forwardDecl, Report *report,
                                     Options const *options, Environment *env) {
  // TODO: write this
}
static void typecheckTypedef(Node const *typedefDecl, Report *report,
                             Options const *options, Environment *env) {
  // TODO: write this
}
static void typecheckBody(Node const *body, Report *report,
                          Options const *options, Environment *env) {
  switch (body->type) {
    case NT_FUNCTION: {
      typecheckFunction(body, report, options, env);
      break;
    }
    case NT_VARDECL: {
      typecheckVarDecl(body, report, options, env);
      break;
    }
    case NT_STRUCTDECL: {
      typecheckStructDecl(body, report, options, env);
      break;
    }
    case NT_STRUCTFORWARDDECL: {
      typecheckStructForwardDecl(body, report, options, env);
      break;
    }
    case NT_UNIONDECL: {
      typecheckUnionDecl(body, report, options, env);
      break;
    }
    case NT_UNIONFORWARDDECL: {
      typecheckUnionForwardDecl(body, report, options, env);
      break;
    }
    case NT_ENUMDECL: {
      typecheckEnumDecl(body, report, options, env);
      break;
    }
    case NT_ENUMFORWARDDECL: {
      typecheckEnumForwardDecl(body, report, options, env);
      break;
    }
    case NT_TYPEDEFDECL: {
      typecheckTypedef(body, report, options, env);
      break;
    }
    default: {
      return;
      // error: not syntactically valid
    }
  }
}

// file level stuff
static Environment *typecheckDecl(ModuleSymbolTableMap *declStabs,
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
      typecheckDecl(declStabs, report, options, asts,
                    moduleAstMapGet(asts, importedId));
      importedTable = moduleSymbolTableMapGet(declStabs, importedId);
    }
    moduleSymbolTableMapPut(&env->imports, importedId, importedTable);
  }

  for (size_t idx = 0; idx < ast->data.file.bodies->size; idx++) {
    Node *body = ast->data.file.bodies->elements[idx];
    typecheckBody(body, report, options, env);
  }

  moduleSymbolTableMapPut(declStabs, currModule, currStab);
  return env;
}
static Environment *typecheckCode(ModuleSymbolTableMapPair *stabs,
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
    typecheckBody(body, report, options, env);
  }

  moduleSymbolTableMapPut(&stabs->codes, currModule, currStab);
  return env;
}

void typecheck(ModuleSymbolTableMapPair *stabs, ModuleEnvironmentMapPair *envs,
               Report *report, Options const *options,
               ModuleAstMapPair const *asts) {
  moduleSymbolTableMapPairInit(stabs);

  for (size_t idx = 0; idx < asts->decls.size; idx++) {
    if (asts->decls.keys[idx] != NULL) {
      Node *ast = asts->decls.values[idx];
      moduleEnvironmentMapPut(
          &envs->decls, ast->data.file.module->data.module.id->data.id.id,
          typecheckDecl(&stabs->decls, report, options, &asts->decls, ast));
    }
  }
  for (size_t idx = 0; idx < asts->codes.size; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *ast = asts->decls.values[idx];
      moduleEnvironmentMapPut(&envs->decls,
                              ast->data.file.module->data.module.id->data.id.id,
                              typecheckCode(stabs, report, options, ast));
    }
  }
}