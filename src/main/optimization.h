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

/**
 * @file
 * Container optimization constants
 */

#ifndef TLC_UTIL_CONTAINER_OPTIMIZATION_H_
#define TLC_UTIL_CONTAINER_OPTIMIZATION_H_

#include <stddef.h>

/** starting capacity of a vector of pointers */
extern size_t const PTR_VECTOR_INIT_CAPACITY;
/** starting capacity of a vector of ints */
extern size_t const INT_VECTOR_INIT_CAPACITY;
/** starting capacity of a vector of bytes */
extern size_t const BYTE_VECTOR_INIT_CAPACITY;
/** growth factor of a vector */
extern size_t const VECTOR_GROWTH_FACTOR;

#endif  // TLC_UTIL_CONTAINER_OPTIMIZATION_H_