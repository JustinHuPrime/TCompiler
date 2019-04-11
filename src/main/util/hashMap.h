// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// An abstract hash map between char const *keys and void *values

#ifndef TLC_UTIL_HASHMAP_H_
#define TLC_UTIL_HASHMAP_H_

#include <stddef.h>

// A hash table between a module name and that module's ast
typedef struct {
  size_t size;
  char const **keys;
  void **values;
} HashMap;

// ctor
HashMap *hashMapCreate(void);

// get
// returns the node, or NULL if the key is not in the table
void *hashMapGet(HashMap *, char const *key);

extern int const HM_OK;
extern int const HM_EEXISTS;
// put - note that key is not owned by the table, but the node is
// takes a dtor function to use on the data if it could not insert the data
// returns: HM_OK if the insertion was successful
//          HM_EEXISTS if the key exists
int hashMapPut(HashMap *, char const *key, void *data, void (*dtor)(void *));

// dtor
// takes a dtor function to free each void pointer
void hashMapDestroy(HashMap *, void (*dtor)(void *));

#endif  // TLC_UTIL_HASHMAP_H_