// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Hash functions

#ifndef TLC_UTIL_HASH_H_
#define TLC_UTIL_HASH_H_

#include <stdint.h>

// uses the djb2 hash function, xor variant, to hash a string
uint64_t djb2(char const *);

// uses the djb2 hash function, addition variant, to hash a string
uint64_t djb2add(char const *);

#endif  // TLC_UTIL_HASH_H_