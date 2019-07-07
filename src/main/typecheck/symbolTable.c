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
Type *referneceTypeCreate(TypeKind kind, SymbolInfo *referenced) {
  Type *t = typeCreate(kind);
  t->data.reference.referenced = referenced;
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
void referneceTypeInit(Type *t, TypeKind kind, struct SymbolInfo *referenced) {
  typeInit(t, kind);
  t->data.reference.referenced = referenced;
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
void functionPtrTypeInit(Type *t, Type *returnType, TypeVector *argumentTypes) {
  typeInit(t, K_FUNCTION_PTR);
  t->data.functionPtr.returnType = returnType;
  t->data.functionPtr.argumentTypes = argumentTypes;
}
Type *typeCopy(Type const *t) {
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
    case K_BOOL: {
      return keywordTypeCreate(t->kind);
    }
    case K_CONST:
    case K_PTR: {
      return modifierTypeCreate(t->kind, typeCopy(t->data.modifier.type));
    }
    case K_FUNCTION_PTR: {
      TypeVector *argTypes = typeVectorCreate();
      for (size_t idx = 0; idx < t->data.functionPtr.argumentTypes->size;
           idx++) {
        typeVectorInsert(
            argTypes,
            typeCopy(t->data.functionPtr.argumentTypes->elements[idx]));
      }
      return functionPtrTypeCreate(typeCopy(t->data.functionPtr.returnType),
                                   argTypes);
    }
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      return referneceTypeCreate(t->kind, t->data.reference.referenced);
    }
    case K_ARRAY: {
      return arrayTypeCreate(typeCopy(t->data.array.type), t->data.array.size);
    }
    default: {
      return NULL;  // error - not a valid enum
    }
  }
}
bool typeIsIncomplete(Type const *t, Environment const *env) {
  switch (t->kind) {
    case K_VOID: {
      return true;
    }
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
    case K_FUNCTION_PTR:
    case K_PTR: {
      return false;
    }
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      SymbolInfo *info = t->data.reference.referenced;
      switch (info->data.type.kind) {
        case TDK_STRUCT: {
          return info->data.type.data.structType.incomplete;
        }
        case TDK_UNION: {
          return info->data.type.data.unionType.incomplete;
        }
        case TDK_ENUM: {
          return info->data.type.data.enumType.incomplete;
        }
        case TDK_TYPEDEF: {
          return typeIsIncomplete(info->data.type.data.typedefType.type, env);
        }
        default: {
          return true;  // error - not a valid enum
        }
      }
    }
    case K_CONST: {
      return typeIsIncomplete(t->data.modifier.type, env);
    }
    case K_ARRAY: {
      return typeIsIncomplete(t->data.array.type, env);
    }
    default: {
      return true;  // error: not a valid enum
    }
  }
}
bool typeEqual(Type const *t1, Type const *t2) {
  if (t1->kind != t2->kind) {
    return false;
  } else {
    switch (t1->kind) {
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
      case K_BOOL: {
        return true;
      }
      case K_FUNCTION_PTR: {
        if (t1->data.functionPtr.argumentTypes->size !=
            t2->data.functionPtr.argumentTypes->size) {
          return false;
        }
        for (size_t idx = 0; idx < t1->data.functionPtr.argumentTypes->size;
             idx++) {
          if (!typeEqual(t1->data.functionPtr.argumentTypes->elements[idx],
                         t2->data.functionPtr.argumentTypes->elements[idx])) {
            return false;
          }
        }

        return typeEqual(t1->data.functionPtr.returnType,
                         t2->data.functionPtr.returnType);
      }
      case K_PTR:
      case K_CONST: {
        return typeEqual(t1->data.modifier.type, t2->data.modifier.type);
      }
      case K_ARRAY: {
        return t1->data.array.size == t2->data.array.size &&
               typeEqual(t1->data.array.type, t2->data.array.type);
      }
      case K_STRUCT:
      case K_UNION:
      case K_ENUM:
      case K_TYPEDEF: {
        return t1->data.reference.referenced == t2->data.reference.referenced;
        // this works because references reference the symbol table entry
      }
      default: {
        return false;  // error: not a valid enum
      }
    }
  }
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

OverloadSetElement *overloadSetElementCreate(void) {
  OverloadSetElement *elm = malloc(sizeof(OverloadSetElement));
  typeVectorInit(&elm->argumentTypes);
  elm->returnType = NULL;
  return elm;
}
OverloadSetElement *overloadSetElementCopy(OverloadSetElement const *from) {
  OverloadSetElement *to = malloc(sizeof(OverloadSetElement));

  to->defined = from->defined;
  to->numOptional = from->numOptional;
  to->returnType = typeCopy(from->returnType);
  to->argumentTypes.capacity = from->argumentTypes.capacity;
  to->argumentTypes.size = from->argumentTypes.size;
  to->argumentTypes.elements =
      malloc(to->argumentTypes.capacity * sizeof(void *));
  for (size_t idx = 0; idx < to->argumentTypes.size; idx++) {
    to->argumentTypes.elements[idx] =
        typeCopy(from->argumentTypes.elements[idx]);
  }

  return to;
}
void overloadSetElementDestroy(OverloadSetElement *elm) {
  if (elm->returnType != NULL) {
    typeDestroy(elm->returnType);
  }
  typeVectorUninit(&elm->argumentTypes);
}

void overloadSetInit(OverloadSet *set) { vectorInit(set); }
void overloadSetInsert(OverloadSet *set, OverloadSetElement *elm) {
  vectorInsert(set, elm);
}
OverloadSetElement *overloadSetLookupCollision(OverloadSet *set,
                                               TypeVector const *argTypes,
                                               size_t numOptional) {
  for (size_t candidateIdx = 0; candidateIdx < set->size; candidateIdx++) {
    // select n args, where n is the larger of the required argument sizes.
    // if that minimum is within both the possible number of args for the
    // candidate and the lookup, try to match types if they all match, return
    // match, else, keep looking
    OverloadSetElement *candidate = set->elements[candidateIdx];
    size_t candidateRequired =
        candidate->argumentTypes.size - candidate->numOptional;
    size_t thisRequired = argTypes->size - numOptional;
    size_t maxRequired =
        candidateRequired > thisRequired ? candidateRequired : thisRequired;
    bool candidateLonger = candidateRequired > thisRequired;
    if (candidateLonger ? argTypes->size >= maxRequired
                        : candidate->argumentTypes.size >= maxRequired) {
      bool match = true;
      for (size_t idx = 0; idx < maxRequired; idx++) {
        if (!typeEqual(
                (candidateLonger ? candidate->argumentTypes.elements
                                 : argTypes->elements)[idx],
                (candidateLonger ? argTypes->elements
                                 : candidate->argumentTypes.elements)[idx])) {
          match = false;
          break;
        }
      }
      if (match) {
        return candidate;
      }
    }
  }

  return NULL;
}
OverloadSetElement *overloadSetLookupDefinition(OverloadSet *set,
                                                TypeVector const *argTypes) {
  for (size_t candidateIdx = 0; candidateIdx < set->size; candidateIdx++) {
    OverloadSetElement *candidate = set->elements[candidateIdx];
    if (candidate->argumentTypes.size == argTypes->size) {
      bool match = true;
      for (size_t argIdx = 0; argIdx < argTypes->size; argIdx++) {
        if (!typeEqual(candidate->argumentTypes.elements[argIdx],
                       argTypes->elements[argIdx])) {
          match = false;
          break;
        }
      }
      if (match) {
        return candidate;
      }
    }
  }

  return NULL;
}
void overloadSetUninit(OverloadSet *set) {
  vectorUninit(set, (void (*)(void *))overloadSetElementDestroy);
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
SymbolInfo *functionSymbolInfoCreate(void) {
  SymbolInfo *si = symbolInfoCreate(SK_FUNCTION);
  overloadSetInit(&si->data.function.overloadSet);
  return si;
}
SymbolInfo *symbolInfoCopy(SymbolInfo const *from) {
  SymbolInfo *to = malloc(sizeof(SymbolInfo));

  to->kind = from->kind;
  switch (to->kind) {
    case SK_VAR: {
      to->data.var.type = typeCopy(from->data.var.type);
      break;
    }
    case SK_TYPE: {
      to->data.type.kind = from->data.type.kind;
      switch (to->data.type.kind) {
        case TDK_STRUCT: {
          TypeVector *toFields = &to->data.type.data.structType.fields;
          TypeVector const *fromFields =
              &from->data.type.data.structType.fields;
          toFields->capacity = fromFields->capacity;
          toFields->size = fromFields->size;
          toFields->elements = malloc(toFields->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toFields->size; idx++) {
            toFields->elements[idx] = typeCopy(fromFields->elements[idx]);
          }
          TypeVector *toNames = &to->data.type.data.structType.names;
          TypeVector const *fromNames = &from->data.type.data.structType.names;
          toNames->capacity = fromNames->capacity;
          toNames->size = fromNames->size;
          toNames->elements = malloc(toNames->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toNames->size; idx++) {
            toNames->elements[idx] = typeCopy(fromNames->elements[idx]);
          }
          to->data.type.data.structType.incomplete =
              from->data.type.data.structType.incomplete;
          break;
        }
        case TDK_UNION: {
          TypeVector *toFields = &to->data.type.data.unionType.fields;
          TypeVector const *fromFields = &from->data.type.data.unionType.fields;
          toFields->capacity = fromFields->capacity;
          toFields->size = fromFields->size;
          toFields->elements = malloc(toFields->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toFields->size; idx++) {
            toFields->elements[idx] = typeCopy(fromFields->elements[idx]);
          }
          StringVector *toNames = &to->data.type.data.unionType.names;
          StringVector const *fromNames = &from->data.type.data.unionType.names;
          toNames->capacity = fromNames->capacity;
          toNames->size = fromNames->size;
          toNames->elements = malloc(toNames->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toNames->size; idx++) {
            toNames->elements[idx] = typeCopy(fromNames->elements[idx]);
          }
          to->data.type.data.unionType.incomplete =
              from->data.type.data.unionType.incomplete;
          break;
        }
        case TDK_ENUM: {
          StringVector *toFields = &to->data.type.data.enumType.fields;
          StringVector const *fromFields =
              &from->data.type.data.enumType.fields;
          toFields->capacity = fromFields->capacity;
          toFields->size = fromFields->size;
          toFields->elements = malloc(toFields->capacity * sizeof(void *));
          for (size_t idx = 0; idx < toFields->size; idx++) {
            toFields->elements[idx] = typeCopy(fromFields->elements[idx]);
          }
          to->data.type.data.enumType.incomplete =
              from->data.type.data.enumType.incomplete;
          break;
        }
        case TDK_TYPEDEF: {
          to->data.type.data.typedefType.type =
              typeCopy(from->data.type.data.typedefType.type);
          break;
        }
      }
      break;
    }
    case SK_FUNCTION: {
      OverloadSet *toOverloads = &to->data.function.overloadSet;
      OverloadSet const *fromOverloads = &from->data.function.overloadSet;
      toOverloads->capacity = fromOverloads->capacity;
      toOverloads->size = fromOverloads->size;
      toOverloads->elements = malloc(toOverloads->size * sizeof(void *));
      for (size_t idx = 0; idx < toOverloads->size; idx++) {
        toOverloads->elements[idx] =
            overloadSetElementCopy(fromOverloads->elements[idx]);
      }
      break;
    }
  }

  return to;
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
      overloadSetUninit(&si->data.function.overloadSet);
      break;
    }
  }
  free(si);
}

SymbolTable *symbolTableCreate(void) { return hashMapCreate(); }
SymbolTable *symbolTableCopy(SymbolTable const *from) {
  SymbolTable *to = malloc(sizeof(SymbolTable));
  to->capacity = from->capacity;
  to->size = from->size;
  to->keys = calloc(to->capacity, sizeof(char const *));
  to->values = malloc(to->capacity * sizeof(void *));
  memcpy(to->keys, from->keys, to->capacity * sizeof(char const *));

  for (size_t idx = 0; idx < to->size; idx++) {
    if (to->keys[idx] != NULL) {
      to->values[idx] = symbolInfoCopy(from->values[idx]);
    }
  }

  return to;
}
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
SymbolInfo *environmentLookupMustSucceed(Environment const *env,
                                         char const *id) {
  return environmentLookupInternal(env, NULL, id, 0, 0, NULL, false);
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