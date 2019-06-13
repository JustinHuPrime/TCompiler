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

// Symbol table for parse time

#ifndef TLC_SYMBOLTABLE_TYPETABLE_H_
#define TLC_SYMBOLTABLE_TYPETABLE_H_

#include "util/container/hashMap.h"

#include "lexer/lexer.h"
#include "util/container/stack.h"
#include "util/errorReport.h"
#include "util/ternary.h"

typedef enum { ST_UNDEFINED = 0, ST_ID, ST_TYPE } SymbolType;

// exported types and identifiers for a module
// specialsization of a generic
typedef HashMap TypeTable;
TypeTable *typeTableCreate(void);
SymbolType typeTableGet(TypeTable const *, char const *key);
void typeTableSet(TypeTable *, char const *key, SymbolType value);
void typeTableDestroy(TypeTable *);

// specialization of a generic
typedef HashMap ModuleTypeTableMap;
ModuleTypeTableMap *moduleTypeTableMapCreate(void);
void moduleTypeTableMapInit(ModuleTypeTableMap *);
TypeTable *moduleTypeTableMapGet(ModuleTypeTableMap const *, char const *key);
int moduleTypeTableMapPut(ModuleTypeTableMap *, char const *key,
                          TypeTable *value);
void moduleTypeTableMapUninit(ModuleTypeTableMap *);
void moduleTypeTableMapDestroy(ModuleTypeTableMap *);

typedef struct {
  ModuleTypeTableMap imports;
  TypeTable *currentModule;
  char const *currentModuleName;
  Stack scopes;  // stack of local env TypeTables
} TypeEnvironment;
void typeEnvironmentInit(TypeEnvironment *, TypeTable *currentModule,
                         char const *currentModuleName);
SymbolType typeEnvironmentLookup(TypeEnvironment const *, Report *report,
                                 TokenInfo const *token, char const *filename);
TypeTable *typeEnvironmentTop(TypeEnvironment const *);
void typeEnvironmentUninit(TypeEnvironment *);

#endif  // TLC_SYMBOLTABLE_TYPETABLE_H_