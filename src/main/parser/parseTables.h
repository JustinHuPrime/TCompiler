// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tables used by the parser

#ifndef TLC_PARSER_PARSETABLES_H_
#define TLC_PARSER_PARSETABLES_H_

#include "util/hashMap.h"

// a hash map between module names and filenames
// specialization of a generic
typedef HashMap ModuleFileTable;
// ctor
ModuleFileTable *moduleFileTableCreate(void);
// get
// returns the filename, or NULL if the key is not in the table
char const *moduleFileTableGet(ModuleFileTable *, char const *key);
// put - note that key is not owned by the table, but the node is
// returns: HT_OK if the insertion was successful
//          HT_EEXISTS if the key exists
int moduleFileTablePut(ModuleFileTable *, char const *key, char const *data);
// dtor
void moduleFileTableDestroy(ModuleFileTable *);

#endif  // TLC_PARSER_PARSETABLES_H_