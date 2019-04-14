// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of generic hash set

#include "util/hashSet.h"
#include "util/hash.h"

#include <stdlib.h>
#include <string.h>

HashSet *hashSetCreate(void) {
  HashSet *set = malloc(sizeof(HashSet));
  set->size = 1;
  set->elements = calloc(1, sizeof(char *));
  return set;
}

bool hashSetContains(HashSet *set, char const *element) {
  uint64_t hash = djb2(element);
  hash %= set->size;

  if (set->elements[hash] == NULL) {
    return false;                                          // not found
  } else if (strcmp(set->elements[hash], element) != 0) {  // collision
    uint64_t hash2 = djb2add(element) + 1;
    for (size_t idx = (hash + hash2) % set->size; idx != hash;
         idx = (idx + hash2) % set->size) {
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
int const HS_EXISTS = 1;
int hashSetAdd(HashSet *set, char *element, bool freeString) {
  uint64_t hash = djb2(element);
  hash %= set->size;

  if (set->elements[hash] == NULL) {
    set->elements[hash] = element;
    return HS_OK;                                          // empty spot
  } else if (strcmp(set->elements[hash], element) != 0) {  // collision
    uint64_t hash2 = djb2add(element) + 1;
    for (size_t idx = (hash + hash2) % set->size; idx != hash;
         idx = (idx + hash2) % set->size) {
      if (set->elements[idx] == NULL) {  // empty spot
        set->elements[hash] = element;
        return HS_OK;
      } else if (strcmp(set->elements[idx], element) ==
                 0) {  // already in there
        if (freeString) free(element);
        return HS_EXISTS;
      }
    }
    size_t oldSize = set->size;
    char **oldElements = set->elements;
    set->size *= 2;
    set->elements = calloc(set->size, sizeof(char const *));
    for (size_t idx = 0; idx < oldSize; idx++) {
      if (oldElements[idx] != NULL) {
        // can't possibly fail, and better to leak than to crash
        hashSetAdd(set, oldElements[idx], false);
      }
    }
    free(oldElements);
    return hashSetAdd(set, element, freeString);  // element isn't in set
  } else {                                        // already in there
    if (freeString) free(element);
    return HS_EXISTS;
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