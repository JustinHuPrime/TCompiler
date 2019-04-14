// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implemetation of parser utility tables

#include "parser/parseTables.h"
#include "dependencyGraph/grapher.h"

#include <stdlib.h>
#include <string.h>

static void typenameSetDestroy(void *ptr) { hashSetDestroy(ptr, false); }
TypenameSetTable *typenameSetTableCreate(void) { return hashMapCreate(); }
NonOwningHashSet *typenameSetTableGet(TypenameSetTable *table,
                                      char const *key) {
  return hashMapGet(table, key);
}
int typenameSetTablePut(TypenameSetTable *table, char const *key,
                        NonOwningHashSet *data) {
  return hashMapPut(table, key, data, typenameSetDestroy);
}
void typenameSetTableDestroy(TypenameSetTable *table) {
  hashMapDestroy(table, typenameSetDestroy);
}

Environment *environmentCreate(ModuleInfo *info) {
  Environment *env = malloc(sizeof(Environment));

  env->info = info;

  env->moduleOverrides = hashSetCreate();
  hashSetAdd(env->moduleOverrides, info->moduleName, false);
  for (size_t idx = 0; idx < info->numDependencies; idx++) {
    hashSetAdd(env->moduleOverrides, info->dependencyNames[idx], false);
  }

  env->typenames = hashSetCreate();

  env->overrideStackSize = 1;
  env->overrideStackCapacity = 1;
  env->overrideStack = malloc(sizeof(NonOwningHashSet *));
  env->overrideStack[0] = hashSetCreate();

  env->numImports = info->numDependencies;
  env->importedTypnames =
      malloc(env->numImports * sizeof(NonOwningHashSet const *));
  for (size_t idx = 0; idx < env->numImports; idx++) {
    env->importedTypnames[idx] = hashSetCreate();
  }

  return env;
}
void environmentPush(Environment *env) {
  if (env->overrideStackCapacity == env->overrideStackSize) {
    env->overrideStackCapacity *= 2;
    env->overrideStack =
        realloc(env->overrideStack,
                env->overrideStackCapacity * sizeof(NonOwningHashSet *));
  }
  env->overrideStack[env->overrideStackSize++] = hashSetCreate();
}
void environmentPop(Environment *env) {
  hashSetDestroy(env->overrideStack[--env->overrideStackSize], false);
}
bool environmentIsTypePlain(Environment *env, char const *name) {
  // note that idx is one more than the actual index
  for (size_t idx = env->overrideStackSize; idx > 0; idx--) {
    if (hashSetContains(env->overrideStack[idx - 1], name)) return false;
  }
  if (hashSetContains(env->typenames, name)) return true;
  for (size_t idx = 0; idx < env->numImports; idx++) {
    if (hashSetContains(env->importedTypnames[idx], name)) return true;
  }
  return false;
}
bool environmentIsTypeScoped(Environment *env, char const *name) {
  if (hashSetContains(env->moduleOverrides, name))
    return false;  // recoginzed module name

  char const *postfix = name;
  while (*postfix != ';') postfix--;
  postfix++;

  size_t prefixLength = strlen(name) - strlen(postfix) - 1;
  char *prefix = malloc(prefixLength);
  memcpy(prefix, name, prefixLength - 1);
  prefix[prefixLength - 1] = '\0';

  // if prefix matches current module's name
  if (strcmp(prefix, env->info->moduleName) == 0) {
    // prefix specifies current module
    for (size_t idx = env->overrideStackSize; idx > 0; idx--) {
      if (hashSetContains(env->overrideStack[idx - 1], name)) {
        free(prefix);
        return false;
      }
    }
    if (hashSetContains(env->typenames, name)) {
      free(prefix);
      return true;
    }
    free(prefix);
    return false;
  } else {
    for (size_t idx = 0; idx < env->info->numDependencies; idx++) {
      if (strcmp(prefix, env->info->dependencyNames[idx])) {
        free(prefix);
        return hashSetContains(env->importedTypnames[idx], name);
      }
    }
    return false;  // undefined ID!
  }
}
NonOwningHashSet *environmentDestroy(Environment *env) {
  hashSetDestroy(env->moduleOverrides, false);
  for (size_t idx = 0; idx < env->overrideStackSize; idx++) {
    hashSetDestroy(env->overrideStack[idx], false);
  }
  free(env->overrideStack);
  free(env->importedTypnames);

  return env->typenames;
}