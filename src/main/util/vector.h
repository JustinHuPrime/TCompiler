// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A vector of pointers, in java generic style

#ifndef TLC_UTIL_VECTOR_H_
#define TLC_UTIL_VECTOR_H_

#include <stddef.h>

// vector, in java generic style
typedef struct {
  size_t size;
  size_t capacity;
  void **elements;
} Vector;

// ctor
Vector *vectorCreate(void);
// insert
void vectorInsert(Vector *, void *);
// dtor
// takes in a destructor function to apply to the elements
void vectorDestroy(Vector *, void (*)(void *));

#endif  // TLC_UTIL_VECTOR_H_