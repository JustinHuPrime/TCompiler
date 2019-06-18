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

#include "typecheck/symbolTable.h"

#include "util/functional.h"
#include "util/nameUtils.h"

#include <stdlib.h>
#include <string.h>

TypeVector *typeVectorCreate(void) { return vectorCreate(); }
void typeVectorInit(TypeVector *v) { vectorInit(v); }
void typeVectorInsert(TypeVector *v, struct Type *t) { vectorInsert(v, t); }
void typeVectorUninit(TypeVector *v) {
  vectorUninit(v, (void (*)(void *))typeDestroy);
}
void typeVectorDestroy(TypeVector *v) {
  vectorDestroy(v, (void (*)(void *))typeDestroy);
}

static Type *typeCreate(TypeKind kind) {
  Type *t = malloc(sizeof(Type));
  t->kind = kind;
  return t;
}
static void typeInit(Type *t, TypeKind kind) { t->kind = kind; }
Type *keywordTypeCreate(TypeKind kind) { return typeCreate(kind); }
Type *referneceTypeCreate(TypeKind kind, char const *name) {
  Type *t = typeCreate(kind);
  t->data.reference.name = name;
  return t;
}
Type *modifierTypeCreate(TypeKind kind, Type *target) {
  Type *t = typeCreate(kind);
  t->data.modifier.type = target;
  return t;
}
Type *arrayTypeCreate(Type *target, size_t size) {
  Type *t = typeCreate(K_ARRAY);
  t->data.array.type = target;
  t->data.array.size = size;
  return t;
}
Type *functionPtrTypeCreate(Type *returnType, TypeVector *argumentTypes) {
  Type *t = typeCreate(K_FUNCTION_PTR);
  t->data.functionPtr.returnType = returnType;
  t->data.functionPtr.argumentTypes = argumentTypes;
  return t;
}
void keywordTypeInit(Type *t, TypeKind kind) { typeInit(t, kind); }
void referneceTypeInit(Type *t, TypeKind kind, char const *name) {
  typeInit(t, kind);
  t->data.reference.name = name;
}
void modifierTypeInit(Type *t, TypeKind kind, Type *target) {
  typeInit(t, kind);
  t->data.modifier.type = target;
}
void arrayTypeInit(Type *t, Type *target, size_t size) {
  typeInit(t, K_ARRAY);
  t->data.array.type = target;
  t->data.array.size = size;
}
void functionTypeInit(Type *t, Type *returnType, TypeVector *argumentTypes) {
  typeInit(t, K_FUNCTION_PTR);
  t->data.functionPtr.returnType = returnType;
  t->data.functionPtr.argumentTypes = argumentTypes;
}
void typeUninit(Type *t) {
  switch (t->kind) {
    case K_VOID:
    case K_UBYTE:
    case K_BYTE:
    case K_CHAR:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_WCHAR:
    case K_ULONG:
    case K_LONG:
    case K_FLOAT:
    case K_DOUBLE:
    case K_BOOL:
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      break;
    }
    case K_CONST:
    case K_PTR: {
      typeDestroy(t->data.modifier.type);
      break;
    }
    case K_ARRAY: {
      typeDestroy(t->data.array.type);
      break;
    }
    case K_FUNCTION_PTR: {
      typeDestroy(t->data.functionPtr.returnType);
      typeVectorDestroy(t->data.functionPtr.argumentTypes);
      break;
    }
  }
}
void typeDestroy(Type *t) {
  typeUninit(t);
  free(t);
}

char const *typeDefinitionKindToString(TypeDefinitionKind kind) {
  switch (kind) {
    case TDK_STRUCT:
      return "a struct";
    case TDK_UNION:
      return "a union";
    case TDK_ENUM:
      return "an enumeration";
    case TDK_TYPEDEF:
      return "a type alias";
  }
  return NULL;  // not a valid enum - type safety violated
}

