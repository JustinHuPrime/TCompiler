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

// implementation of numeric utilities

#include "util/numeric.h"

// sign 0, exponent 127 - 127, fraction 1.0
// 0b0 01111111 00000000000000000000000
uint32_t const FLOAT_BITS_ONE = 0x3f800000;
// sign 0, exponent 1023 - 1023, fraction 1.0
// 0b0 01111111111 0000000000000000000000000000000000000000000000000000
uint64_t const DOUBLE_BITS_ONE = 0x3ff0000000000000;

size_t increaseToMultipleOf(size_t n, size_t m) {
  return n + ((m - (n % m)) % m);
}