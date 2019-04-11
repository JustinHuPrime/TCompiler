// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of non-cryptographic hash functions

#include "util/hash.h"

// uses the djb2 hash function, xor variant, to hash a string
uint64_t djb2(char const *s) {
  uint64_t hash = 5381;
  while (*s != '\0') {
    hash *= 33;
    hash ^= (uint64_t)*s;
    s++;
  }
  return hash;
}

// uses the djb2 hash function, addition variant, to hash a string
uint64_t djb2add(char const *s) {
  uint64_t hash = 5381;
  while (*s != '\0') {
    hash *= 33;
    hash += (uint64_t)*s;
    s++;
  }
  return hash;
}