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

// Implementation of generic hash map

#include "util/container/hashMap.h"

#include "optimization.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** hash a string using djb2 xor variant */
static uint64_t djb2(char const *s) {
  uint64_t hash = 5381;
  while (*s != '\0') {
    hash *= 33;
    hash ^= (uint64_t)*s;
    s++;
  }
  return hash;
}

/** has a string using djb2 add variant */
static uint64_t djb2add(char const *s) {
  uint64_t hash = 5381;
  while (*s != '\0') {
    hash *= 33;
    hash += (uint64_t)*s;
    s++;
  }
  return hash;
}

void hashMapInit(HashMap *map) {
  map->size = 0;
  map->capacity = PTR_VECTOR_INIT_CAPACITY;
  map->keys = calloc(map->capacity, sizeof(char const *));
  map->values = malloc(map->capacity * sizeof(void *));
}

void *hashMapGet(HashMap const *map, char const *key) {
  uint64_t hash = djb2(key);
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
  uint64_t hash = djb2(key) % map->capacity;

  if (map->keys[hash] == NULL) {
    map->keys[hash] = key;
    map->values[hash] = data;
    map->size++;
    return 0;                                      // empty spot
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key) + 1;
    for (size_t idx = (hash + hash2) % map->capacity; idx != hash;
         idx = (idx + hash2) % map->capacity) {
      if (map->keys[idx] == NULL) {  // empty spot
        map->keys[idx] = key;
        map->values[idx] = data;
        map->size++;
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
    for (size_t idx = 0; idx < oldSize; idx++) {
      if (oldKeys[idx] != NULL) {
        // can set, since we know the elemnt doesn't exist
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
  uint64_t hash = djb2(key) % map->capacity;

  if (map->keys[hash] == NULL) {
    map->keys[hash] = key;
    map->values[hash] = data;
    map->size++;
    return;                                        // empty spot
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key) + 1;
    for (size_t idx = (hash + hash2) % map->capacity; idx != hash;
         idx = (idx + hash2) % map->capacity) {
      if (map->keys[idx] == NULL) {  // empty spot
        map->keys[idx] = key;
        map->values[idx] = data;
        map->size++;
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
    for (size_t idx = 0; idx < oldCap; idx++) {
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
  for (size_t idx = 0; idx < map->capacity; idx++) {
    if (map->keys[idx] != NULL) {
      dtor(map->values[idx]);
    }
  }
  free(map->keys);
  free(map->values);
}