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

#include "util/digitChain.h"

#include <stdlib.h>
#include <string.h>

#include "util/conversions.h"

void digitChainInit(DigitChain *chain, char const *digits) {
  chain->size = strlen(digits);
  chain->digits = malloc(chain->size * sizeof(uint8_t));
  for (size_t offset = 1; offset <= chain->size; offset++)
    chain->digits[offset - 1] =
        (uint8_t)(charToU8(digits[chain->size - offset]) - charToU8('0'));
}
uint8_t digitChainMul2(DigitChain *chain) {
  uint8_t carry = 0;
  for (size_t idx = 0; idx < chain->size; idx++) {
    uint8_t result = (uint8_t)(chain->digits[idx] * 2 + carry);
    carry = result / 10;
    chain->digits[idx] = result % 10;
  }
  return carry;
}
uint8_t digitChainRound(DigitChain *chain, uint8_t evenResult) {
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
bool digitChainIsZero(DigitChain *chain) {
  for (size_t idx = 0; idx < chain->size; idx++) {
    if (chain->digits[idx] != 0) return false;
  }
  return true;
}
void digitChainUninit(DigitChain *chain) { free(chain->digits); }