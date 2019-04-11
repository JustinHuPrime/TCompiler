// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Information attached to modules created during the dependency-finding phase

#include "util/moduleInfoTable.h"

#include <stdlib.h>

ModuleInfo *moduleInfoCreate(const char *moduleName, size_t numDependencies,
                             const char **dependencyNames) {
  ModuleInfo *info = malloc(sizeof(ModuleInfo));
  info->moduleName = moduleName;
  info->numDependencies = numDependencies;
  info->dependencyNames = dependencyNames;
  return info;
}
void moduleInfoDestroy(ModuleInfo *info) { free(info); }

ModuleInfoTable *moduleInfoTableCreate(void) { return hashMapCreate(); }
ModuleInfo *moduleInfoTableGet(ModuleInfoTable *table, char const *key) {
  return hashMapGet(table, key);
}
int moduleInfoTablePut(ModuleInfoTable *table, char const *key,
                       ModuleInfo *data) {
  return hashMapPut(table, key, data, (void (*)(void *))moduleInfoDestroy);
}
void moduleInfoTableDestroy(ModuleInfoTable *table) {
  hashMapDestroy(table, (void (*)(void *))moduleInfoDestroy);
}