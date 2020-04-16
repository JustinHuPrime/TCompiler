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
// The T Language Compiler. If not, see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * type punning conversions
 */

#ifndef TLC_UTIL_CONVERSIONS_H_
#define TLC_UTIL_CONVERSIONS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * converts a char to an unsigned byte
 * @param c char to convert
 */
uint8_t charToU8(char c);

/**
 * converts an unsigned byte to a hex nybble
 *
 * @param n value to convert - must be in the range [0, 15]
 */
char u8ToNybble(uint8_t n);

/**
 * is the character a hex nybble? (i.e. is c in [0-9a-fA-F])
 *
 * @param c character to query
 */
bool isNybble(char c);

#endif  // TLC_UTIL_CONVERSIONS_H_