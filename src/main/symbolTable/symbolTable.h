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

#include "util/hashMap.h"
#include "util/stack.h"
#include "util/vector.h"

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
      void *info;  // TODO: figure this out later
    } type;
    struct {
      void *type;  // TODO: figure this out later
    } function;
  } data;
} SymbolInfo;
SymbolInfo *symbolInfoCreate(void);
void symbolInfoDestroy(SymbolInfo *);

// symbol table for a module
// specialsiation of a generic
typedef HashMap SymbolTable;
SymbolTable *symbolTableCreate(void);
SymbolInfo *symbolTableGet(SymbolTable const *, char const *key);
int symbolTablePut(SymbolTable *, char const *key, SymbolInfo *value);
void symbolTableDestroy(SymbolTable *);

typedef struct {
  Vector *imports;  // vector of symbol tables
  SymbolTable *currentModule;
  Stack *scopes;  // stack of symbol tables
} Environment;
Environment *environmentCreate(SymbolTable *currentModule);
SymbolInfo *environmentLookup(Environment const *, char const *);
void environmentDestroy(Environment *);

#endif  // TLC_SYMBOLTABLE_SYMBOLTABLE_H_