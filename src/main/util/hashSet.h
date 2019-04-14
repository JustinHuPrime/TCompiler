// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A set of strings

#ifndef TLC_UTIL_HASHSET_H_
#define TLC_UTIL_HASHSET_H_

#include <stdbool.h>
#include <stddef.h>

// A hash set of strings
typedef struct {
  size_t size;
  char **elements;
} HashSet;

// ctor
HashSet *hashSetCreate(void);

// contains
bool hashSetContains(HashSet *, char const *element);

extern int const HS_OK;
extern int const HS_EXISTS;
// put
// frees the string if it isn't put in to the set if freeString is true.
// returns: HS_OK if the element did not already exist
//          HS_EXISTS if the element did exist
int hashSetAdd(HashSet *, char *element, bool freeString);

// dtor
// if freeStrings is true, then the elements are free'd.
void hashSetDestroy(HashSet *, bool freeStrings);

// alternate names for easier semantics
typedef HashSet NonOwningHashSet;
typedef HashSet OwningHashSet;

#endif  // TLC_UTIL_HASHSET_H_