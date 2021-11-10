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

#include <stdlib.h>
#include <string.h>

#include "internalError.h"
#include "numericSizing.h"
#include "util/container/bigInteger.h"
#include "util/container/digitChain.h"
#include "util/format.h"

uint8_t charToU8(char c) {
  union {
    char c;
    uint8_t u;
  } convert;
  convert.c = c;
  return convert.u;
}
char u8ToChar(uint8_t u) {
  union {
    char c;
    uint8_t u;
  } convert;
  convert.u = u;
  return convert.c;
}

float bitsToFloat(uint32_t bits) {
  union {
    uint32_t u;
    float f;
  } convert;
  convert.u = bits;
  return convert.f;
}
double bitsToDouble(uint64_t bits) {
  union {
    uint64_t u;
    double d;
  } convert;
  convert.u = bits;
  return convert.d;
}
uint32_t floatToBits(float f) {
  union {
    uint32_t u;
    float f;
  } convert;
  convert.f = f;
  return convert.u;
}
uint64_t doubleToBits(double d) {
  union {
    uint64_t u;
    double d;
  } convert;
  convert.d = d;
  return convert.u;
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
  for (; *string != '\0'; ++string) {
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
            "invalid octal literal passed to octalToInteger");
    }
  }

  // get value
  uint64_t magnitude = 0;
  for (; *string != '\0'; ++string) {
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
  for (; *string != '\0'; ++string) {
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
  for (; *string != '\0'; ++string) {
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

static uint64_t floatOrDoubleStringToBits(
    char const *string, size_t mantissaBits, int64_t maxExponent,
    int64_t minNormalExponent, int64_t minSubnormalExponent, uint64_t signMask,
    uint64_t plusInfinity, uint64_t minusInfinity, uint64_t mantissaMask) {
  int8_t sign;
  // check sign
  switch (string[0]) {
    case '-': {
      // negative
      sign = -1;
      ++string;
      break;
    }
    default: {
      // positive
      if (string[0] >= '0' && string[0] <= '9') {
        sign = 1;
      } else if (string[0] == '+') {
        sign = 1;
        ++string;
      } else {
        error(__FILE__, __LINE__,
              "invalid float literal passed to floatStringToBits");
      }
      break;
    }
  }
  // may be a zero - stop here if it is
  bool isZero = true;
  for (char const *scan = string; *scan != '\0'; ++scan) {
    if (*scan != '0' && *scan != '.') {
      isZero = false;
      break;
    }
  }
  if (isZero) return sign == -1 ? signMask : 0x0;

  // exponent
  int64_t exponent;
  // mantissa used to construct the number
  uint64_t roundedMantissa;

  // convert whole number part
  BigInteger mantissa;
  bigIntInit(&mantissa);

  for (; *string != '.'; ++string) {
    bigIntMul(&mantissa, 10);
    bigIntAdd(&mantissa, (uint8_t)(charToU8(*string) - charToU8('0')));
  }
  ++string;  // skip the dot

  if (bigIntCountSigBits(&mantissa) > mantissaBits) {
    // has mantissaBits or more significant (non-leading-zero) bits set
    // round off the decimal part (remember the rounding error sign), then
    // round it off to mantissaBits bits
    uint8_t firstDigit = (uint8_t)(charToU8(*string) - charToU8('0'));
    if (firstDigit == 0) {
      // if following digits are all zeroes, then no rounding, otherwise round
      // down
      ++string;
      for (; *string != '\0'; ++string) {
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
      ++string;
      for (; *string != '\0'; ++string) {
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
        if (bigIntGetBitAtIndex(&mantissa, 0) == 0) {
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
      if (bigIntIsZero(&mantissa)) --exponent;
    }

    // round off the rest of the fractional part
    uint8_t adjustment =
        digitChainRound(&chain, bigIntGetBitAtIndex(&mantissa, 0));
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
    uint64_t bits = sign == -1 ? signMask : 0x0;
    size_t ndigits = mantissaBits - (size_t)(minNormalExponent - exponent) + 1;
    size_t sigBitsBefore = bigIntCountSigBits(&mantissa);
    bigIntRoundToN(&mantissa, ndigits);
    size_t sigBitsAfter = bigIntCountSigBits(&mantissa);
    size_t sigBitDelta = sigBitsAfter - sigBitsBefore;
    roundedMantissa = bigIntGetNBits(&mantissa, mantissaBits);

    if (ndigits != 0) {
      bits |= roundedMantissa >>
              ((size_t)(minNormalExponent - exponent) - 1 - sigBitDelta);
    } else {
      bits |= roundedMantissa >> (mantissaBits - 1);
    }
    bigIntUninit(&mantissa);
    return bits;
  } else if (exponent < minSubnormalExponent) {
    // rounds down to zero
    bigIntUninit(&mantissa);
    return sign == -1 ? signMask : 0x0;
  } else {
    // normal
    uint64_t bits = sign == -1 ? signMask : 0x0;
    bits |= (uint64_t)(exponent + maxExponent) << mantissaBits;
    bits |= roundedMantissa & mantissaMask;
    bigIntUninit(&mantissa);
    return bits;
  }
}

uint32_t floatStringToBits(char const *string) {
  return (uint32_t)floatOrDoubleStringToBits(
      string, FLOAT_MANTISSA_BITS, FLOAT_EXPONENT_MAX, FLOAT_EXPONENT_MIN,
      FLOAT_EXPONENT_MIN_SUBNORMAL, FLOAT_SIGN_MASK, FLOAT_EXPONENT_MASK,
      FLOAT_SIGN_MASK | FLOAT_EXPONENT_MASK, FLOAT_MANTISSA_MASK);
}

uint64_t doubleStringToBits(char const *string) {
  return floatOrDoubleStringToBits(
      string, DOUBLE_MANTISSA_BITS, DOUBLE_EXPONENT_MAX, DOUBLE_EXPONENT_MIN,
      DOUBLE_EXPONENT_MIN_SUBNORMAL, DOUBLE_SIGN_MASK, DOUBLE_EXPONENT_MASK,
      DOUBLE_SIGN_MASK | DOUBLE_EXPONENT_MASK, DOUBLE_MANTISSA_MASK);
}

uint8_t s8ToU8(int8_t s) {
  union {
    uint8_t u;
    int8_t s;
  } u;
  u.s = s;
  return u.u;
}
uint16_t s16ToU16(int16_t s) {
  union {
    uint16_t u;
    int16_t s;
  } u;
  u.s = s;
  return u.u;
}
uint32_t s32ToU32(int32_t s) {
  union {
    uint32_t u;
    int32_t s;
  } u;
  u.s = s;
  return u.u;
}
uint64_t s64ToU64(int64_t s) {
  union {
    uint64_t u;
    int64_t s;
  } u;
  u.s = s;
  return u.u;
}

uint32_t uintToFloatBits(uint64_t i) {
  char *s = format("%lu.0", i);
  uint32_t bits = floatStringToBits(s);
  free(s);
  return bits;
}
uint32_t intToFloatBits(int64_t i) {
  char *s = format("%ld.0", i);
  uint32_t bits = floatStringToBits(s);
  free(s);
  return bits;
}
uint64_t uintToDoubleBits(uint64_t i) {
  char *s = format("%lu.0", i);
  uint64_t bits = doubleStringToBits(s);
  free(s);
  return bits;
}
uint64_t intToDoubleBits(int64_t i) {
  char *s = format("%ld.0", i);
  uint64_t bits = doubleStringToBits(s);
  free(s);
  return bits;
}

uint64_t floatBitsToDoubleBits(uint32_t f) {
  bool positive = (f & FLOAT_SIGN_MASK) == 0;
  int64_t exponent =
      ((f & FLOAT_EXPONENT_MASK) >> FLOAT_MANTISSA_BITS) - FLOAT_EXPONENT_MAX;
  uint64_t mantissa = f & FLOAT_MANTISSA_MASK;

  if (exponent < FLOAT_EXPONENT_MIN) {
    // renormalize
    while (mantissa == (mantissa & DOUBLE_MANTISSA_MASK)) {
      exponent--;
      mantissa <<= 1;
    }
  }

  uint64_t bits = positive ? 0 : DOUBLE_SIGN_MASK;
  bits |= (uint64_t)(exponent + DOUBLE_EXPONENT_MAX) << DOUBLE_MANTISSA_BITS;
  bits |= mantissa << (DOUBLE_MANTISSA_BITS - FLOAT_MANTISSA_BITS);
  return bits;
}
