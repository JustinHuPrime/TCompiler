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

// Tests for hashset

#include "util/hashSet.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void hashSetTest(TestStatus *status) {
  HashSet *set = hashSetCreate();
  test(status, "[util] [hashSet] [ctor] ctor produces set with capacity one",
       set->capacity == 1);
  test(status, "[util] [hashSet] [ctor] ctor produces set with size zero",
       set->size == 0);
  test(status,
       "[util] [hashSet] [ctor] ctor produces set with non-null array of "
       "elements",
       set->elements != NULL);
  test(status, "[util] [hashSet] [ctor] ctor produces zeroed element array",
       set->elements[0] == NULL);

  char *a = strcpy(malloc(2), "a");
  hashSetAdd(set, a, true);
  test(status,
       "[util] [hashSet] [hashSetAdd] put does not update capacity if there is "
       "no collision",
       set->capacity == 1);
  test(
      status,
      "[util] [hashSet] [hashSetAdd] put does adds one to size if it put it in",
      set->size == 1);
  test(status,
       "[util] [hashSet] [hashSetAdd] put inserts element into only slot",
       set->elements[0] == a);

  char *b = strcpy(malloc(2), "b");
  hashSetAdd(set, b, true);
  test(status,
       "[util] [hashSet] [hashSetAdd] put does update capacity if there is a "
       "collision",
       set->size == 2);
  test(
      status,
      "[util] [hashSet] [hashSetAdd] put does adds one to size if it put it in",
      set->size == 2);
  test(
      status,
      "[util] [hashSet] [hashSetAdd] put inserts element into appropriate slot",
      set->elements[1] == b);
  test(
      status,
      "[util] [hashSet] [hashSetAdd] put keeps old element in appropriate slot",
      set->elements[0] == a);

  int retVal = hashSetAdd(set, strcpy(malloc(2), "b"), true);
  test(status,
       "[util] [hashSet] [hashSetAdd] put produces error if trying to add with "
       "existing key",
       retVal == HS_EEXISTS);

  test(status,
       "[util] [hashSet] [hashSetContains] contains returns true value for "
       "existing element",
       hashSetContains(set, "a"));
  test(status,
       "[util] [hashSet] [hashSetContains] contains returns correct value for "
       "nonexisting element",
       !hashSetContains(set, "c"));

  hashSetDestroy(set, true);
}