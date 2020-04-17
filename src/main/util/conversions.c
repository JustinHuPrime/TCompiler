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

#include "util/conversions.h"

#include "internalError.h"
#include "numericSizing.h"
#include "optimization.h"

#include <stdlib.h>
#include <string.h>

uint8_t charToU8(char c) {
  union {
    char c;
    uint8_t u;
  } u;
  u.c = c;
  return u.u;
}

float bitsToFloat(uint32_t bits) {
  union {
    uint32_t u;
    float f;
  } u;
  u.u = bits;
  return u.f;
}
double bitsToDouble(uint64_t bits) {
  union {
    uint64_t u;
    double d;
  } u;
  u.u = bits;
  return u.d;
}
uint32_t floatToBits(float f) {
  union {
    uint32_t u;
    float f;
  } u;
  u.f = f;
  return u.u;
}
uint64_t doubleToBits(double d) {
  union {
    uint64_t u;
    double d;
  } u;
  u.d = d;
  return u.u;
}

char u8ToNybble(uint8_t n) {
  if (n <= 9) {
    return (char)('0' + n);
  } else if (n <= 15) {
    return (char)('a' + n - 10);
  } else {
    error(__FILE__, __LINE__, "non-nybble value given");
  }
}
uint8_t nybbleToU8(char c) {
  if (c >= '0' && c <= '9') {
    return (uint8_t)(charToU8(c) - charToU8('0'));
  } else if (c >= 'a' && c <= 'f') {
    return (uint8_t)(charToU8(c) - charToU8('a') + 10);
  } else if (c >= 'A' && c <= 'F') {
    return (uint8_t)(charToU8(c) - charToU8('A') + 10);
  } else {
    error(__FILE__, __LINE__, "non-nybble character given");
  }
}
bool isNybble(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

int binaryToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut) {
  // check sign
  switch (string[0]) {
    case '0': {
      // 0b...
      *sign = 0;  // unsigned
      string += 2;
      break;
    }
    case '-': {
      // -0b...
      *sign = -1;
      string += 3;
      break;
    }
    case '+': {
      // +0...
      *sign = 1;
      string += 3;
      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid binary literal passed to binaryToInteger");
    }
  }

  // get value
  uint64_t magnitude = 0;
  for (; *string != '\0'; string++) {
    uint64_t oldMagnitude = magnitude;
    magnitude *= 2;
    magnitude += (uint8_t)(charToU8(*string) - charToU8('0'));
    if (magnitude < oldMagnitude) {
      // overflowed!
      return -1;
    }
  }

  *magnitudeOut = magnitude;
  return 0;
}
int octalToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut) {
  // check sign
  switch (string[0]) {
    case '0': {
      // 0...
      *sign = 0;  // unsigned
      string += 1;
      break;
    }
    case '-': {
      // -0...
      *sign = -1;
      string += 2;
      break;
    }
    case '+': {
      // +0...
      *sign = 1;
      string += 2;
      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid octal literal passed to octalToInteger");
    }
  }

  // get value
  uint64_t magnitude = 0;
  for (; *string != '\0'; string++) {
    uint64_t oldMagnitude = magnitude;
    magnitude *= 8;
    magnitude += (uint8_t)(charToU8(*string) - charToU8('0'));
    if (magnitude < oldMagnitude) {
      // overflowed!
      return -1;
    }
  }

  *magnitudeOut = magnitude;
  return 0;
}
int decimalToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut) {
  // check sign
  switch (string[0]) {
    case '-': {
      // -...
      *sign = -1;
      string += 1;
      break;
    }
    case '+': {
      // +...
      *sign = 1;
      string += 1;
      break;
    }
    default: {
      if (string[0] >= '0' && string[0] <= '9')
        *sign = 0;
      else
        error(__FILE__, __LINE__,
              "invalid decimal literal passed to decimalToInteger");
    }
  }

  // get value
  uint64_t magnitude = 0;
  for (; *string != '\0'; string++) {
    uint64_t oldMagnitude = magnitude;
    magnitude *= 10;
    magnitude += (uint8_t)(charToU8(*string) - charToU8('0'));
    if (magnitude < oldMagnitude) {
      // overflowed!
      return -1;
    }
  }

  *magnitudeOut = magnitude;
  return 0;
}
int hexadecimalToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut) {
  // check sign
  switch (string[0]) {
    case '0': {
      // 0...
      *sign = 0;  // unsigned
      string += 2;
      break;
    }
    case '-': {
      // -0...
      *sign = -1;
      string += 3;
      break;
    }
    case '+': {
      // +0...
      *sign = 1;
      string += 3;
      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid hexadecimal literal passed to hexadecimalToInteger");
    }
  }

  // get value
  uint64_t magnitude = 0;
  for (; *string != '\0'; string++) {
    uint64_t oldMagnitude = magnitude;
    magnitude *= 16;
    magnitude += nybbleToU8(*string);
    if (magnitude < oldMagnitude) {
      // overflowed!
      return -1;
    }
  }

  *magnitudeOut = magnitude;
  return 0;
}

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
static void bigIntInit(BigInteger *integer) {
  integer->roundingErrorSign = 0;
  integer->capacity = INT_VECTOR_INIT_CAPACITY;
  integer->size = 1;
  integer->digits = malloc(integer->capacity * sizeof(uint32_t));
  integer->digits[0] = 0;
}
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
 * multiplies the BigInteger by n
 *
 * @param integer integer to multiply
 * @param n value to multiply by
 */
