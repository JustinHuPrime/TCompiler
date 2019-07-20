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

// Symbol table for typecheck time

#ifndef TLC_SYMBOLTABLE_SYMBOLTABLE_H_
#define TLC_SYMBOLTABLE_SYMBOLTABLE_H_

#include "util/container/hashMap.h"
#include "util/container/stack.h"
#include "util/container/vector.h"
#include "util/errorReport.h"

#include "util/container/vector.h"

struct Type;
struct Environment;
struct SymbolInfo;

typedef enum {
  K_VOID,
  K_UBYTE,
  K_BYTE,
  K_CHAR,
  K_USHORT,
  K_SHORT,
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
  K_AGGREGATE_INIT,
} TypeKind;

// specialization of a generic
typedef Vector TypeVector;
// ctor
TypeVector *typeVectorCreate(void);
// in place ctor
void typeVectorInit(TypeVector *);
// copy ctor
TypeVector *typeVectorCopy(TypeVector *);
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
      struct SymbolInfo *referenced;
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
    // special type for aggregate init expression
    struct {
      TypeVector *elementTypes;
    } aggregateInit;
  } data;
} Type;

// ctor
Type *keywordTypeCreate(TypeKind kind);
Type *referneceTypeCreate(TypeKind kind, struct SymbolInfo *referenced);
Type *modifierTypeCreate(TypeKind kind, Type *target);
Type *arrayTypeCreate(Type *target, size_t size);
Type *functionPtrTypeCreate(Type *returnType, TypeVector *argumentTypes);
Type *aggregateInitTypeCreate(TypeVector *elementTypes);
// in-place ctor
void keywordTypeInit(Type *, TypeKind kind);
void referneceTypeInit(Type *, TypeKind kind, struct SymbolInfo *referenced);
void modifierTypeInit(Type *, TypeKind kind, Type *target);
void arrayTypeInit(Type *, Type *target, size_t size);
void functionPtrTypeInit(Type *, Type *returnType, TypeVector *argumentTypes);
void aggregateInitTypeInit(Type *, TypeVector *elementTypes);
// copy ctor
Type *typeCopy(Type const *);
// if type is incomplete, returns true, else false
bool typeIsIncomplete(Type const *, struct Environment const *);
// determines if two types are completely equal
bool typeEqual(Type const *, Type const *);
// can a thing of the second type be assigned to a thing of the first type
// assumes first type is an lvalue
bool typeAssignable(Type const *to, Type const *from);
// can a these two types be compared
bool typeComparable(Type const *, Type const *);
// can a thing of the second type be cast to a thing of the first type
bool typeCastable(Type const *to, Type const *from);
// common predicates
bool typeIsBoolean(Type const *);
bool typeIsIntegral(Type const *);
bool typeIsSignedIntegral(Type const *);
bool typeIsFloat(Type const *);
bool typeIsNumeric(Type const *);
bool typeIsValuePointer(Type const *);
bool typeIsFunctionPointer(Type const *);
bool typeIsPointer(Type const *);
bool typeIsCompound(Type const *);
bool typeIsArray(Type const *);
// merge types from conditional branches
Type *typeTernaryExpMerge(Type const *, Type const *);
// merge types from arithmetic branches
Type *typeArithmeticExpMerge(Type const *, Type const *);
// strip constness off of start of type, non-destructively
Type *typeGetNonConst(Type *);
// strip pointerness and constness off of start of type, produces new type
Type *typeGetDereferenced(Type *);
// strip arrayness and constness off of start of type, produces new type
Type *typeGetArrayElement(Type *);
char *typeToString(Type const *);
// in-place dtor
void typeUninit(Type *);
// dtor
void typeDestroy(Type *);

// POD struct containing data for an overload set instance
typedef struct {
  Type *returnType;
  TypeVector argumentTypes;
  size_t numOptional;
  bool defined;
} OverloadSetElement;
// ctor
OverloadSetElement *overloadSetElementCreate(void);
OverloadSetElement *overloadSetElementCopy(OverloadSetElement const *);
// dtor
void overloadSetElementDestroy(OverloadSetElement *);

