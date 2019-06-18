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

ModuleSymbolTableMap *moduleSymbolTableMapCreate(void) {
  return hashMapCreate();
}
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
void moduleSymbolTableMapDestroy(ModuleSymbolTableMap *map) {
  hashMapDestroy(map, (void (*)(void *))symbolTableDestroy);
}

ModuleSymbolTableMapPair *moduleSymbolTableMapPairCreate(void) {
  ModuleSymbolTableMapPair *pair = malloc(sizeof(ModuleSymbolTableMapPair));
  moduleSymbolTableMapPairInit(pair);
  return pair;
}
void moduleSymbolTableMapPairInit(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapInit(&pair->decls);
  moduleSymbolTableMapInit(&pair->codes);
}
void moduleSymbolTableMapPairUninit(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapUninit(&pair->decls);
  moduleSymbolTableMapUninit(&pair->codes);
}
void moduleSymbolTableMapPairDestroy(ModuleSymbolTableMapPair *pair) {
  moduleSymbolTableMapPairUninit(pair);
  free(pair);
}

void typecheck(ModuleSymbolTableMapPair *stabs, Report *report,
               Options const *options, ModuleAstMapPair const *asts) {
  moduleSymbolTableMapPairInit(stabs);
}