static void bigIntMul(BigInteger *integer, uint32_t n) {
  uint32_t carry = 0;
  for (size_t idx = 0; idx < integer->size; idx++) {
    uint64_t digitResult = (uint64_t)integer->digits[idx] * n + carry;
    integer->digits[idx] = (uint32_t)(digitResult % 0x100000000);
    carry = (uint32_t)(digitResult / 0x100000000);
    if (idx + 1 == integer->size && carry != 0)
      // add a digit to be carried in to
      bigIntAddDigit(integer);
  }
}
/**
 * adds n (as a uint32_t) to the BigInteger
 *
 * @param integer integer to add to
 * @param n value to add
 */
static void bigIntAdd(BigInteger *integer, uint32_t n) {
  uint32_t carry = n;
  for (size_t idx = 0; idx < integer->size; idx++) {
    uint64_t digitResult = (uint64_t)integer->digits[idx] + carry;
    integer->digits[idx] = (uint32_t)(digitResult % 0x100000000);
    carry = (uint32_t)(digitResult / 0x100000000);
    if (idx + 1 == integer->size && carry != 0)
      // add a digit to be carried in to
      bigIntAddDigit(integer);
  }
}
/**
 * counts number of non-leading zero bits in the bignum
 *
 * @param integer integer to count bits in
 * @returns number of non-leading zero bits
 */
static size_t bigIntCountSigBits(BigInteger *integer) {
  size_t count = 32 * (integer->size - 1);
  for (uint32_t msd = integer->digits[integer->size - 1]; msd != 0; msd >>= 1)
    count++;
  return count;
}
/**
 * gets the nth bit from the least significant bit
 *
 * @param integer integer to get the bit from
 * @param idx index to get
 */
static uint8_t bigIntBitAtIndex(BigInteger *integer, size_t idx) {
  uint32_t digit = integer->digits[idx / 32];
  return (digit & (1U << idx % 32)) != 0 ? 1 : 0;
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
    integer->digits[idx] = (uint32_t)(digitResult % UINT32_MAX);
    carry = (uint32_t)(digitResult / UINT32_MAX);
    if (idx + 1 == integer->size && carry != 0)
      // add a digit to be carried in to
      bigIntAddDigit(integer);
  }
}
/**
 * change the n most significant bits to be as though the number was rounded
 * to n bits
 *
 * doesn't actually change bits if they would be truncated.
 *
 * @param integer integer to round
 * @param n number of bits to round to
 */
