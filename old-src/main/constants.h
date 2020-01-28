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

// declaration of numerical an other constants

#ifndef TLC_CONSTANTS_H_
#define TLC_CONSTANTS_H_

#include <stddef.h>
#include <stdint.h>

// sizeof fundamental data types
extern size_t const BYTE_WIDTH;  // should always be 1, included for consistency
extern size_t const SHORT_WIDTH;     // 2 on 64 bit, 2 on 32 bit
extern size_t const INT_WIDTH;       // 4 on 64 bit, 4 on 32 bit
extern size_t const LONG_WIDTH;      // 8 on 64 bit, 4 on 32 bit
extern size_t const FLOAT_WIDTH;     // 4 on 64 bit, 4 on 32 bit
extern size_t const DOUBLE_WIDTH;    // 8 on 64 bit, 4 on 32 bit
extern size_t const POINTER_WIDTH;   // 8 on 64 bit, 4 on 32 bit
extern size_t const CHAR_WIDTH;      // should always be 1
extern size_t const WCHAR_WIDTH;     // should always be 4
extern size_t const REGISTER_WIDTH;  // 8 on 64 bit, 4 on 32 bit

// absolute value of limits for some data type
extern uint64_t const UBYTE_MAX;
extern uint64_t const BYTE_MAX;
extern uint64_t const BYTE_MIN;
extern uint64_t const USHORT_MAX;
extern uint64_t const SHORT_MAX;
extern uint64_t const SHORT_MIN;
extern uint64_t const UINT_MAX;
extern uint64_t const INT_MAX;
extern uint64_t const INT_MIN;
extern uint64_t const ULONG_MAX;
extern uint64_t const LONG_MAX;
extern uint64_t const LONG_MIN;

// compiler version
extern char const *VERSION_STRING;

#endif  // TLC_CONSTANTS_H_