// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of hash functions

#include "util/hash.h"

#include <stdint.h>

uint64_t djb2(char const *s) {
  uint64_t hash = 5381;
  while (*s != '\0') {
    hash *= 33;
    hash ^= (uint64_t)*s;
    s++;
  }
  return hash;
}

uint64_t djb2add(char const *s) {
  uint64_t hash = 5381;
  while (*s != '\0') {
    hash *= 33;
    hash += (uint64_t)*s;
    s++;
  }
  return hash;
}