static void bigIntRoundToN(BigInteger *integer, size_t n) {
  // get a 'bit pointer' to the first bit in the first digit
  size_t cutoffIndex = bigIntCountSigBits(integer) - 1;
  // move that pointer to the bits to cut off
  if (cutoffIndex < n + 1) return;  // already rounded
  cutoffIndex -= n + 1;

  // cutoffIndex now points at the bits to round off
  uint8_t firstBit = bigIntBitAtIndex(integer, cutoffIndex);
  if (firstBit == 0) {
    // if following digits are all zeroes, then no rounding, otherwise round
    // down
    for (size_t offset = 1; offset < cutoffIndex; offset++) {
      if (bigIntBitAtIndex(integer, cutoffIndex - offset) == 1) {
        // round down
        integer->roundingErrorSign = -1;
      }
    }
  } else {
    // may be a round to even
    bool rounded = false;
    for (size_t offset = 1; offset < cutoffIndex; offset++) {
      if (bigIntBitAtIndex(integer, cutoffIndex - offset) != 0) {
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
        if (bigIntBitAtIndex(integer, cutoffIndex + 1) == 0) {
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
}
/**
 * get the n most significant, non-leading zero bits
 *
 * assumes that n bits are available
 *
 * @param integer integer to get bits from
 * @param n number of bits to get - less than or equal to 64
 * @returns bits, assembled as unsigned integer and right-aligned
 */
static uint64_t bigIntGetNBits(BigInteger *integer, size_t n) {
  uint64_t retval = 0x0;
  size_t start = bigIntCountSigBits(integer) - 1;
  for (size_t offset = 0; offset < n; offset++) {
    retval <<= 1;
    retval |= bigIntBitAtIndex(integer, start - offset);
  }
  return retval;
}
/**
 * returns true if the bigInt is currently representing zero
 *
 * @param integer integer to consider
 * @returns if the integer is zero
 */
static bool bigIntIsZero(BigInteger *integer) {
  return integer->size == 1 && integer->digits[0] == 0;
}
/**
 * uninitializes a big integer
 *
 * @param integer integer to uninit
 */
static void bigIntUninit(BigInteger *integer) { free(integer->digits); }

/**
 * a chain of decimal digits, as unpacked BCD
 */
typedef struct {
  uint8_t *digits; /**< digits, from least to most significant */
  size_t size;     /**< number of digits */
} DigitChain;
/**
 * constructs a digit chain from a sequence of digit ascii characters
 *
 * @param chain chain to initialize
 * @param digits digit characters to read in
 */
static void digitChainInit(DigitChain *chain, char const *digits) {
  chain->size = strlen(digits);
  chain->digits = malloc(chain->size * sizeof(uint8_t));
  for (size_t offset = 1; offset <= chain->size; offset++)
    chain->digits[offset - 1] =
        (uint8_t)(charToU8(digits[chain->size - offset]) - charToU8('0'));
}
/**
 * multiplies the chain by two, and returns the overflow
 *
 * @param chain chain to operate on
 * @returns any carry that could not be applied to a digit
 */
static uint8_t digitChainMul2(DigitChain *chain) {
  uint8_t carry = 0;
  for (size_t idx = 0; idx < chain->size; idx++) {
    uint8_t result = (uint8_t)(chain->digits[idx] * 2 + carry);
    carry = result / 10;
    chain->digits[idx] = result % 10;
  }
  return carry;
}
/**
 * returns an adjustment for the rest of the chain, rounding to even
 *
 * @param chain chain to operate on
 * @param evenResult result to return to round to even
 * @returns the delta (+1 or 0) for the number based on the current state of the
 * chain
 */
static uint8_t digitChainRound(DigitChain *chain, uint8_t evenResult) {
  // look at the highest value
  size_t offset = 1;
  if (chain->digits[chain->size - offset] < 5) {
    // round down
    return 0;
  } else if (chain->digits[chain->size - offset] > 5) {
    // round up
    return 1;
  } else {
    // may be round to even
    offset++;
    for (; offset <= chain->size; offset++) {
      if (chain->digits[chain->size - offset] != 0) {
        // 0.5...x... - round up
        return 1;
      }
    }
    return evenResult;
  }
}
/**
 * finds whether the digit chain is zero
 *
 * @param chain chain to query
 * @returns true if the digit chain has all zeroes
 */
static bool digitChainIsZero(DigitChain *chain) {
  for (size_t idx = 0; idx < chain->size; idx++) {
    if (chain->digits[idx] != 0) return false;
  }
  return true;
}
/**
 * frees resourced allocate by the digit chain
 *
 * @param chain chain to uninit
 */
static void digitChainUninit(DigitChain *chain) { free(chain->digits); }

static uint64_t floatOrDoubleStringToBits(
    char const *string, size_t mantissaBits, int64_t maxExponent,
    int64_t minNormalExponent, int64_t minSubnormalExponent, uint64_t minusZero,
    uint64_t plusInfinity, uint64_t minusInfinity, uint64_t mantissaMask) {
  int8_t sign;
  // check sign
  switch (string[0]) {
    case '-': {
      // negative
      sign = -1;
      string++;
      break;
    }
    default: {
      // positive
      if (string[0] >= '0' && string[0] <= '9') {
        sign = 1;
      } else if (string[0] == '+') {
        sign = 1;
        string++;
      } else {
        error(__FILE__, __LINE__,
              "invalid float literal passed to floatStringToBits");
      }
      break;
    }
  }
  // may be a zero - stop here if it is
  bool isZero = true;
  for (char const *scan = string; *scan != '\0'; scan++) {
    if (*scan != '0' && *scan != '.') {
      isZero = false;
      break;
    }
  }
  if (isZero) return sign == -1 ? minusZero : 0x0;

  // exponent
  int64_t exponent;
  // mantissa used to construct the number
  uint64_t roundedMantissa;

  // convert whole number part
  BigInteger mantissa;
  bigIntInit(&mantissa);

  for (; *string != '.'; string++) {
    bigIntMul(&mantissa, 10);
    bigIntAdd(&mantissa, (uint8_t)(charToU8(*string) - charToU8('0')));
  }
  string++;  // skip the dot

  if (bigIntCountSigBits(&mantissa) > mantissaBits) {
    // has mantissaBits or more significant (non-leading-zero) bits set
    // round off the decimal part (remember the rounding error sign), then
    // round it off to mantissaBits bits
    uint8_t firstDigit = (uint8_t)(charToU8(*string) - charToU8('0'));
    if (firstDigit == 0) {
      // if following digits are all zeroes, then no rounding, otherwise round
      // down
      string++;
      for (; *string != '\0'; string++) {
        uint8_t digit = (uint8_t)(charToU8(*string) - charToU8('0'));
        if (digit != 0) {
          // round down
          mantissa.roundingErrorSign = -1;
          break;
        }
      }
    } else if (firstDigit < 5) {
      // rounded down
      mantissa.roundingErrorSign = -1;
    } else if (firstDigit > 5) {
      // rounded up
      bigIntAdd(&mantissa, 1);
      mantissa.roundingErrorSign = 1;
    } else {
      // may be a round to even
      string++;
      for (; *string != '\0'; string++) {
        uint8_t digit = (uint8_t)(charToU8(*string) - charToU8('0'));
        if (digit != 0) {
          // 0.5...x... - round up
          bigIntAdd(&mantissa, 1);
          mantissa.roundingErrorSign = 1;
          break;
        }
      }
      if (mantissa.roundingErrorSign == 0) {
        // didn't round - now round to even
        if (bigIntBitAtIndex(&mantissa, 0) == 0) {
          // round down
          mantissa.roundingErrorSign = -1;
        } else {
          // round up
          bigIntAdd(&mantissa, 1);
          mantissa.roundingErrorSign = 1;
        }
      }
    }

    // done rounding the decimal part - round the rest
    bigIntRoundToN(&mantissa, mantissaBits + 1);
    roundedMantissa = bigIntGetNBits(&mantissa, mantissaBits + 1);

    // get the exponent
    exponent = (int64_t)(bigIntCountSigBits(&mantissa) - 1);
  } else {
    // fill in enough bits to make mantissaBits bits
    exponent = (int64_t)(bigIntCountSigBits(&mantissa) - 1);

    // construct a digit chain from the fractional part
    DigitChain chain;
    digitChainInit(&chain, string);
    while (bigIntCountSigBits(&mantissa) < mantissaBits + 1) {
      bigIntMul(&mantissa, 2);
      uint8_t digit = digitChainMul2(&chain);
      bigIntAdd(&mantissa, digit);
      if (bigIntIsZero(&mantissa)) exponent--;
    }

    // round off the rest of the fractional part
    uint8_t adjustment =
        digitChainRound(&chain, bigIntBitAtIndex(&mantissa, 0));
    bigIntAdd(&mantissa, adjustment);
    roundedMantissa = bigIntGetNBits(&mantissa, mantissaBits + 1);
    // mantissa->roundingErrorSign == 0 at this point - set it for the digit
    // chain's conversion
    if (adjustment == 0) {
      if (digitChainIsZero(&chain)) {
        mantissa.roundingErrorSign = 0;
      } else {
        mantissa.roundingErrorSign = -1;
      }
    } else {
      mantissa.roundingErrorSign = 1;
    }

    digitChainUninit(&chain);
  }

  // check for infinity
  if (exponent > maxExponent) {
    // infinity!
    bigIntUninit(&mantissa);
    return sign == 1 ? plusInfinity : minusInfinity;
  } else if (exponent < minNormalExponent && exponent >= minSubnormalExponent) {
    // subnormal
    uint64_t bits = sign == -1 ? minusZero : 0x0;
    size_t ndigits = mantissaBits - (size_t)(minNormalExponent - exponent);
    bigIntRoundToN(&mantissa, ndigits);
    roundedMantissa = bigIntGetNBits(&mantissa, mantissaBits);  // trouble here?
    bits |= roundedMantissa >> (mantissaBits - ndigits - 1);
    bigIntUninit(&mantissa);
    return bits;
  } else if (exponent < minSubnormalExponent) {
    // rounds down to zero
    bigIntUninit(&mantissa);
    return sign == -1 ? minusZero : 0x0;
  } else {
    // normal
    uint64_t bits = sign == -1 ? minusZero : 0x0;
    bits |= (uint64_t)(exponent + maxExponent) << mantissaBits;
    bits |= roundedMantissa & mantissaMask;
    bigIntUninit(&mantissa);
    return bits;
  }
}

uint32_t floatStringToBits(char const *string) {
  return (uint32_t)floatOrDoubleStringToBits(
      string, FLOAT_MANTISSA_BITS, FLOAT_EXPONENT_MAX, FLOAT_EXPONENT_MIN,
      FLOAT_EXPONENT_MIN_SUBNORMAL, 0x80000000, 0x7f800000, 0xff800000,
      0x7fffff);
}

uint64_t doubleStringToBits(char const *string) {
  return floatOrDoubleStringToBits(
      string, DOUBLE_MANTISSA_BITS, DOUBLE_EXPONENT_MAX, DOUBLE_EXPONENT_MIN,
      DOUBLE_EXPONENT_MIN_SUBNORMAL, 0x8000000000000000, 0x7ff0000000000000,
      0xfff0000000000000, 0xfffffffffffff);
}