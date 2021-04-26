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

// global constants

#include "util/numericSizing.h"

size_t const BYTE_WIDTH = 1;
size_t const SHORT_WIDTH = 2;
size_t const INT_WIDTH = 4;
size_t const LONG_WIDTH = 8;
size_t const FLOAT_WIDTH = 4;
size_t const DOUBLE_WIDTH = 8;
size_t const POINTER_WIDTH = 8;
size_t const CHAR_WIDTH = 1;
size_t const WCHAR_WIDTH = 4;

uint64_t const UBYTE_MAX = 255;
uint64_t const BYTE_MAX = 127;
uint64_t const BYTE_MIN = 128;
uint64_t const USHORT_MAX = 65535;
uint64_t const SHORT_MAX = 32767;
uint64_t const SHORT_MIN = 32768;
uint64_t const UINT_MAX = 4294967295;
uint64_t const INT_MAX = 2147483647;
uint64_t const INT_MIN = 2147483648;
uint64_t const ULONG_MAX = 18446744073709551615UL;
uint64_t const LONG_MAX = 9223372036854775807;
uint64_t const LONG_MIN = 9223372036854775808UL;

size_t const FLOAT_MANTISSA_BITS = 23;
int64_t const FLOAT_EXPONENT_MAX = 127;
int64_t const FLOAT_EXPONENT_MIN = -126;
int64_t const FLOAT_EXPONENT_MIN_SUBNORMAL = -150;

size_t const DOUBLE_MANTISSA_BITS = 52;
int64_t const DOUBLE_EXPONENT_MAX = 1023;
int64_t const DOUBLE_EXPONENT_MIN = -1022;
int64_t const DOUBLE_EXPONENT_MIN_SUBNORMAL = -1075;