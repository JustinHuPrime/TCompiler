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

#include "util/bigInteger.h"

#include "optimization.h"

#include <stdio.h>  // FIXME:
#include <stdlib.h>

/**
 * adds a digit to the BigInteger initialized to zero
 *
 * @param integer integer to add digit to
 */
static void bigIntAddDigit(BigInteger *integer) {
  if (integer->size == integer->capacity) {
    integer->capacity *= VECTOR_GROWTH_FACTOR;
    integer->digits =
        realloc(integer->digits, integer->capacity * sizeof(uint32_t));
  }
  integer->digits[integer->size++] = 0;
}

/**
 * clears the nth bit from the least signficant bit
 *
 * @param integer integer to set the bit in
 * @param idx index to set
 */
static void bigIntClearBitAtIndex(BigInteger *integer, size_t idx) {
  integer->digits[idx / 32] &= ~(0x1U << idx % 32);
}

/**
 * adds one to the nth bit
 *
 * assumes that such a bit exists
 *
 * @param integer integer to add to
 * @param n bit to add one to
 */
static void bigIntAddOneToBit(BigInteger *integer, size_t n) {
  uint32_t carry = 1U << n % 32;
  for (size_t idx = n / 32; idx < integer->size; idx++) {
    uint64_t digitResult = integer->digits[idx] + carry;
    integer->digits[idx] = (uint32_t)(digitResult % 0x100000000);
    carry = (uint32_t)(digitResult / 0x100000000);
    if (idx + 1 == integer->size && carry != 0)
      // add a digit to be carried in to
      bigIntAddDigit(integer);
  }
}

void bigIntInit(BigInteger *integer) {
  integer->roundingErrorSign = 0;
  integer->capacity = INT_VECTOR_INIT_CAPACITY;
  integer->size = 1;
  integer->digits = malloc(integer->capacity * sizeof(uint32_t));
  integer->digits[0] = 0;
}

void bigIntMul(BigInteger *integer, uint64_t n) {
  uint64_t carry = 0;
  for (size_t idx = 0; idx < integer->size; idx++) {
    uint64_t digitResult = (uint64_t)integer->digits[idx] * n + carry;
    integer->digits[idx] = (uint32_t)(digitResult % 0x100000000);
    carry = (uint32_t)(digitResult / 0x100000000);
    if (idx + 1 == integer->size && carry != 0)
      // add a digit to be carried in to
      bigIntAddDigit(integer);
  }
}

void bigIntAdd(BigInteger *integer, uint64_t n) {
  uint64_t carry = n;
  for (size_t idx = 0; idx < integer->size; idx++) {
    uint64_t digitResult = (uint64_t)integer->digits[idx] + carry;
    integer->digits[idx] = (uint32_t)(digitResult % 0x100000000);
    carry = (uint32_t)(digitResult / 0x100000000);
    if (idx + 1 == integer->size && carry != 0)
      // add a digit to be carried in to
      bigIntAddDigit(integer);
  }
}

size_t bigIntCountSigBits(BigInteger *integer) {
  if (integer->size == 0) return 0;
  size_t count = 32 * (integer->size - 1);
  for (uint32_t msd = integer->digits[integer->size - 1]; msd != 0; msd >>= 1)
    count++;
  return count;
}

uint8_t bigIntGetBitAtIndex(BigInteger *integer, size_t idx) {
  uint32_t digit = integer->digits[idx / 32];
  return (digit >> idx % 32) & 0x1U;
}

void bigIntRoundToN(BigInteger *integer, size_t n) {
  size_t sigBits = bigIntCountSigBits(integer);
  if (sigBits <= n) return;  // already rounded
  // get a 'bit pointer' to the digits to remove
  size_t cutoffIndex = sigBits - 1 - n;

  printf("DEBUG: cutoffIndex = %zu\n", cutoffIndex);

  uint8_t firstBit = bigIntGetBitAtIndex(integer, cutoffIndex);
  if (firstBit == 0) {
    // if following digits are all zeroes, then no rounding, otherwise round
    // down
    for (size_t offset = 1; offset <= cutoffIndex; offset++) {
      if (bigIntGetBitAtIndex(integer, cutoffIndex - offset) == 1) {
        // round down
        integer->roundingErrorSign = -1;
      }
    }
  } else {
    // may be a round to even
    bool rounded = false;
    for (size_t offset = 1; offset <= cutoffIndex; offset++) {
      if (bigIntGetBitAtIndex(integer, cutoffIndex - offset) != 0) {
        // 0b...1...1... - round up
        integer->roundingErrorSign = 1;
        bigIntAddOneToBit(integer, cutoffIndex + 1);
        rounded = true;
        break;
      }
    }
    if (!rounded) {
      // didn't round, might round to even depending on the error
      if (integer->roundingErrorSign == -1) {
        // rounded down to get here - round up
        bigIntAddOneToBit(integer, cutoffIndex + 1);
        integer->roundingErrorSign = 1;
      } else if (integer->roundingErrorSign == 1) {
        // rounded up to get here - round down
        integer->roundingErrorSign = -1;
      } else {
        // really a round to even
        if (bigIntGetBitAtIndex(integer, cutoffIndex + 1) == 0) {
          // round down
          integer->roundingErrorSign = -1;
        } else {
          // round up
          bigIntAddOneToBit(integer, cutoffIndex + 1);
          integer->roundingErrorSign = 1;
        }
      }
    }
  }

  // zero out the bits
  for (size_t idx = 0; idx <= cutoffIndex; idx++)
    bigIntClearBitAtIndex(integer, idx);
}

uint64_t bigIntGetNBits(BigInteger *integer, size_t n) {
  uint64_t retval = 0x0;
  size_t start = bigIntCountSigBits(integer) - 1;
  for (size_t offset = 0; offset < n; offset++) {
    retval <<= 1;
    retval |= bigIntGetBitAtIndex(integer, start - offset);
  }
  return retval;
}

bool bigIntIsZero(BigInteger *integer) {
  return integer->size == 1 && integer->digits[0] == 0;
}

void bigIntUninit(BigInteger *integer) { free(integer->digits); }