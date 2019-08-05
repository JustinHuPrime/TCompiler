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

// Utilities for T-strings (uint8_t/uint32_t based ASCII/UTF-32 null terminated
// character strings)

#ifndef TLC_UTIL_TSTRING_H_
#define TLC_UTIL_TSTRING_H_

#include <stddef.h>
#include <stdint.h>

uint8_t *tstrdup(uint8_t *);
uint32_t *twstrdup(uint32_t *);
size_t tstrlen(uint8_t const *);
size_t twstrlen(uint32_t const *);

#endif  // TLC_UTIL_TSTRING_H_