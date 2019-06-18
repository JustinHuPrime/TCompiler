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

// definition of types

#ifndef TLC_SYMBOLTABLE_TYPE_H_
#define TLC_SYMBOLTABLE_TYPE_H_

#include "util/container/vector.h"

struct Type;

typedef enum {
  K_VOID,
  K_UBYTE,
  K_BYTE,
  K_CHAR,
  K_UINT,
  K_INT,
  K_WCHAR,
  K_ULONG,
  K_LONG,
  K_FLOAT,
  K_DOUBLE,
  K_BOOL,
  K_STRUCT,
  K_UNION,
  K_ENUM,
  K_TYPEDEF,
  K_CONST,
  K_ARRAY,
  K_PTR,
  K_FUNCTION_PTR,
} TypeKind;

// specialization of a generic
typedef Vector TypeVector;
// ctor
TypeVector *typeVectorCreate(void);
// in place ctor
void typeVectorInit(TypeVector *);
// insert
void typeVectorInsert(TypeVector *, struct Type *);
// in place dtor
// takes in a destructor function to apply to the elements
void typeVectorUninit(TypeVector *);
// dtor
void typeVectorDestroy(TypeVector *);

typedef struct Type {
  TypeKind kind;
  union {
    // struct, union, enum, typedef
    // look up the type in the stab
    struct {
      char const *name;
    } reference;
    // const, ptr
    struct {
      struct Type *type;
    } modifier;
    // arrays
    struct {
      struct Type *type;
      size_t size;
    } array;
    // function pointer
    struct {
      struct Type *returnType;
      TypeVector *argumentTypes;
    } functionPtr;
  } data;
} Type;

// ctor
Type *keywordTypeCreate(TypeKind kind);
Type *referneceTypeCreate(TypeKind kind, char const *name);
Type *modifierTypeCreate(TypeKind kind, Type *target);
Type *arrayTypeCreate(Type *target, size_t size);
Type *functionTypeCreate(Type *returnType, TypeVector *argumentTypes);
// in-place ctor
void keywordTypeInit(Type *, TypeKind kind);
void referneceTypeInit(Type *, TypeKind kind, char const *name);
void modifierTypeInit(Type *, TypeKind kind, Type *target);
void arrayTypeInit(Type *, Type *target, size_t size);
void functionTypeInit(Type *, Type *returnType, TypeVector *argumentTypes);
// in-place dtor
void typeUninit(Type *);
// dtor
void typeDestroy(Type *);

#endif  // TLC_SYMBOLTABLE_TYPE_H_