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

/**
 * @file
 * A java-style generic vector of void *values
 */

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
 * allocating ctor
 *
 * @returns allocated and initialiezd empty Vector
 */
Vector *vectorCreate(void);
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