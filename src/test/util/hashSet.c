// Copyright 2019 Justin Hu
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
       set->size == 1);
  test(status,
       "[util] [hashSet] [ctor] ctor produces set with non-null array of "
       "elements",
       set->elements != NULL);
  test(status, "[util] [hashSet] [ctor] ctor produces zeroed element array",
       set->elements[0] == NULL);

  char *a = strcpy(malloc(2), "a");
  hashSetAdd(set, a, true);
  test(status,
       "[util] [hashSet] [hashSetAdd] put does not update size if there is no "
       "collision",
       set->size == 1);
  test(status,
       "[util] [hashSet] [hashSetAdd] put inserts element into only slot",
       set->elements[0] == a);

  char *b = strcpy(malloc(2), "b");
  hashSetAdd(set, b, true);
  test(status,
       "[util] [hashSet] [hashSetAdd] put does updates size if there is a "
       "collision",
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