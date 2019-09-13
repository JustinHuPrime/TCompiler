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

// tests for vector

#include "util/container/vector.h"

#include "unitTests/tests.h"
#include "util/container/optimization.h"
#include "util/functional.h"

void vectorTest(TestStatus *status) {
  Vector *v = vectorCreate();
  test(status, "[util] [vector] [ctor] ctor produces size zero", v->size == 0);
  test(status,
       "[util] [vector] [ctor] ctor produces capacity PTR_VECTOR_INIT_CAPACITY",
       v->capacity == PTR_VECTOR_INIT_CAPACITY);
  test(status, "[util] [vector] [ctor] ctor produces non-null elements array",
       v->elements != NULL);

  for (size_t idx = 0; idx < PTR_VECTOR_INIT_CAPACITY - 1; idx++) {
    vectorInsert(v, (void *)0);
  }
  vectorInsert(v, (void *)1);
  test(status, "[util] [vector] [vectorInsert] insertion changes size",
       v->size == PTR_VECTOR_INIT_CAPACITY);
  test(status,
       "[util] [vector] [vectorInsert] insertion doesn't change capacity if "
       "not full",
       v->capacity == PTR_VECTOR_INIT_CAPACITY);
  test(status,
       "[util] [vector] [vectorInsert] inserted element is in the appropriate "
       "slot",
       v->elements[v->size - 1] == (void *)1);

  vectorInsert(v, (void *)2);
  test(status, "[util] [vector] [vectorInsert] insertion changes size",
       v->size == PTR_VECTOR_INIT_CAPACITY + 1);
  test(status,
       "[util] [vector] [vectorInsert] insertion changes capacity if full",
       v->capacity == PTR_VECTOR_INIT_CAPACITY * 2);
  test(status,
       "[util] [vector] [vectorInsert] inserted element is in the appropriate "
       "slot",
       v->elements[v->size - 1] == (void *)2);
  test(status, "[util] [vector] [vectorInsert] previous element is unchanged",
       v->elements[v->size - 2] == (void *)1);

  vectorDestroy(v, nullDtor);
}