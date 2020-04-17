// Copyright 2020 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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