static SymbolInfo *symbolInfoCreate(SymbolKind kind) {
  SymbolInfo *si = malloc(sizeof(SymbolInfo));
  si->kind = kind;
  return si;
}
SymbolInfo *varSymbolInfoCreate(Type *type) {
  SymbolInfo *si = symbolInfoCreate(SK_VAR);
  si->data.var.type = type;
  return si;
}
SymbolInfo *structSymbolInfoCreate(void) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE);
  si->data.type.kind = TDK_STRUCT;
  si->data.type.data.structType.incomplete = true;
  typeVectorInit(&si->data.type.data.structType.fields);
  stringVectorInit(&si->data.type.data.structType.names);
  return si;
}
SymbolInfo *unionSymbolInfoCreate(void) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE);
  si->data.type.kind = TDK_UNION;
  si->data.type.data.unionType.incomplete = true;
  typeVectorInit(&si->data.type.data.unionType.fields);
  stringVectorInit(&si->data.type.data.unionType.names);
  return si;
}
SymbolInfo *enumSymbolInfoCreate(void) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE);
  si->data.type.kind = TDK_ENUM;
  si->data.type.data.enumType.incomplete = true;
  stringVectorInit(&si->data.type.data.enumType.fields);
  return si;
}
SymbolInfo *typedefSymbolInfoCreate(Type *what) {
  SymbolInfo *si = symbolInfoCreate(SK_TYPE);
  si->data.type.kind = TDK_TYPEDEF;
  si->data.type.data.typedefType.type = what;
  return si;
}
SymbolInfo *functionSymbolInfoCreate(Type *returnType) {
  SymbolInfo *si = symbolInfoCreate(SK_FUNCTION);
  si->data.function.returnType = returnType;
  vectorInit(&si->data.function.argumentTypeSets);
  return si;
}
char const *symbolInfoToKindString(SymbolInfo const *si) {
  switch (si->kind) {
    case SK_VAR: {
      return "a variable";
    }
    case SK_TYPE: {
      switch (si->data.type.kind) {
        case TDK_STRUCT:
          return "a struct";
        case TDK_UNION:
          return "a union";
        case TDK_ENUM:
          return "an enum";
        case TDK_TYPEDEF:
          return "a typedef";
        default:
          return NULL;  // error - not a valid enum
      }
    }
    case SK_FUNCTION: {
      return "a function";
    }
    default:
      return NULL;  // error - not a valid enum
  }
}
void symbolInfoDestroy(SymbolInfo *si) {
  switch (si->kind) {
    case SK_VAR: {
      typeDestroy(si->data.var.type);
      break;
    }
    case SK_TYPE: {
      switch (si->data.type.kind) {
        case TDK_STRUCT: {
          typeVectorUninit(&si->data.type.data.structType.fields);
          stringVectorUninit(&si->data.type.data.structType.names, false);
          break;
        }
        case TDK_UNION: {
          typeVectorUninit(&si->data.type.data.unionType.fields);
          stringVectorUninit(&si->data.type.data.unionType.names, false);
          break;
        }
        case TDK_ENUM: {
          stringVectorUninit(&si->data.type.data.unionType.fields, false);
          break;
        }
        case TDK_TYPEDEF: {
          typeDestroy(si->data.type.data.typedefType.type);
          break;
        }
      }
      break;
    }
    case SK_FUNCTION: {
      typeDestroy(si->data.function.returnType);
      vectorUninit(&si->data.function.argumentTypeSets,
                   (void (*)(void *))typeVectorUninit);
      break;
    }
  }
  free(si);
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
void moduleTableMapInit(ModuleTableMap *map) { hashMapInit(map); }
SymbolTable *moduleTableMapGet(ModuleTableMap const *table, char const *key) {
  return hashMapGet(table, key);
}
int moduleTableMapPut(ModuleTableMap *table, char const *key,
                      SymbolTable *value) {
  return hashMapPut(table, key, value, nullDtor);
}
void moduleTableMapUninit(ModuleTableMap *map) { hashMapUninit(map, nullDtor); }
void moduleTableMapDestroy(ModuleTableMap *table) {
  hashMapDestroy(table, nullDtor);
}

Environment *environmentCreate(SymbolTable *currentModule,
                               char const *currentModuleName) {
  Environment *env = malloc(sizeof(Environment));
  environmentInit(env, currentModule, currentModuleName);
  return env;
}
void environmentInit(Environment *env, SymbolTable *currentModule,
                     char const *currentModuleName) {
  env->currentModule = currentModule;
  env->currentModuleName = currentModuleName;
  moduleTableMapInit(&env->imports);
  stackInit(&env->scopes);
}
static SymbolInfo *environmentLookupInternal(Environment const *env,
                                             Report *report, char const *id,
                                             size_t line, size_t character,
                                             char const *filename,
                                             bool reportErrors) {
  if (isScoped(id)) {
    char *moduleName;
    char *shortName;
    splitName(id, &moduleName, &shortName);
    if (strcmp(moduleName, env->currentModuleName) == 0) {
      return symbolTableGet(env->currentModule, shortName);
    } else {
      for (size_t idx = 0; idx < env->imports.capacity; idx++) {
        if (strcmp(moduleName, env->imports.keys[idx]) == 0) {
          SymbolInfo *info =
              symbolTableGet(env->imports.values[idx], shortName);
          if (info != NULL) {
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

      SymbolInfo *enumType = environmentLookupInternal(
          env, report, enumModuleName, line, character, filename, false);
      free(enumName);
      free(enumModuleName);

      if (enumType != NULL && enumType->kind == SK_TYPE) {
        free(moduleName);
        free(shortName);
        return enumType;
      }
    }

    free(moduleName);
    free(shortName);
    if (reportErrors) {
      reportError(report, "%s:%zu:%zu: error: undefined identifier '%s'",
                  filename, line, character, id);
    }
    return NULL;
  } else {
    for (size_t idx = env->scopes.size; idx-- > 0;) {
      SymbolInfo *info = symbolTableGet(env->scopes.elements[idx], id);
      if (info != NULL) return info;
    }
    SymbolInfo *info = symbolTableGet(env->currentModule, id);
    if (info != NULL) return info;

    char const *foundModule = NULL;
    for (size_t idx = 0; idx < env->imports.capacity; idx++) {
      if (env->imports.keys[idx] != NULL) {
        SymbolInfo *current = symbolTableGet(env->imports.values[idx], id);
        if (current != NULL) {
          if (info == NULL) {
            info = current;
            foundModule = env->imports.keys[idx];
          } else {
            if (reportErrors) {
              reportError(report,
                          "%s:%zu:%zu: error: identifier '%s' is ambiguous",
                          filename, line, character, id);
              reportMessage(report, "\tcandidate module: %s",
                            env->imports.keys[idx]);
              reportMessage(report, "\tcandidate module: %s", foundModule);
            }
            return NULL;
          }
        }
      }
    }
    if (info != NULL) {
      return info;
    } else {
      if (reportErrors) {
        reportError(report, "%s:%zu:%zu: error: undefined identifier '%s'",
                    filename, line, character, id);
      }
      return NULL;
    }
  }
}
SymbolInfo *environmentLookup(Environment const *env, Report *report,
                              char const *id, size_t line, size_t character,
                              char const *filename) {
  return environmentLookupInternal(env, report, id, line, character, filename,
                                   true);
}
SymbolTable *environmentTop(Environment const *env) {
  return env->scopes.size == 0 ? env->currentModule : stackPeek(&env->scopes);
}
void environmentPush(Environment *env) {
  stackPush(&env->scopes, symbolTableCreate());
}
SymbolTable *environmentPop(Environment *env) { return stackPop(&env->scopes); }
void environmentUninit(Environment *env) {
  moduleTableMapUninit(&env->imports);
  stackUninit(&env->scopes, (void (*)(void *))symbolTableDestroy);
}
void environmentDestroy(Environment *env) {
  environmentUninit(env);
  free(env);
}