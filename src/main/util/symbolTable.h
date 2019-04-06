// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#ifndef TLC_UTIL_SYMBOLTABLE_H_
#define TLC_UTIL_SYMBOLTABLE_H_

#include "ast/ast.h"

#include <stddef.h>

typedef enum {
  ST_STRUCT,
  ST_UNION,
  ST_ENUM,
  ST_TYPEDEF,
  ST_VAR,
} SymbolType;

typedef struct {
  char *symbolName;
  SymbolType type;
  Node *const data;
} SymbolTableEntry;

typedef struct {
  size_t size;
  size_t capacity;
  SymbolTableEntry *entries;
} SymbolTable;

SymbolTable *symbolTableCreate(void);
void symbolTableInsert(SymbolTable *);
SymbolTableEntry *symbolTableLookup(SymbolTable *, char const *);
void symbolTableDestroy(SymbolTable *);

#endif  // TLC_UTIL_SYMBOLTABLE_H_