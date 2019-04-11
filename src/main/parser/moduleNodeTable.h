// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Storage used for abstract syntax trees

#ifndef TLC_PARSER_MODULENODETABLE_H_
#define TLC_PARSER_MODULENODETABLE_H_

#include "ast/ast.h"

// a hash table entry
typedef struct {
  char const *key;
  Node *value;
} ModuleNodeTableEntry;

// A hash table between a module name and that module's ast
typedef struct {
  size_t size;
  ModuleNodeTableEntry *entries;
} ModuleNodeTable;

// ctor
ModuleNodeTable *moduleNodeTableCreate(void);

// get
// returns the node, or NULL if the key is not in the table
Node *moduleNodeTableGet(ModuleNodeTable *, char const *key);

extern int const MNT_OK;
extern int const MNT_EEXISTS;
// put - note that key is not owned by the table, but the node is
// returns: MNT_OK if the insertion was successful
//          MNT_EEXISTS if the key exists
int moduleNodeTablePut(ModuleNodeTable *, char const *key, Node *data);

// dtor
void moduleNodeTableDestroy(ModuleNodeTable *);

// a pair of tables for decl modules and code modules - POD object
typedef struct {
  ModuleNodeTable *decls;
  ModuleNodeTable *codes;
} ModuleNodeTablePair;

// ctor
ModuleNodeTablePair *moduleNodeTablePairCreate(void);
// dtor
void moduleNodeTablePairDestroy(ModuleNodeTablePair *);

#endif  // TLC_PARSER_MODULENODETABLE_H_