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

// Symbol table for parse and typecheck time

#ifndef TLC_SYMBOLTABLE_SYMBOLTABLE_H_
#define TLC_SYMBOLTABLE_SYMBOLTABLE_H_

#include "lexer/lexer.h"
#include "util/container/hashMap.h"
#include "util/container/stack.h"
#include "util/container/vector.h"
#include "util/errorReport.h"
#include "util/ternary.h"

// information for a symbol in some module
typedef struct {
  enum {
    SK_VAR,
    SK_TYPE,
    SK_FUNCTION,
  } kind;
  union {
    struct {
      void *type;  // TODO: figure this out later
    } var;
    struct {
      enum {
        TK_STRUCT,
        TK_UNION,
        TK_ENUM,
        TK_TYPEDEF,
      } typeKind;
      void *type;  // TODO: figure this out later
      union {
        struct {
          void *type;  // TODO: figure this out later
        } structType;
        struct {
          void *type;  // TODO: figure this out later
        } unionType;
        struct {
          void *type;  // TODO: figure this out later
        } enumType;
        struct {
          void *type;  // TODO: figure this out later
        } typedefType;
      } data;
    } type;
    struct {
      void *type;  // TODO: figure this out later
    } function;
    struct {
      void *parentEnum;  // TODO: figure this out later
    } enumField;
  } data;
} SymbolInfo;
SymbolInfo *symbolInfoCreate(void);
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
Environment *environmentCreate(SymbolTable *currentModule,
                               char const *currentModuleName);
void environmentInit(Environment *, SymbolTable *currentModule,
                     char const *currentModuleName);
TernaryValue environmentIsType(Environment const *, Report *report,
                               TokenInfo const *token, char const *filename);
void environmentUninit(Environment *);
void environmentDestroy(Environment *);

#endif  // TLC_SYMBOLTABLE_SYMBOLTABLE_H_