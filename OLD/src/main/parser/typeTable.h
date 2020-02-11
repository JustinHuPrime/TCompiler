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

#include "util/container/stack.h"
#include "util/errorReport.h"

struct TokenInfo;

typedef enum { ST_UNDEFINED = 0, ST_ID, ST_TYPE, ST_ENUMCONST } SymbolType;

// exported types and identifiers for a module
// specialsization of a generic
typedef HashMap TypeTable;
// ctor
TypeTable *typeTableCreate(void);
// copy ctor
TypeTable *typeTableCopy(TypeTable *);
// get
SymbolType typeTableGet(TypeTable const *, char const *key);
// set (*not* put!)
void typeTableSet(TypeTable *, char const *key, SymbolType value);
// dtor
void typeTableDestroy(TypeTable *);

// specialization of a generic
typedef HashMap ModuleTypeTableMap;
// ctor
ModuleTypeTableMap *moduleTypeTableMapCreate(void);
// in-place ctor
void moduleTypeTableMapInit(ModuleTypeTableMap *);
// get
TypeTable *moduleTypeTableMapGet(ModuleTypeTableMap const *, char const *key);
// put
int moduleTypeTableMapPut(ModuleTypeTableMap *, char const *key,
                          TypeTable *value);
// in-place dtor
void moduleTypeTableMapUninit(ModuleTypeTableMap *);
// dtor
void moduleTypeTableMapDestroy(ModuleTypeTableMap *);

typedef struct {
  ModuleTypeTableMap imports;
  TypeTable *currentModule;
  char const *currentModuleName;
  Stack scopes;  // stack of local env TypeTables
} TypeEnvironment;
// in-place ctor
void typeEnvironmentInit(TypeEnvironment *, TypeTable *currentModule,
                         char const *currentModuleName);
// get the symbol type of a token, reporting errors to report with the given
// filename
SymbolType typeEnvironmentLookup(TypeEnvironment const *, Report *report,
                                 struct TokenInfo const *token,
                                 char const *filename);
// gets the topmost type table
TypeTable *typeEnvironmentTop(TypeEnvironment const *);
// adds a type table to the top
void typeEnvironmentPush(TypeEnvironment *);
// remove a type table from the top
void typeEnvironmentPop(TypeEnvironment *);
// in-place dtor
void typeEnvironmentUninit(TypeEnvironment *);

#endif  // TLC_SYMBOLTABLE_TYPETABLE_H_