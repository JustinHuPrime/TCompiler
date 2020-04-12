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
 * numeric size and limit constants
 */

#ifndef TLC_NUMERICSIZING_H_
#define TLC_NUMERICSIZING_H_

#include <stddef.h>
#include <stdint.h>

/** sizeof fundamental data types - number of bits in a byte must be 8 */
extern size_t const BYTE_WIDTH;
extern size_t const SHORT_WIDTH;
extern size_t const INT_WIDTH;
extern size_t const LONG_WIDTH;
extern size_t const FLOAT_WIDTH;
extern size_t const DOUBLE_WIDTH;
extern size_t const POINTER_WIDTH;
extern size_t const CHAR_WIDTH;
extern size_t const WCHAR_WIDTH;

/** absolute value of limits for some data type */
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

/** number of bits in floating point numbers */
extern size_t const FLOAT_MANTISSA_BITS;
extern int64_t const FLOAT_EXPONENT_MAX;
extern int64_t const FLOAT_EXPONENT_MIN;
extern int64_t const FLOAT_EXPONENT_MIN_SUBNORMAL;

extern size_t const DOUBLE_MANTISSA_BITS;
extern int64_t const DOUBLE_EXPONENT_MAX;
extern int64_t const DOUBLE_EXPONENT_MIN;
extern int64_t const DOUBLE_EXPONENT_MIN_SUBNORMAL;

#endif  // TLC_NUMERICSIZING_H_