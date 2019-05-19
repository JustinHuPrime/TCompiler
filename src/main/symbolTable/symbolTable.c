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

// Implementation of the symbol table

#include "symbolTable/symbolTable.h"

#include <stdlib.h>

SymbolInfo *symbolInfoCreate(void) { return malloc(sizeof(SymbolInfo)); }
void symbolInfoDestroy(SymbolInfo *si) {
  switch (si->kind) {
    case SK_VAR: {
      break;
    }
    case SK_TYPE: {
      break;
    }
    case SK_FUNCTION: {
      break;
    }
  }
}

SymbolTable *symbolTableCreate(void) { return hashMapCreate(); }
SymbolInfo *symbolTableGet(SymbolTable const *table, char const *key) {
  return hashMapGet(table, key);
}
int symbolTablePut(SymbolTable *table, char const *key, SymbolInfo *value) {
  return hashMapPut(table, key, value, (void (*)(void *))symbolInfoDestroy);
}
void symbolTableDestroy(SymbolTable *table) {
  hashMapDestroy(table, (void (*)(void *))symbolInfoDestroy);
}

Environment *environmentCreate(SymbolTable *currentModule) {
  Environment *env = malloc(sizeof(Environment));
  env->currentModule = currentModule;
  env->imports = vectorCreate();
  env->scopes = stackCreate();
  return env;
}
SymbolInfo *environmentLookup(Environment const *env, char const *name) {
  // TODO: something
}
void environmentDestroy(Environment *env) {
  symbolTableDestroy(env->currentModule);
  vectorDestroy(env->imports, (void (*)(void *))symbolTableDestroy);
  stackDestroy(env->scopes, (void (*)(void *))symbolTableDestroy);
  free(env);
}