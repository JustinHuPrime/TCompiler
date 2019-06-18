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

// implementation of of types

#include "typecheck/type.h"

#include <stdlib.h>

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
Type *functionTypeCreate(Type *returnType, TypeVector *argumentTypes) {
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