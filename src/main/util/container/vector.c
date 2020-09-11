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

// Implementation of vector of pointers, in java generic style

#include "util/container/vector.h"

#include <stdlib.h>

#include "optimization.h"

void vectorInit(Vector *vector) {
  vector->size = 0;
  vector->capacity = PTR_VECTOR_INIT_CAPACITY;
  vector->elements = malloc(vector->capacity * sizeof(void *));
}
Vector *vectorCreate(void) {
  Vector *v = malloc(sizeof(Vector));
  vectorInit(v);
  return v;
}
void vectorInsert(Vector *vector, void *element) {
  if (vector->size == vector->capacity) {
    vector->capacity *= VECTOR_GROWTH_FACTOR;  // using exponential growth
    vector->elements =
        realloc(vector->elements, vector->capacity * sizeof(void *));
  }
  vector->elements[vector->size++] = element;
}
void vectorUninit(Vector *vector, void (*dtor)(void *)) {
  for (size_t idx = 0; idx < vector->size; ++idx) dtor(vector->elements[idx]);
  free(vector->elements);
}