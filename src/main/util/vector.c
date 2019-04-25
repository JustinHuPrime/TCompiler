// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of vector of pointers, in java generic style

#include "util/vector.h"

#include <stdlib.h>

Vector *vectorCreate(void) {
  Vector *vector = malloc(sizeof(Vector));
  vector->size = 0;
  vector->capacity = 1;
  vector->elements = malloc(sizeof(void *));
  return vector;
}
void vectorInsert(Vector *vector, void *element) {
  if (vector->size == vector->capacity) {
    vector->capacity *= 2;
    vector->elements =
        realloc(vector->elements, vector->capacity * sizeof(void *));
  }
  vector->elements[vector->size++] = element;
}
void vectorDestroy(Vector *vector, void (*dtor)(void *)) {
  for (size_t idx = 0; idx < vector->size; idx++) dtor(vector->elements[idx]);
  free(vector->elements);
  free(vector);
}