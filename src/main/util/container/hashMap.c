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

// Implementation of generic hash map

#include "util/container/hashMap.h"

#include <stdlib.h>
#include <string.h>

#include "optimization.h"
#include "util/hash.h"

void hashMapInit(HashMap *map) {
  map->size = 0;
  map->capacity = PTR_VECTOR_INIT_CAPACITY;
  map->keys = calloc(map->capacity, sizeof(char const *));
  map->values = malloc(map->capacity * sizeof(void *));
}

void *hashMapGet(HashMap const *map, char const *key) {
  uint64_t hash = djb2xor(key);
  hash %= map->capacity;

  if (map->keys[hash] == NULL) {
    return NULL;                                   // not found
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key) + 1;
    for (size_t idx = (hash + hash2) % map->capacity; idx != hash;
         idx = (idx + hash2) % map->capacity) {
      if (map->keys[idx] == NULL) {
        return NULL;
      } else if (strcmp(map->keys[idx], key) == 0) {  // found it!
        return map->values[idx];
      }
    }
    return NULL;  // searched everywhere, couldn't find it
  } else {        // found it
    return map->values[hash];
  }
}

int hashMapPut(HashMap *map, char const *key, void *data) {
  uint64_t hash = djb2xor(key) % map->capacity;

  if (map->keys[hash] == NULL) {
    map->keys[hash] = key;
    map->values[hash] = data;
    ++map->size;
    return 0;                                      // empty spot
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key) + 1;
    for (size_t idx = (hash + hash2) % map->capacity; idx != hash;
         idx = (idx + hash2) % map->capacity) {
      if (map->keys[idx] == NULL) {  // empty spot
        map->keys[idx] = key;
        map->values[idx] = data;
        ++map->size;
        return 0;
      } else if (strcmp(map->keys[idx], key) == 0) {  // already in there
        return -1;
      }
    }
    size_t oldSize = map->capacity;  // unavoidable collision
    char const **oldKeys = map->keys;
    void **oldValues = map->values;
    map->capacity *= 2;
    map->keys = calloc(map->capacity, sizeof(char const *));
    map->values = malloc(map->capacity * sizeof(void *));  // resize the map
    map->size = 0;
    for (size_t idx = 0; idx < oldSize; ++idx) {
      if (oldKeys[idx] != NULL) {
        // can set, since we know the element doesn't exist
        hashMapSet(map, oldKeys[idx], oldValues[idx]);
      }
    }
    free(oldKeys);
    free(oldValues);
    return hashMapPut(map, key, data);  // recurse
  } else {                              // already in there
    return -1;
  }
}

void hashMapSet(HashMap *map, char const *key, void *data) {
  uint64_t hash = djb2xor(key) % map->capacity;

  if (map->keys[hash] == NULL) {
    map->keys[hash] = key;
    map->values[hash] = data;
    ++map->size;
    return;                                        // empty spot
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key) + 1;
    for (size_t idx = (hash + hash2) % map->capacity; idx != hash;
         idx = (idx + hash2) % map->capacity) {
      if (map->keys[idx] == NULL) {  // empty spot
        map->keys[idx] = key;
        map->values[idx] = data;
        ++map->size;
        return;
      } else if (strcmp(map->keys[idx], key) == 0) {  // already in there
        map->values[idx] = data;
        return;
      }
    }
    size_t oldCap = map->capacity;  // unavoidable collision
    char const **oldKeys = map->keys;
    void **oldValues = map->values;
    map->capacity *= 2;
    map->keys = calloc(map->capacity, sizeof(char const *));
    map->values = malloc(map->capacity * sizeof(void *));  // resize the map
    map->size = 0;
    for (size_t idx = 0; idx < oldCap; ++idx) {
      if (oldKeys[idx] != NULL) {
        hashMapSet(map, oldKeys[idx], oldValues[idx]);
      }
    }
    free(oldKeys);
    free(oldValues);
    hashMapSet(map, key, data);  // recurse
    return;
  } else {  // already in there
    map->values[hash] = data;
    return;
  }
}

void hashMapUninit(HashMap *map, void (*dtor)(void *)) {
  for (size_t idx = 0; idx < map->capacity; ++idx) {
    if (map->keys[idx] != NULL) {
      dtor(map->values[idx]);
    }
  }
  free(map->keys);
  free(map->values);
}