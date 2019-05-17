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

// Implementation of generic hash set

#include "util/hashSet.h"
#include "util/hash.h"

#include <stdlib.h>
#include <string.h>

HashSet *hashSetCreate(void) {
  HashSet *set = malloc(sizeof(HashSet));
  set->capacity = 1;
  set->size = 0;
  set->elements = calloc(1, sizeof(char *));
  return set;
}

bool hashSetContains(HashSet const *set, char const *element) {
  uint64_t hash = djb2(element);
  hash %= set->capacity;

  if (set->elements[hash] == NULL) {
    return false;                                          // not found
  } else if (strcmp(set->elements[hash], element) != 0) {  // collision
    uint64_t hash2 = djb2add(element) + 1;
    for (size_t idx = (hash + hash2) % set->capacity; idx != hash;
         idx = (idx + hash2) % set->capacity) {
      if (set->elements[idx] == NULL) {
        return false;
      } else if (strcmp(set->elements[idx], element) == 0) {  // found it!
        return true;
      }
    }
    return false;  // searched everywhere, couldn't find it
  } else {         // found it
    return true;
  }
}

int const HS_OK = 0;
int const HS_EEXISTS = 1;
int hashSetAdd(HashSet *set, char *element, bool freeString) {
  uint64_t hash = djb2(element);
  hash %= set->capacity;

  if (set->elements[hash] == NULL) {
    set->elements[hash] = element;
    set->size++;
    return HS_OK;                                          // empty spot
  } else if (strcmp(set->elements[hash], element) != 0) {  // collision
    uint64_t hash2 = djb2add(element) + 1;
    for (size_t idx = (hash + hash2) % set->capacity; idx != hash;
         idx = (idx + hash2) % set->capacity) {
      if (set->elements[idx] == NULL) {  // empty spot
        set->elements[hash] = element;
        set->size++;
        return HS_OK;
      } else if (strcmp(set->elements[idx], element) ==
                 0) {  // already in there
        if (freeString) free(element);
        return HS_EEXISTS;
      }
    }
    size_t oldCap = set->capacity;
    char **oldElements = set->elements;
    set->capacity *= 2;
    set->elements = calloc(set->capacity, sizeof(char const *));
    set->size = 0;
    for (size_t idx = 0; idx < oldCap; idx++) {
      if (oldElements[idx] != NULL) {
        // can't possibly fail, and better to leak than to crash
        hashSetAdd(set, oldElements[idx], false);
      }
    }
    free(oldElements);
    return hashSetAdd(set, element, freeString);  // element isn't in set
  } else {                                        // already in there
    if (freeString) free(element);
    return HS_EEXISTS;
  }
}

void hashSetDestroy(HashSet *set, bool freeStrings) {
  if (freeStrings) {
    for (size_t idx = 0; idx < set->size; idx++) {
      if (set->elements[idx] != NULL) {
        free(set->elements[idx]);
      }
    }
  }
  free(set->elements);
  free(set);
}