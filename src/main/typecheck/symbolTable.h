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

#include "lexer/lexer.h"
#include "typecheck/type.h"
#include "util/container/hashMap.h"
#include "util/container/stack.h"
#include "util/container/vector.h"
#include "util/errorReport.h"
#include "util/ternary.h"

typedef enum {
  SK_VAR,
  SK_TYPE,
  SK_FUNCTION,
} SymbolKind;
char const *symbolKindToString(SymbolKind);

typedef enum {
  TDK_STRUCT,
  TDK_UNION,
  TDK_ENUM,
  TDK_TYPEDEF,
} TypeDefinitionKind;
char const *typeDefinitionKindToString(TypeDefinitionKind);

// information for a symbol in some module
typedef struct {
  SymbolKind kind;
  union {
    struct {
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
      Type *returnType;
      Vector argumentTypeSets;  // vector of vectors of types
    } function;
  } data;
} SymbolInfo;
// ctor
SymbolInfo *varSymbolInfoCreate(Type *);
SymbolInfo *structSymbolInfoCreate(void);
SymbolInfo *unionSymbolInfoCreate(void);
SymbolInfo *enumSymbolInfoCreate(void);
SymbolInfo *typedefSymbolInfoCreate(Type *);
SymbolInfo *functionSymbolInfoCreate(Type *returnType);
// dtor
void symbolInfoDestroy(SymbolInfo *);

// symbol table for a module
// specialsization of a generic
typedef HashMap SymbolTable;
SymbolTable *symbolTableCreate(void);
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

typedef struct {
  ModuleTableMap imports;         // map of symbol tables, non-owning
  SymbolTable *currentModule;     // non-owing current stab
  char const *currentModuleName;  // non-owning current module name
  Stack scopes;                   // stack of local env symbol tables (owning)
} Environment;
void environmentInit(Environment *, SymbolTable *currentModule,
                     char const *currentModuleName);
TernaryValue environmentIsType(Environment const *, Report *report,
                               TokenInfo const *token, char const *filename);
void environmentUninit(Environment *);

#endif  // TLC_SYMBOLTABLE_SYMBOLTABLE_H_