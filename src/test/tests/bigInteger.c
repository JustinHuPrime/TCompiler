// Copyright 2020-2021 Justin Hu
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
 * tests for bigInteger
 */

#include "util/container/bigInteger.h"

#include <stdlib.h>

#include "engine.h"
#include "tests.h"
#include "util/conversions.h"
#include "util/format.h"
#include "util/random.h"

static void testBigIntegerInit(void) {
  BigInteger integer;
  bigIntInit(&integer);
  test("bigInteger initializes with right rounding error",
       integer.roundingErrorSign == 0);
  test("bigInteger initializes with right size", integer.size == 1);
  test("bigInteger initializes with right value", integer.digits[0] == 0);
  bigIntUninit(&integer);
}

static void testBigIntegerArithmetic(void) {
  srand(0);  // make test deterministic

  bool ok = true;
  for (size_t count = 0; count < 1000; ++count) {
    BigInteger integer;
    bigIntInit(&integer);

    uint64_t number = longRand();
    char *numberString = format("%lu", number);

    for (char *curr = numberString; *curr != '\0'; ++curr) {
      bigIntMul(&integer, 10);
      bigIntAdd(&integer, (uint8_t)(charToU8(*curr) - charToU8('0')));
    }
    free(numberString);

    size_t nbits = 0;
    for (uint64_t copy = number; copy != 0; copy >>= 1) ++nbits;

    uint64_t fromBigInt = bigIntGetNBits(&integer, nbits);
    if (number != fromBigInt) ok = false;

    bigIntUninit(&integer);
  }
  test("bigInteger has the right calculated bits", ok);
}

static size_t countBits(uint64_t n) {
  size_t count = 0;
  while (n != 0) {
    n >>= 1;
    ++count;
  }
  return count;
}

static void testBigIntegerRounding(void) {
  srand(0);  // deterministic tests

  bool ok = true;
  for (size_t count = 0; count < 1000; ++count) {
    BigInteger integer;
    bigIntInit(&integer);

    uint64_t number = longRand() >> 1;
    bigIntAdd(&integer, number);

    size_t nbits = bigIntCountSigBits(&integer);
    size_t roundTo = ((unsigned)rand() % nbits) + 1;

    bigIntRoundToN(&integer, roundTo);
    uint64_t bigIntRounded = bigIntGetNBits(&integer, roundTo);
    uint64_t keepMask = UINT64_MAX << (nbits - roundTo);
    uint64_t removeMask = ~keepMask;
    uint64_t removed = number & removeMask;
    uint64_t kept = number & keepMask;
    uint64_t half = 1UL << (nbits - roundTo - 1);

    // round off removed
    kept >>= nbits - roundTo;
    if (removed > half) {
      // round kept up by one
      kept += 1;
    } else if (removed == half) {
      // round to even
      if (kept % 2 != 0) kept += 1;
    }
    // don't do anything if removed < half
    kept >>= countBits(kept) - roundTo;

    if (bigIntRounded != kept) ok = false;

    bigIntUninit(&integer);
  }
  test("bigInteger has the right rounded bits", ok);
}

void testBigInteger(void) {
  testBigIntegerInit();
  testBigIntegerArithmetic();
  testBigIntegerRounding();
}