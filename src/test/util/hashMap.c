// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for hashmap

#include "util/hashMap.h"

#include "tests.h"
#include "util/functional.h"

void hashMapTest(TestStatus *status) {
  HashMap *map = hashMapCreate();
  test(status, "[util] [hashMap] [ctor] ctor produces map with capacity one",
       map->size == 1);
  test(
      status,
      "[util] [hashMap] [ctor] ctor produces map with non-null array of values",
      map->values != NULL);
  test(status,
       "[util] [hashMap] [ctor] ctor produces map with non-null array of keys",
       map->keys != NULL);
  test(status, "[util] [hashMap] [ctor] ctor produces zeroed key array",
       map->keys[0] == NULL);

  char const *a = "a";
  hashMapPut(map, a, (void *)1, nullDtor);
  test(status,
       "[util] [hashMap] [hashMapPut] put does not update size if there is no "
       "collision",
       map->size == 1);
  test(status, "[util] [hashMap] [hashMapPut] put inserts key into only slot",
       map->keys[0] == a);
  test(status, "[util] [hashMap] [hashMapPut] put inserts value into only slot",
       map->values[0] == (void *)1);

  char const *b = "b";
  hashMapPut(map, b, (void *)2, nullDtor);
  test(status,
       "[util] [hashMap] [hashMapPut] put updates size if there is a "
       "collision",
       map->size == 2);
  test(status,
       "[util] [hashMap] [hashMapPut] put inserts key into appropriate slot",
       map->keys[1] == b);
  test(status,
       "[util] [hashMap] [hashMapPut] put inserts value into appropriate slot",
       map->values[1] == (void *)2);
  test(status,
       "[util] [hashMap] [hashMapPut] put keeps old key in appropriate slot",
       map->keys[0] == a);
  test(status,
       "[util] [hashMap] [hashMapPut] put keeps old value in appropriate slot",
       map->values[0] == (void *)1);

  int retVal = hashMapPut(map, b, (void *)2, nullDtor);
  test(status,
       "[util] [hashMap] [hashMapPut] put produces error if trying to add with "
       "existing key",
       retVal == HM_EEXISTS);

  test(status,
       "[util] [hashMap] [hashMapGet] get returns correct value for "
       "existing "
       "key",
       hashMapGet(map, "a") == (void *)1);
  test(status,
       "[util] [hashMap] [hashMapGet] get returns correct value for "
       "nonexistant key",
       hashMapGet(map, "c") == NULL);

  hashMapSet(map, b, (void *)3);
  test(status,
       "[util] [hashMap] [hashMapSet] set doesn't update size if there is no "
       "collision",
       map->size == 2);
  test(status,
       "[util] [hashMap] [hashMapSet] set keeps key in appropriate slot",
       map->keys[1] == b);
  test(status,
       "[util] [hashMap] [hashMapSet] set changes value in appropriate slot",
       map->values[1] == (void *)3);
  test(status,
       "[util] [hashMap] [hashMapSet] set keeps old key in appropriate slot",
       map->keys[0] == a);
  test(status,
       "[util] [hashMap] [hashMapSet] set keeps old value in appropriate slot",
       map->values[0] == (void *)1);

  char const *c = "c";
  hashMapSet(map, c, (void *)4);
  test(status,
       "[util] [hashMap] [hashMapSet] set updates size if there is a collision",
       map->size == 4);
  test(status, "[util] [hashMap] [hashMapSet] set adds key in appropriate slot",
       map->keys[2] == c);
  test(status,
       "[util] [hashMap] [hashMapSet] set adds value in appropriate slot",
       map->values[2] == (void *)4);
  test(status,
       "[util] [hashMap] [hashMapSet] set keeps old key in appropriate slot",
       map->keys[3] == b);
  test(status,
       "[util] [hashMap] [hashMapSet] set keeps old value in appropriate slot",
       map->values[3] == (void *)3);
  test(status,
       "[util] [hashMap] [hashMapSet] set keeps old key in appropriate slot",
       map->keys[0] == a);
  test(status,
       "[util] [hashMap] [hashMapSet] set keeps old value in appropriate slot",
       map->values[0] == (void *)1);

  hashMapDestroy(map, nullDtor);
}