// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A vector-style symbol table

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

// Constructor
SymbolTable *symbolTableCreate(void);

// Inserts an entry into the table with the given name, and returns a pointer to
// the inserted entry
SymbolTableEntry *symbolTableInsert(SymbolTable *, char *name);
// Finds an entry in the table, and returns it. Returns NULL if no entry could
// be found
SymbolTableEntry *symbolTableLookup(SymbolTable *, char const *name);

// Destructor
void symbolTableDestroy(SymbolTable *);

#endif  // TLC_UTIL_SYMBOLTABLE_H_