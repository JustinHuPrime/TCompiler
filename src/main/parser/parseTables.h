// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tables used by the parser

#ifndef TLC_PARSER_PARSETABLES_H_
#define TLC_PARSER_PARSETABLES_H_

#include "dependencyGraph/grapher.h"
#include "util/hashMap.h"
#include "util/hashSet.h"

// hash map b/w module name and provided types.
// This map owns the sets, which do not own the pointers
typedef HashMap TypenameSetTable;
// ctor
TypenameSetTable *typenameSetTableCreate(void);
// get
NonOwningHashSet *typenameSetTableGet(TypenameSetTable *, char const *key);
// put
int typenameSetTablePut(TypenameSetTable *, char const *key,
                        NonOwningHashSet *data);
// destroy
void typenameSetTableDestroy(TypenameSetTable *);

// holds data related to the current typenames and non-typenames
typedef struct {
  ModuleInfo const *info;
  NonOwningHashSet *moduleOverrides;
  NonOwningHashSet *typenames;

  size_t overrideStackSize;
  size_t overrideStackCapacity;
  NonOwningHashSet **overrideStack;

  size_t numImports;
  NonOwningHashSet **importedTypnames;
} Environment;

// ctor - sets up the environment for a particular module
Environment *environmentCreate(ModuleInfo *);
// adds a layer of override
void environmentPush(Environment *);
// removes a layer of override, destroying the non-owning hashSet
void environmentPop(Environment *);
// searches for a name in the environment
bool environmentIsTypePlain(Environment *, char const *name);
bool environmentIsTypeScoped(Environment *, char const *name);
// dtor - returns the typenames to put in the typename set table
// override stack is free'd, imported typenames are not owned.
NonOwningHashSet *environmentDestroy(Environment *);

#endif  // TLC_PARSER_PARSETABLES_H_