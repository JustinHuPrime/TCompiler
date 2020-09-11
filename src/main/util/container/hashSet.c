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

#include "util/container/hashSet.h"

#include <stdlib.h>
#include <string.h>

#include "optimization.h"
#include "util/hash.h"

void hashSetInit(HashSet *set) {
  set->size = 0;
  set->capacity = PTR_VECTOR_INIT_CAPACITY;
  set->elements = calloc(set->capacity, sizeof(char const *));
}

bool hashSetContains(HashSet const *set, char const *s) {
  uint64_t hash = djb2xor(s);
  hash %= set->capacity;

  if (set->elements[hash] == NULL) {
    return false;                                    // not found
  } else if (strcmp(set->elements[hash], s) != 0) {  // collision
    uint64_t hash2 = djb2add(s) + 1;
    for (size_t idx = (hash + hash2) % set->capacity; idx != hash;
         idx = (idx + hash2) % set->capacity) {
      if (set->elements[idx] == NULL) {
        return false;
      } else if (strcmp(set->elements[idx], s) == 0) {  // found it!
        return true;
      }
    }
    return false;  // searched everywhere, couldn't find it
  } else {         // found it
    return true;
  }
}

int hashSetPut(HashSet *set, char const *s) {
  uint64_t hash = djb2xor(s) % set->capacity;

  if (set->elements[hash] == NULL) {
    set->elements[hash] = s;
    set->size++;
    return 0;                                        // empty spot
  } else if (strcmp(set->elements[hash], s) != 0) {  // collision
    uint64_t hash2 = djb2add(s) + 1;
    for (size_t idx = (hash + hash2) % set->capacity; idx != hash;
         idx = (idx + hash2) % set->capacity) {
      if (set->elements[idx] == NULL) {  // empty spot
        set->elements[idx] = s;
        set->size++;
        return 0;
      } else if (strcmp(set->elements[idx], s) == 0) {  // already in there
        return -1;
      }
    }
    size_t oldSize = set->capacity;  // unavoidable collision
    char const **oldElements = set->elements;
    set->capacity *= 2;
    set->elements = calloc(set->capacity, sizeof(char const *));
    set->elements = malloc(set->capacity * sizeof(void *));  // resize the set
    set->size = 0;
    for (size_t idx = 0; idx < oldSize; idx++) {
      if (oldElements[idx] != NULL) hashSetPut(set, oldElements[idx]);
    }
    free(oldElements);
    return hashSetPut(set, s);  // recurse
  } else {                      // already in there
    return -1;
  }
}

void hashSetUninit(HashSet *set) { free(set->elements); }