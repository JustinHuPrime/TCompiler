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

// Tests for hashmap

#include "util/container/hashMap.h"

#include "optimization.h"
#include "unitTests/tests.h"
#include "util/format.h"
#include "util/functional.h"

#include <stdlib.h>

void hashMapTest(TestStatus *status) {
  HashMap *map = hashMapCreate();
  test(status,
       "[util] [hashMap] [ctor] ctor produces map with capacity "
       "PTR_VECTOR_INIT_CAPACITY",
       map->capacity == PTR_VECTOR_INIT_CAPACITY);
  test(status, "[util] [hashMap] [ctor] ctor produces map with size zero",
       map->size == 0);
  test(
      status,
      "[util] [hashMap] [ctor] ctor produces map with non-null array of values",
      map->values != NULL);
  test(status,
       "[util] [hashMap] [ctor] ctor produces map with non-null array of keys",
       map->keys != NULL);
  test(status, "[util] [hashMap] [ctor] ctor produces zeroed key array",
       map->keys[0] == NULL);

  char **strings = malloc((PTR_VECTOR_INIT_CAPACITY - 1) * sizeof(char *));
  for (size_t count = 0; count < PTR_VECTOR_INIT_CAPACITY - 1; count++) {
    strings[count] = format("%zu", count);
    hashMapPut(map, strings[count], (void *)0, nullDtor);
  }

  char const *a = "a";
  hashMapPut(map, a, (void *)1, nullDtor);
  test(status,
       "[util] [hashMap] [hashMapPut] put does not update capacity if there is "
       "no collision",
       map->capacity == PTR_VECTOR_INIT_CAPACITY);
  test(status, "[util] [hashMap] [hashMapPut] put updates size properly",
       map->size == PTR_VECTOR_INIT_CAPACITY);

  char const *b = "b";
  hashMapPut(map, b, (void *)2, nullDtor);
  test(status,
       "[util] [hashMap] [hashMapPut] put updates capacity if there is a "
       "collision",
       map->capacity == PTR_VECTOR_INIT_CAPACITY * 2);
  test(status, "[util] [hashMap] [hashMapPut] put updates size properly",
       map->size == PTR_VECTOR_INIT_CAPACITY + 1);

  int retVal = hashMapPut(map, b, (void *)2, nullDtor);
  test(status,
       "[util] [hashMap] [hashMapPut] put produces error if trying to add with "
       "existing key",
       retVal == HM_EEXISTS);
  test(status, "[util] [hashMap] [hashMapPut] bad put doesn't change capacity",
       map->capacity == PTR_VECTOR_INIT_CAPACITY * 2);
  test(status, "[util] [hashMap] [hashMapPut] bad put doesn't change size",
       map->size == PTR_VECTOR_INIT_CAPACITY + 1);

  test(status,
       "[util] [hashMap] [hashMapGet] get returns correct value for existing "
       "key",
       hashMapGet(map, "a") == (void *)1);
  test(status,
       "[util] [hashMap] [hashMapGet] get returns correct value for "
       "nonexistant key",
       hashMapGet(map, "c") == NULL);

  hashMapSet(map, b, (void *)3);
  test(status,
       "[util] [hashMap] [hashMapSet] set doesn't update capacity if there is "
       "no collision",
       map->capacity == PTR_VECTOR_INIT_CAPACITY * 2);
  test(status,
       "[util] [hashMap] [hashMapSet] set doesn't update size if key exists",
       map->size == PTR_VECTOR_INIT_CAPACITY + 1);

  hashMapDestroy(map, nullDtor);

  for (size_t count = 0; count < PTR_VECTOR_INIT_CAPACITY - 1; count++) {
    free(strings[count]);
  }
  free(strings);
}