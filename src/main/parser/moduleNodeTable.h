// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Storage used for abstract syntax trees

#ifndef TLC_PARSER_MODULENODETABLE_H_
#define TLC_PARSER_MODULENODETABLE_H_

#include "ast/ast.h"
#include "util/hashMap.h"

// specialization of a generic
typedef HashMap ModuleNodeTable;

// ctor
ModuleNodeTable *moduleNodeTableCreate(void);

// get
// returns the node, or NULL if the key is not in the table
Node *moduleNodeTableGet(ModuleNodeTable *, char const *key);

// put - note that key is not owned by the table, but the node is
// returns: HT_OK if the insertion was successful
//          HT_EEXISTS if the key exists
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