// Copyright 2020 Justin Hu
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

// A vector of pointers, in java generic style

#ifndef TLC_UTIL_CONTAINER_VECTOR_H_
#define TLC_UTIL_CONTAINER_VECTOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** a vector of pointers, in java generic style */
typedef struct {
  size_t size;
  size_t capacity;
  void **elements;
} Vector;

/**
 * in place ctor
 *
 * @param v Vector to initialize
 */
void vectorInit(Vector *v);
/**
 * insert - amortized constant time
 *
 * @param v Vector to add to
 * @param elm element to add
 */
void vectorInsert(Vector *v, void *elm);
/**
 * in place dtor
 *
 * @param v Vector to deinitialize
 * @param dtor function pointer to call on elements
 */
void vectorUninit(Vector *v, void (*dtor)(void *));

#endif  // TLC_UTIL_CONTAINER_VECTOR_H_