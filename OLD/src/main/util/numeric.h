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

// numeric utilities

#ifndef TLC_UTIL_NUMERIC_H_
#define TLC_UTIL_NUMERIC_H_

#include <stddef.h>
#include <stdint.h>

extern uint32_t const FLOAT_BITS_ONE;
extern uint64_t const DOUBLE_BITS_ONE;
extern uint32_t const FLOAT_BITS_ZERO;
extern uint64_t const DOUBLE_BITS_ZERO;

size_t increaseToMultipleOf(size_t n, size_t m);

#endif  // TLC_UTIL_NUMERIC_H_