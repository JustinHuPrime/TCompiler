// Copyright 2020 Justin Hu
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

/**
 * @file
 * A java-style generic hash map between char const *keys and void *values
 */

#ifndef TLC_UTIL_CONTAINER_HASHMAP_H_
#define TLC_UTIL_CONTAINER_HASHMAP_H_

#include <stddef.h>

/** A hash table between a string (not owned) and a value pointer */
typedef struct {
  size_t size;
  size_t capacity;
  char const **keys;
  void **values;
} HashMap;

/**
 * initialize map in-place
 *
 * @param map map to initialize
 */
void hashMapInit(HashMap *map);

/**
 * Returns the value, or NULL if the key is not in the table. Constant time
 * operation
 *
 * @param map map to search in
 * @param key key to search for
 */
void *hashMapGet(HashMap const *map, char const *key);

/**
 * Tries to insert a key into the table. Note that key is not owned by the
 * table, but the node is. Amortized constant time operation
 *
 * @param map map to insert into
 * @param key key to insert
 * @param value value to insert
 * @returns 0 if inserertion is successful, -1 if the key exists
 */
int hashMapPut(HashMap *map, char const *key, void *value);

/**
 * Sets a key in the table, if it doesn't exist, adds it. Always succeeds,
 * amortized constant time operation
 *
 * @param map map to add/set in
 * @param key key to add/set
 * @param value value to add/set
 */
void hashMapSet(HashMap *map, char const *key, void *value);

/**
 * deinitialize map in-place
 *
 * @param map map to deinitialize
 * @param dtor function pointer to call on values
 */
void hashMapUninit(HashMap *map, void (*dtor)(void *));

#endif  // TLC_UTIL_CONTAINER_HASHMAP_H_