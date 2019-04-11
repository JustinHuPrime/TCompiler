// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of module-node tables

#include "parser/moduleNodeTable.h"

#include <stdlib.h>

ModuleNodeTable *moduleNodeTableCreate(void) { return hashMapCreate(); }
Node *moduleNodeTableGet(ModuleNodeTable *table, char const *key) {
  return hashMapGet(table, key);
}
int moduleNodeTablePut(ModuleNodeTable *table, char const *key, Node *data) {
  return hashMapPut(table, key, data, (void (*)(void *))nodeDestroy);
}
void moduleNodeTableDestroy(ModuleNodeTable *table) {
  hashMapDestroy(table, (void (*)(void *))nodeDestroy);
}

ModuleNodeTablePair *moduleNodeTablePairCreate(void) {
  ModuleNodeTablePair *pair = malloc(sizeof(ModuleNodeTablePair));
  pair->decls = moduleNodeTableCreate();
  pair->codes = moduleNodeTableCreate();
  return pair;
}
void moduleNodeTablePairDestroy(ModuleNodeTablePair *pair) {
  moduleNodeTableDestroy(pair->decls);
  moduleNodeTableDestroy(pair->codes);
  free(pair);
}