// Copyright 2020 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
 * create a dynamically allocated map
 */
HashMap *hashMapCreate(void);

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