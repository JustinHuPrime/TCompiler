// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of module-node maps

#include "util/hashMap.h"
#include "util/hash.h"

#include <stdlib.h>
#include <string.h>

HashMap *hashMapCreate(void) {
  HashMap *map = malloc(sizeof(HashMap));
  map->size = 1;
  map->keys = calloc(1, sizeof(char const *));
  map->values = malloc(sizeof(void *));
  return map;
}

void *hashMapGet(HashMap *map, char const *key) {
  uint64_t hash = djb2(key);
  hash %= map->size;

  if (map->keys[hash] == NULL) {
    return NULL;                                   // not found
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key);
    for (size_t idx = (hash + hash2) % map->size; idx != hash;
         idx = (idx + hash2) % map->size) {
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
  uint64_t hash = djb2(key);
  hash %= map->size;

  if (map->keys[hash] == NULL) {
    map->keys[hash] = key;
    map->values[hash] = data;
    return HM_OK;                                  // empty spot
  } else if (strcmp(map->keys[hash], key) != 0) {  // collision
    uint64_t hash2 = djb2add(key);
    for (size_t idx = (hash + hash2) % map->size; idx != hash;
         idx = (idx + hash2) % map->size) {
      if (map->keys[idx] == NULL) {  // empty spot
        map->keys[hash] = key;
        map->values[hash] = data;
        return HM_OK;
      } else if (strcmp(map->keys[idx], key) == 0) {  // already in there
        dtor(data);
        return HM_EEXISTS;
      }
    }
    size_t oldSize = map->size;
    char const **oldKeys = map->keys;
    void **oldValues = map->values;
    map->size *= 2;
    map->keys = calloc(map->size, sizeof(char const *));
    map->values = malloc(map->size * sizeof(void *));  // resize the map
    for (size_t idx = 0; idx < oldSize; idx++) {
      if (oldKeys[idx] != NULL) {
        hashMapPut(map, oldKeys[idx], oldValues[idx], dtor);
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

void hashMapDestroy(HashMap *map, void (*dtor)(void *)) {
  for (size_t idx = 0; idx < map->size; idx++) {
    if (map->keys[idx] != NULL) {
      dtor(map->values[idx]);
    }
  }
  free(map->keys);
  free(map->values);
  free(map);
}