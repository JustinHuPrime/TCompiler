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

#include "util/format.h"
#include "util/nameUtils.h"

#include <stdlib.h>
#include <string.h>

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

ModuleTableMap *moduleTableMapCreate(void) { return hashMapCreate(); }
SymbolTable *moduleTableMapGet(ModuleTableMap const *table, char const *key) {
  return hashMapGet(table, key);
}
int moduleTableMapPut(ModuleTableMap *table, char const *key,
                      SymbolTable *value) {
  return hashMapPut(table, key, value, (void (*)(void *))symbolTableDestroy);
}
void moduleTableMapDestroy(ModuleTableMap *table) {
  hashMapDestroy(table, (void (*)(void *))symbolTableDestroy);
}

Environment *environmentCreate(SymbolTable *currentModule,
                               char const *currentModuleName) {
  Environment *env = malloc(sizeof(Environment));
  env->currentModule = currentModule;
  env->currentModuleName = currentModuleName;
  env->imports = moduleTableMapCreate();
  stackInit(&env->scopes);
  return env;
}
TernaryValue environmentIsType(Environment const *env, Report *report,
                               TokenInfo const *token, char const *filename) {
  if (isScoped(token->data.string)) {
    char *moduleName;
    char *shortName;
    splitName(token->data.string, &moduleName, &shortName);
    if (strcmp(moduleName, env->currentModuleName) == 0) {
      SymbolInfo *info = symbolTableGet(env->currentModule, shortName);
      if (info != NULL) {
        free(moduleName);
        free(shortName);
        return info->kind == SK_TYPE ? YES : NO;
      }
    } else {
      for (size_t idx = 0; idx < env->imports->capacity; idx++) {
        if (strcmp(moduleName, env->imports->keys[idx]) == 0) {
          SymbolInfo *info =
              symbolTableGet(env->imports->values[idx], shortName);
          if (info != NULL) {
            free(moduleName);
            free(shortName);
            return info->kind == SK_TYPE ? YES : NO;
          }
        }
      }
    }

    if (isScoped(moduleName)) {
      char *enumModuleName;
      char *enumName;
      splitName(moduleName, &enumModuleName, &enumName);
      // TODO: check to see if it's a enum type
    }

    free(moduleName);
    free(shortName);
    reportError(report,
                format("%s:%zu:%zu: error: undefined identifier '%s'", filename,
                       token->line, token->character, token->data.string));
    return INDETERMINATE;
  } else {
    for (size_t idx = env->scopes.size; idx-- > 0;) {
      SymbolInfo *info =
          symbolTableGet(env->scopes.elements[idx], token->data.string);
      if (info != NULL) return info->kind == SK_TYPE ? YES : NO;
    }
    SymbolInfo *info = symbolTableGet(env->currentModule, token->data.string);
    if (info != NULL) return info->kind == SK_TYPE ? YES : NO;

    char const *foundModule = NULL;
    for (size_t idx = 0; idx < env->imports->capacity; idx++) {
      if (env->imports->keys[idx] != NULL) {
        SymbolInfo *current =
            symbolTableGet(env->imports->values[idx], token->data.string);
        if (current != NULL) {
          if (info == NULL) {
            info = current;
            foundModule = env->imports->keys[idx];
          } else {
            reportError(
                report,
                format(
                    "%s:%zu:%zu: error: identifier '%s' is "
                    "ambiguous\n\tcandidate module: %s\n\tcandidate module: %s",
                    filename, token->line, token->character, token->data.string,
                    env->imports->keys[idx], foundModule));
            return INDETERMINATE;
          }
        }
      }
    }
    if (info != NULL) {
      return info->kind == SK_TYPE ? YES : NO;
    } else {
      reportError(report, format("%s:%zu:%zu: error: undefined identifier '%s'",
                                 filename, token->line, token->character,
                                 token->data.string));
      return INDETERMINATE;
    }
  }
}
void environmentDestroy(Environment *env) {
  symbolTableDestroy(env->currentModule);
  moduleTableMapDestroy(env->imports);
  stackUninit(&env->scopes, (void (*)(void *))symbolTableDestroy);
  free(env);
}