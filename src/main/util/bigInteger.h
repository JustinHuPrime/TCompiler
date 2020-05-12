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
 * big integers - used for string to double conversions
 */

#ifndef TLC_UTIL_BIGNUM_H_
#define TLC_UTIL_BIGNUM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * unsigned BigInteger struct
 */
typedef struct {
  uint32_t *digits;         /**< digits, from least to most significant */
  size_t size;              /**< number of digits */
  size_t capacity;          /**< size allocated */
  int8_t roundingErrorSign; /**< direction of error of last round */
} BigInteger;

/**
 * initializes a BigInteger
 *
 * @param integer integer to initialize
 */
void bigIntInit(BigInteger *integer);
/**
 * multiplies the BigInteger by n
 *
 * @param integer integer to multiply
 * @param n value to multiply by
 */
void bigIntMul(BigInteger *integer, uint64_t n);
/**
 * adds n (as a uint32_t) to the BigInteger
 *
 * @param integer integer to add to
 * @param n value to add
 */
void bigIntAdd(BigInteger *integer, uint64_t n);
/**
 * counts number of non-leading zero bits in the bignum
 *
 * @param integer integer to count bits in
 * @returns number of non-leading zero bits
 */
size_t bigIntCountSigBits(BigInteger *integer);
/**
 * gets the nth bit from the least significant bit
 *
 * @param integer integer to get the bit from
 * @param idx index to get
 */
uint8_t bigIntGetBitAtIndex(BigInteger *integer, size_t idx);
/**
 * round the number to n bits
 *
 * @param integer integer to round
 * @param n number of bits to round to
 */
void bigIntRoundToN(BigInteger *integer, size_t n);
/**
 * get the n most significant, non-leading zero bits
 *
 * assumes that n bits are available
 *
 * @param integer integer to get bits from
 * @param n number of bits to get - less than or equal to 64
 * @returns bits, assembled as unsigned integer and right-aligned
 */
uint64_t bigIntGetNBits(BigInteger *integer, size_t n);
/**
 * returns true if the bigInt is currently representing zero
 *
 * @param integer integer to consider
 * @returns if the integer is zero
 */
bool bigIntIsZero(BigInteger *integer);
/**
 * uninitializes a big integer
 *
 * @param integer integer to uninit
 */
void bigIntUninit(BigInteger *integer);

#endif  // TLC_UTIL_BIGNUM_H_