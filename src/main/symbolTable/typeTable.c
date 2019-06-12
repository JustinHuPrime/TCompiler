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

// Implementation of the type table

#include "symbolTable/typeTable.h"

#include "util/functional.h"
#include "util/nameUtils.h"

TypeTable *typeTableCreate(void) { return hashMapCreate(); }
SymbolType typeTableGet(TypeTable const *table, char const *key) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbad-function-cast"
  return (intptr_t)hashMapGet(table, key);
#pragma GCC diagnostic pop
}
void typeTableSet(TypeTable *table, char const *key, SymbolType value) {
  hashMapSet(table, key, (void *)value);
}
void typeTableDestroy(TypeTable *table) { hashMapDestroy(table, nullDtor); }

ModuleTypeTableMap *moduleTypeTableMapCreate(void) { return hashMapCreate(); }
void moduleTypeTableMapInit(ModuleTypeTableMap *map) { hashMapInit(map); }
TypeTable *moduleTypeTableMapGet(ModuleTypeTableMap const *map,
                                 char const *key) {
  return hashMapGet(map, key);
}
int moduleTypeTableMapPut(ModuleTypeTableMap *map, char const *key,
                          TypeTable *value) {
  return hashMapPut(map, key, value, (void (*)(void *))typeTableDestroy);
}
void moduleTypeTableMapUninit(ModuleTypeTableMap *map) {
  hashMapUninit(map, (void (*)(void *))typeTableDestroy);
}
void moduleTypeTableMapDestroy(ModuleTypeTableMap *map) {
  hashMapDestroy(map, (void (*)(void *))typeTableDestroy);
}

void typeEnvironmentInit(TypeEnvironment *env, TypeTable *currentModule,
                         char const *currentModuleName) {
  env->currentModule = currentModule;
  env->currentModuleName = currentModuleName;
  moduleTypeTableMapInit(&env->imports);
  stackInit(&env->scopes);
}
TernaryValue typeEnvironmentIsType(TypeEnvironment const *env, Report *report,
                                   TokenInfo const *token,
                                   char const *filename) {
  // TODO: write this
}
void typeEnvironmentUninit(TypeEnvironment *env) {
  moduleTypeTableMapUninit(&env->imports);
  stackUninit(&env->scopes, (void (*)(void *))typeTableDestroy);
}