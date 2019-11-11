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

// Implementation of vector of pointers, in java generic style

#include "util/container/vector.h"

#include "optimization.h"
#include "util/functional.h"

#include <stdlib.h>
#include <string.h>

Vector *vectorCreate(void) {
  Vector *vector = malloc(sizeof(Vector));
  vectorInit(vector);
  return vector;
}
void vectorInit(Vector *vector) {
  vector->size = 0;
  vector->capacity = PTR_VECTOR_INIT_CAPACITY;
  vector->elements = malloc(vector->capacity * sizeof(void *));
}
Vector *vectorCopy(Vector *v, void *(*elmCopy)(void *)) {
  Vector *vector = malloc(sizeof(Vector));
  vector->size = v->size;
  vector->capacity = v->capacity;
  vector->elements = malloc(vector->capacity * sizeof(void *));
  for (size_t idx = 0; idx < vector->size; idx++) {
    vector->elements[idx] = elmCopy(v->elements[idx]);
  }
  return vector;
}
void vectorInsert(Vector *vector, void *element) {
  if (vector->size == vector->capacity) {
    vector->capacity *= 2;  // using exponential growth
    vector->elements =
        realloc(vector->elements, vector->capacity * sizeof(void *));
  }
  vector->elements[vector->size++] = element;
}
Vector *vectorMerge(Vector *v1, Vector *v2) {
  Vector *out = malloc(sizeof(Vector));
  out->size = v1->size + v2->size;
  out->capacity = v1->capacity > v2->capacity ? v1->capacity : v2->capacity;
  while (out->capacity < out->size) {
    out->capacity *= 2;
  }
  out->elements = malloc(out->capacity * sizeof(void *));

  // copy data
  memcpy(out->elements, v1->elements, v1->size * sizeof(void *));
  memcpy(out->elements + v1->size * sizeof(void *), v2->elements,
         v2->size * sizeof(void *));

  // clean up v1 and v2
  free(v1->elements);
  free(v1);
  free(v2->elements);
  free(v2);

  return out;
}
void vectorUninit(Vector *vector, void (*dtor)(void *)) {
  for (size_t idx = 0; idx < vector->size; idx++) dtor(vector->elements[idx]);
  free(vector->elements);
}
void vectorDestroy(Vector *vector, void (*dtor)(void *)) {
  vectorUninit(vector, dtor);
  free(vector);
}

StringVector *stringVectorCreate(void) { return vectorCreate(); }
void stringVectorInit(StringVector *v) { vectorInit(v); }
void stringVectorInsert(StringVector *v, char *s) { vectorInsert(v, s); }
void stringVectorUninit(StringVector *v, bool freeStrings) {
  vectorUninit(v, freeStrings ? free : nullDtor);
}
void stringVectorDestroy(StringVector *v, bool freeStrings) {
  vectorDestroy(v, freeStrings ? free : nullDtor);
}

SizeVector *sizeVectorCreate(void) {
  SizeVector *v = malloc(sizeof(SizeVector));
  sizeVectorInit(v);
  return v;
}
void sizeVectorInit(SizeVector *v) {
  v->size = 0;
  v->capacity = PTR_VECTOR_INIT_CAPACITY;
  v->elements = malloc(v->capacity * sizeof(size_t));
}
void sizeVectorInsert(SizeVector *v, size_t d) {
  if (v->size == v->capacity) {
    v->capacity *= 2;
    v->elements = realloc(v->elements, v->capacity * sizeof(size_t));
  }
  v->elements[v->size++] = d;
}
bool sizeVectorContains(SizeVector *v, size_t d) {
  for (size_t idx = 0; idx < v->size; idx++) {
    if (v->elements[idx] == d) {
      return true;
    }
  }
  return false;
}
void sizeVectorUninit(SizeVector *v) { free(v->elements); }
void sizeVectorDestroy(SizeVector *v) {
  sizeVectorUninit(v);
  free(v);
}