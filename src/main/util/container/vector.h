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

// A vector of pointers, in java generic style

#ifndef TLC_UTIL_CONTAINER_VECTOR_H_
#define TLC_UTIL_CONTAINER_VECTOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// a vector of pointers, in java generic style
typedef struct {
  size_t size;
  size_t capacity;
  void **elements;
} Vector;

// ctor
Vector *vectorCreate(void);
// in place ctor
void vectorInit(Vector *);
// copy ctor - takes a function to copy elements
Vector *vectorCopy(Vector *, void *(*elmCopy)(void *));
// insert - amortized constant time
void vectorInsert(Vector *, void *);
// merge - linear with memory used
Vector *vectorMerge(Vector *, Vector *);
// in place dtor
// takes in a destructor function to apply to the elements
void vectorUninit(Vector *, void (*dtor)(void *));
// dtor
void vectorDestroy(Vector *, void (*dtor)(void *));

// vector of strings
typedef Vector StringVector;
// ctor
StringVector *stringVectorCreate(void);
// in place ctor
void stringVectorInit(StringVector *);
// insert
void stringVectorInsert(StringVector *, char *);
// in place dtor
// takes in a destructor function to apply to the elements
void stringVectorUninit(StringVector *, bool freeStrings);
// dtor
void stringVectorDestroy(StringVector *, bool freeStrings);

// vector of size_t
typedef struct {
  size_t size;
  size_t capacity;
  size_t *elements;
} SizeVector;
// ctor
SizeVector *sizeVectorCreate(void);
// in place ctor
void sizeVectorInit(SizeVector *);
// insert
void sizeVectorInsert(SizeVector *, size_t);
// in place dtor
void sizeVectorUninit(SizeVector *);
// dtor
void sizeVectorDestroy(SizeVector *);

#endif  // TLC_UTIL_CONTAINER_VECTOR_H_