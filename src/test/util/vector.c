// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// tests for vector

#include "util/vector.h"

#include "tests.h"
#include "util/functional.h"

void vectorTest(TestStatus *status) {
  Vector *v = vectorCreate();
  test(status, "[util] [vector] [ctor] ctor produces size zero", v->size == 0);
  test(status, "[util] [vector] [ctor] ctor produces capacity one",
       v->capacity == 1);
  test(status, "[util] [vector] [ctor] ctor produces non-null elements array",
       v->elements != NULL);

  vectorInsert(v, (void *)1);
  test(status, "[util] [vector] [vectorInsert] insertion changes size",
       v->size == 1);
  test(status,
       "[util] [vector] [vectorInsert] insertion doesn't change capacity if "
       "not full",
       v->capacity == 1);
  test(status,
       "[util] [vector] [vectorInsert] inserted element is in the appropriate "
       "slot",
       v->elements[0] == (void *)1);

  vectorInsert(v, (void *)2);
  test(status, "[util] [vector] [vectorInsert] insertion changes size",
       v->size == 2);
  test(status,
       "[util] [vector] [vectorInsert] insertion changes capacity if full",
       v->capacity == 2);
  test(status,
       "[util] [vector] [vectorInsert] inserted element is in the appropriate "
       "slot",
       v->elements[1] == (void *)2);
  test(status, "[util] [vector] [vectorInsert] previous element is unchanged",
       v->elements[0] == (void *)1);

  vectorDestroy(v, nullDtor);
}