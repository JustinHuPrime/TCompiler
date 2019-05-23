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

// An java-style generic hash map between char const *keys and void *values

#ifndef TLC_UTIL_CONTAINER_HASHMAP_H_
#define TLC_UTIL_CONTAINER_HASHMAP_H_

#include <stddef.h>

// A hash table between a string and a value pointer
typedef struct {
  size_t size;
  size_t capacity;
  char const **keys;
  void **values;
} HashMap;

// ctor
HashMap *hashMapCreate(void);
// in-place ctor
void hashMapInit(HashMap *);

// get
// returns the node, or NULL if the key is not in the table
void *hashMapGet(HashMap const *, char const *key);

extern int const HM_OK;
extern int const HM_EEXISTS;
// put - note that key is not owned by the table, but the node is
// takes a dtor function to use on the data if it could not insert the data
// returns: HM_OK if the insertion was successful
//          HM_EEXISTS if the key exists
int hashMapPut(HashMap *, char const *key, void *value, void (*dtor)(void *));

// set - sets a key in the table, if it doesn't exist, adds it.
// always succeeds
void hashMapSet(HashMap *, char const *key, void *value);

// dtor
// takes a dtor function to free each void pointer
// in place dtor
void hashMapUninit(HashMap *, void (*dtor)(void *));
// regular dtor
void hashMapDestroy(HashMap *, void (*dtor)(void *));

#endif  // TLC_UTIL_CONTAINER_HASHMAP_H_