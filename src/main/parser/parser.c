// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of the parser interface and utilities

#include "parser/parser.h"

#include "parser/parseTables.h"
#include "util/hashSet.h"

// these includes must be in this order
// clang-format off
#include "parser/impl/parser.tab.h"
#include "parser/impl/lex.yy.h"
// clang-format on

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

static void parseDeclFile(Report *report, Options *options, ModuleInfo *info,
                          ModuleInfoTable *infoTable,
                          TypenameSetTable *typenameTable,
                          ModuleNodeTable *nodeTable) {
  // set up environment
  Environment *env = environmentCreate(info);
  for (size_t idx = 0; idx < info->numDependencies; idx++) {
    // parse dependencies, skipping over any that are already done
    NonOwningHashSet *typenames =
        typenameSetTableGet(typenameTable, info->dependencyNames[idx]);
    if (typenames != NULL) {
      // exists, don't re-do it, just add it to the environment
      env->importedTypnames[idx] = typenames;
    } else {
      // does not exist - parse it recursively
      ModuleInfo *dependencyInfo =
          moduleInfoTableGet(infoTable, info->dependencyNames[idx]);
      parseDeclFile(report, options, dependencyInfo, infoTable, typenameTable,
                    nodeTable);
      env->importedTypnames[idx] =
          typenameSetTableGet(typenameTable, info->dependencyNames[idx]);
    }
  }

  // guarenteed to open - d-graph already opened it once
  FILE *inFile = fopen(info->fileName, "r");
  yyscan_t scanner;
  astlex_init(&scanner);
  astset_in(inFile, scanner);
  astset_extra(env, scanner);

  Node *ast;
  int retVal = astparse(scanner, &ast, env);

  astlex_destroy(scanner);
  fclose(inFile);

  typenameSetTablePut(typenameTable, info->moduleName, environmentDestroy(env));
}
static void parseCodeFile(Report *report, Options *options,
                          char const *filename,
                          TypenameSetTable *typenameTable) {}

ModuleNodeTablePair *parseFiles(Report *report, Options *options,
                                FileList *files, ModuleInfoTable *infoTable) {
  TypenameSetTable *typenames = typenameSetTableCreate();
  ModuleNodeTablePair *pair = moduleNodeTablePairCreate();

  for (size_t idx = 0; idx < infoTable->size; idx++) {
    if (infoTable->keys[idx] == NULL) continue;
    parseDeclFile(report, options, infoTable->values[idx], infoTable, typenames,
                  pair->decls);
  }

  typenameSetTableDestroy(typenames);
  return pair;
}