// a 'set' of overload options
typedef Vector OverloadSet;
// in-place ctor
void overloadSetInit(OverloadSet *);
// add
void overloadSetInsert(OverloadSet *, OverloadSetElement *);
// lookup
// finds a single one that potentially collides
OverloadSetElement *overloadSetLookupCollision(OverloadSet *,
                                               TypeVector const *argTypes,
                                               size_t numOptional);
// finds a single one that matches the argument types (ignoring defaults)
// exactly
OverloadSetElement *overloadSetLookupDefinition(OverloadSet *,
                                                TypeVector const *argTypes);
// finds all that fit this set of arguments - multiple indicates ambiguity
OverloadSet *overloadSetLookupCall(OverloadSet *, TypeVector const *argTypes);
// dtor
void overloadSetUninit(OverloadSet *);

typedef enum {
  SK_VAR,
  SK_TYPE,
  SK_FUNCTION,
} SymbolKind;

typedef enum {
  TDK_STRUCT,
  TDK_UNION,
  TDK_ENUM,
  TDK_TYPEDEF,
} TypeDefinitionKind;
char const *typeDefinitionKindToString(TypeDefinitionKind);

// information for a symbol in some module
typedef struct SymbolInfo {
  SymbolKind kind;
  union {
    struct {
      bool bound;
      Type *type;
    } var;
    struct {
      TypeDefinitionKind kind;
      union {
        struct {
          bool incomplete;
          TypeVector fields;
          StringVector names;
        } structType;
        struct {
          bool incomplete;
          TypeVector fields;
          StringVector names;
        } unionType;
        struct {
          bool incomplete;
          StringVector fields;
        } enumType;
        struct {
          Type *type;
        } typedefType;
      } data;
    } type;
    struct {
      OverloadSet overloadSet;
    } function;
    struct {
      Type *parentEnum;
    } enumConst;
  } data;
} SymbolInfo;
// ctor
SymbolInfo *varSymbolInfoCreate(Type *, bool bound);
SymbolInfo *structSymbolInfoCreate(void);
SymbolInfo *unionSymbolInfoCreate(void);
SymbolInfo *enumSymbolInfoCreate(void);
SymbolInfo *typedefSymbolInfoCreate(Type *);
SymbolInfo *functionSymbolInfoCreate(void);
SymbolInfo *symbolInfoCopy(SymbolInfo const *);
// printing
char const *symbolInfoToKindString(SymbolInfo const *);
// dtor
void symbolInfoDestroy(SymbolInfo *);

// symbol table for a module
// specialsization of a generic
typedef HashMap SymbolTable;
SymbolTable *symbolTableCreate(void);
SymbolTable *symbolTableCopy(SymbolTable const *);
SymbolInfo *symbolTableGet(SymbolTable const *, char const *key);
int symbolTablePut(SymbolTable *, char const *key, SymbolInfo *value);
void symbolTableDestroy(SymbolTable *);

// map b/w module name and symbol table
// specialization of a generic
typedef HashMap ModuleTableMap;
ModuleTableMap *moduleTableMapCreate(void);
void moduleTableMapInit(ModuleTableMap *);
SymbolTable *moduleTableMapGet(ModuleTableMap const *, char const *key);
int moduleTableMapPut(ModuleTableMap *, char const *key, SymbolTable *value);
void moduleTableMapUninit(ModuleTableMap *);
void moduleTableMapDestroy(ModuleTableMap *);

typedef struct Environment {
  ModuleTableMap imports;         // map of symbol tables, non-owning
  SymbolTable *currentModule;     // non-owing current stab
  char const *currentModuleName;  // non-owning current module name
  Stack scopes;                   // stack of local env symbol tables (owning)
} Environment;
Environment *environmentCreate(SymbolTable *currentModule,
                               char const *currentModuleName);
void environmentInit(Environment *, SymbolTable *currentModule,
                     char const *currentModuleName);
SymbolInfo *environmentLookup(Environment const *, Report *report,
                              char const *id, size_t line, size_t character,
                              char const *filename);
SymbolTable *environmentTop(Environment const *);
void environmentPush(Environment *);
SymbolTable *environmentPop(Environment *);
void environmentUninit(Environment *);
void environmentDestroy(Environment *);

#endif  // TLC_SYMBOLTABLE_SYMBOLTABLE_H_