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

// Hash functions

#ifndef TLC_UTIL_HASH_H_
#define TLC_UTIL_HASH_H_

#include <stdint.h>

// uses the djb2 hash function, xor variant, to hash a string
uint64_t djb2(char const *);

// uses the djb2 hash function, addition variant, to hash a string
uint64_t djb2add(char const *);

#endif  // TLC_UTIL_HASH_H_