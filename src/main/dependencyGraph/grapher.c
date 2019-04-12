// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Information attached to modules created during the dependency-finding phase

#include "dependencyGraph/grapher.h"

// these includes must be in this order
// clang-format off
#include "dependencyGraph/impl/parser.tab.h"
#include "dependencyGraph/impl/lex.yy.h"
// clang-format on

#include <stdlib.h>

ModuleInfo *moduleInfoCreate(bool isCode) {
  ModuleInfo *info = malloc(sizeof(ModuleInfo));
  info->isCode = isCode;
  info->numDependencies = 0;
  info->capacityDependencies = 1;
  info->dependencyNames = malloc(sizeof(char *));
  return info;
}
void moduleInfoAddDependency(ModuleInfo *info, char *name) {
  if (info->capacityDependencies == info->numDependencies) {
    info->capacityDependencies *= 2;
    info->dependencyNames = realloc(
        info->dependencyNames, info->capacityDependencies * sizeof(char *));
  }
  info->dependencyNames[info->numDependencies++] = name;
}
void moduleInfoDestroy(ModuleInfo *info) {
  free(info->moduleName);
  for (size_t idx = 0; idx < info->numDependencies; idx++) {
    free(info->dependencyNames[idx]);
  }
  free(info->dependencyNames);
  free(info);
}

ModuleInfoTable *moduleInfoTableCreate(Report *report, FileList *files) {
  ModuleInfoTable *table = hashMapCreate();

  // create table
  for (size_t idx = 0; idx < files->numDecl; idx++) {
    ModuleInfo *info = moduleInfoCreate(false);

    FILE *inFile = fopen(files->decls[idx], "r");
    if (inFile == NULL) {
      size_t messageLength = 26 + strlen(files->decls[idx]);
      char *message = malloc(messageLength);
      snprintf(message, messageLength, "%s: error: cannot open file",
               files->decls[idx]);
      reportError(report, message);
      continue;
    }

    yyscan_t scanner;
    dglex_init(&scanner);
    dgset_in(inFile, scanner);

    if (dgparse(scanner, info) == 1) {
      size_t messageLength = 39 + strlen(files->decls[idx]);
      char *message = malloc(messageLength);
      snprintf(message, messageLength,
               "%s: error: cannot parse file for modules", files->decls[idx]);
      reportError(report, message);

      dglex_destroy(scanner);
      fclose(inFile);
      continue;
    }

    dglex_destroy(scanner);
    fclose(inFile);

    moduleInfoTablePut(table, files->decls[idx], info);
  }

  // check for duplicated requires (warn)
  // check for circular dependency (error)

  return table;
}
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