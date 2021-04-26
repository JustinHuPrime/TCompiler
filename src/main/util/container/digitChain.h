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
 * decimal digit chain - used for string to double conversions
 */

#ifndef TLC_UTIL_CONTAINER_DIGITCHAIN_H_
#define TLC_UTIL_CONTAINER_DIGITCHAIN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
void digitChainInit(DigitChain *chain, char const *digits);
/**
 * multiplies the chain by two, and returns the overflow
 *
 * @param chain chain to operate on
 * @returns any carry that could not be applied to a digit
 */
uint8_t digitChainMul2(DigitChain *chain);
/**
 * returns an adjustment for the rest of the chain, rounding to even
 *
 * @param chain chain to operate on
 * @param evenResult result to return to round to even
 * @returns the delta (+1 or 0) for the number based on the current state of the
 * chain
 */
uint8_t digitChainRound(DigitChain *chain, uint8_t evenResult);
/**
 * finds whether the digit chain is zero
 *
 * @param chain chain to query
 * @returns true if the digit chain has all zeroes
 */
bool digitChainIsZero(DigitChain *chain);
/**
 * frees resourced allocate by the digit chain
 *
 * @param chain chain to uninit
 */
void digitChainUninit(DigitChain *chain);

#endif  // TLC_UTIL_CONTAINER_DIGITCHAIN_H_