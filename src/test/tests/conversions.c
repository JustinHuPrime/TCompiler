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
 * unit tests for numeric conversions
 */

#include "util/conversions.h"

#include "engine.h"
#include "tests.h"
#include "util/format.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char *ignored;

static uint64_t longRand(void) {
  uint64_t high = (uint32_t)rand();
  uint64_t low = (uint32_t)rand();
  return high << 32 | low;
}

static void testNormalFloatConversions(void) {
  srand(0);  // keep tests repeatable

  // test floats
  bool floatOK = true;
  for (size_t count = 0; count < 10000; count++) {
    uint8_t sign = (uint8_t)(rand() % 2);
    uint16_t exponent = (uint16_t)(rand() % 0x100);
    uint64_t mantissa = (uint64_t)(rand() % 0x1000000);

    uint32_t floatBits = ((uint32_t)sign << 31) | ((uint32_t)exponent << 23) |
                         (uint32_t)mantissa;
    float originalValue = bitsToFloat(floatBits);
    if (!isnormal(originalValue)) {  // check for nan and infinities
      count--;
      continue;
    }
    char *stringValue = format("%.46f", (double)originalValue);

    float stdlibValue = strtof(stringValue, &ignored);
    uint32_t stdlibBits = floatToBits(stdlibValue);
    uint32_t conversionBits = floatStringToBits(stringValue);
    if (stdlibBits != conversionBits) floatOK = false;
    free(stringValue);
  }
  test("float parsing succeeded", floatOK);
}

static void testNormalDoubleConversions(void) {
  srand(0);  // keep tests repeatable

  // test floats
  bool doubleOK = true;
  for (size_t count = 0; count < 10000; count++) {
    uint8_t sign = (uint8_t)(rand() % 2);
    uint16_t exponent = (uint16_t)(rand() % 0x800);
    uint64_t mantissa = (uint64_t)(longRand() % 0x10000000000000);

    uint64_t doubleBits = ((uint64_t)sign << 63) | ((uint64_t)exponent << 52) |
                          (uint64_t)mantissa;
    double originalValue = bitsToDouble(doubleBits);
    if (!isnormal(originalValue)) {  // check for nan and infinities
      count--;
      continue;
    }
    char *stringValue = format("%.325f", originalValue);

    double stdlibValue = strtod(stringValue, &ignored);
    uint64_t stdlibBits = doubleToBits(stdlibValue);
    uint64_t conversionBits = doubleStringToBits(stringValue);
    if (stdlibBits != conversionBits) doubleOK = false;
    free(stringValue);
  }
  test("double parsing succeeded", doubleOK);
}

void testConversions(void) {
  testNormalFloatConversions();
  testNormalDoubleConversions();
}