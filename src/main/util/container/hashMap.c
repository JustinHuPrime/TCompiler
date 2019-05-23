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

// Implementation of generic hash map

#include "util/container/hashMap.h"
#include "util/hash.h"

#include <stdlib.h>
#include <string.h>

HashMap *hashMapCreate(void) {
  HashMap *map = malloc(sizeof(HashMap));
  hashMapInit(map);
  return map;
}
void hashMapInit(HashMap *map) {
  map->size = 0;
  map->capacity = 1;
  map->keys = calloc(1, sizeof(char const *));
  map->values = malloc(sizeof(void *));
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

int const HM_OK = 0;
int const HM_EEXISTS = 1;
int hashMapPut(HashMap *map, char const *key, void *data,
               void (*dtor)(void *)) {
  uint64_t hash = djb2(key) % map->capacity;

  if (map->keys[hash] == NULL) {
    map->keys[hash] = key;
    map->values[hash] = data;
    map->size++;
    return HM_OK;                                  // empty spot
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key) + 1;
    for (size_t idx = (hash + hash2) % map->capacity; idx != hash;
         idx = (idx + hash2) % map->capacity) {
      if (map->keys[idx] == NULL) {  // empty spot
        map->keys[idx] = key;
        map->values[idx] = data;
        map->size++;
        return HM_OK;
      } else if (strcmp(map->keys[idx], key) == 0) {  // already in there
        dtor(data);
        return HM_EEXISTS;
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
    return hashMapPut(map, key, data, dtor);  // recurse
  } else {                                    // already in there
    dtor(data);
    return HM_EEXISTS;
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
void hashMapDestroy(HashMap *map, void (*dtor)(void *)) {
  hashMapUninit(map, dtor);
  free(map);
}