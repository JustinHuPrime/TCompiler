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

#include "parser/typeTable.h"

#include "util/functional.h"
#include "util/nameUtils.h"

#include <stdlib.h>
#include <string.h>

TypeTable *typeTableCreate(void) { return hashMapCreate(); }
TypeTable *typeTableCopy(TypeTable *from) {
  TypeTable *to = malloc(sizeof(TypeTable));
  memcpy(to, from, sizeof(TypeTable));  // shallow copy is okay - keys held
                                        // weakly, value is integer
  return to;
}
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
static SymbolType typeEnvironmentLookupInternal(TypeEnvironment const *env,
                                                Report *report,
                                                TokenInfo const *token,
                                                char const *filename,
                                                bool reportErrors) {
  if (isScoped(token->data.string)) {
    char *moduleName;
    char *shortName;
    splitName(token->data.string, &moduleName, &shortName);
    if (strcmp(moduleName, env->currentModuleName) == 0) {
      return typeTableGet(env->currentModule, shortName);
    } else {
      for (size_t idx = 0; idx < env->imports.capacity; idx++) {
        if (strcmp(moduleName, env->imports.keys[idx]) == 0) {
          SymbolType info = typeTableGet(env->imports.values[idx], shortName);
          if (info != ST_UNDEFINED) {
            free(moduleName);
            free(shortName);
            return info;
          }
        }
      }
    }

    if (isScoped(moduleName)) {
      char *enumModuleName;
      char *enumName;
      splitName(moduleName, &enumModuleName, &enumName);

      TokenInfo enumModule;
      memcpy(&enumModule, token, sizeof(TokenInfo));
      enumModule.data.string = enumModuleName;

      SymbolType enumType = typeEnvironmentLookupInternal(
          env, report, &enumModule, filename, false);
      free(enumName);
      free(enumModuleName);
      tokenInfoUninit(&enumModule);

      if (enumType == ST_TYPE) {
        free(moduleName);
        free(shortName);

        return ST_ID;
      }
    }

    free(moduleName);
    free(shortName);
    if (reportErrors) {
      reportError(report, "%s:%zu:%zu: error: undefined identifier '%s'",
                  filename, token->line, token->character, token->data.string);
    }
    return ST_UNDEFINED;
  } else {
    for (size_t idx = env->scopes.size; idx-- > 0;) {
      SymbolType info =
          typeTableGet(env->scopes.elements[idx], token->data.string);
      if (info != ST_UNDEFINED) return info;
    }
    SymbolType info = typeTableGet(env->currentModule, token->data.string);
    if (info != ST_UNDEFINED) return info;

    char const *foundModule = NULL;
    for (size_t idx = 0; idx < env->imports.capacity; idx++) {
      if (env->imports.keys[idx] != NULL) {
        SymbolType current =
            typeTableGet(env->imports.values[idx], token->data.string);
        if (current != ST_UNDEFINED) {
          if (info == ST_UNDEFINED) {
            info = current;
            foundModule = env->imports.keys[idx];
          } else {
            if (reportErrors) {
              reportError(
                  report, "%s:%zu:%zu: error: identifier '%s' is ambiguous",
                  filename, token->line, token->character, token->data.string);
              reportMessage(report, "\tcandidate module: %s",
                            env->imports.keys[idx]);
              reportMessage(report, "\tcandidate module: %s", foundModule);
            }
            return ST_UNDEFINED;
          }
        }
      }
    }
    if (info != ST_UNDEFINED) {
      return info;
    } else {
      if (reportErrors) {
        reportError(report, "%s:%zu:%zu: error: undefined identifier '%s'",
                    filename, token->line, token->character,
                    token->data.string);
      }
      return ST_UNDEFINED;
    }
  }
}
SymbolType typeEnvironmentLookup(TypeEnvironment const *env, Report *report,
                                 TokenInfo const *token, char const *filename) {
  return typeEnvironmentLookupInternal(env, report, token, filename, true);
}
TypeTable *typeEnvironmentTop(TypeEnvironment const *env) {
  return env->scopes.size == 0 ? env->currentModule : stackPeek(&env->scopes);
}
void typeEnvironmentPush(TypeEnvironment *env) {
  stackPush(&env->scopes, typeTableCreate());
}
void typeEnvironmentPop(TypeEnvironment *env) {
  typeTableDestroy(stackPop(&env->scopes));
}
void typeEnvironmentUninit(TypeEnvironment *env) {
  moduleTypeTableMapUninit(&env->imports);
  stackUninit(&env->scopes, (void (*)(void *))typeTableDestroy);
}