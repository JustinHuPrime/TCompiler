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
 * type conversions
 */

#ifndef TLC_UTIL_CONVERSIONS_H_
#define TLC_UTIL_CONVERSIONS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * converts a char to an unsigned byte through a type pun
 *
 * @param c char to convert
 * @returns converted value
 */
uint8_t charToU8(char c);

/**
 * converts a set of 32 bits to a float through a type pun
 *
 * @param bits bits to convert
 * @returns converted value
 */
float bitsToFloat(uint32_t bits);
/**
 * converts a set of 32 bits to a double through a type pun
 *
 * @param bits bits to convert
 * @returns converted value
 */
double bitsToDouble(uint64_t bits);
/**
 * converts a float to a set of 32 bits through a type pun
 *
 * @param f float to convert
 * @returns converted value
 */
uint32_t floatToBits(float f);
/**
 * converts a double to a set of 64 bits through a type pun
 *
 * @param d double to convert
 * @returns converted value
 */
uint64_t doubleToBits(double d);

/**
 * converts an unsigned byte to a hex nybble character
 *
 * @param n value to convert - must be in the range [0, 15]
 */
char u8ToNybble(uint8_t n);
/**
 * converts a hex nybble character to an unsigned byte
 *
 * @param c character to convert - must be in /[0-9a-fA-F]/
 */
uint8_t nybbleToU8(char c);
/**
 * is the character a hex nybble? (i.e. is c in /[0-9a-fA-F]/ ?)
 *
 * @param c character to query
 */
bool isNybble(char c);

/**
 * converts a binary integer to a sign and a magnitude
 *
 * @param string string to convert (from lexer - assumes string is good)
 * @param sign output pointer to sign
 * @param magnitudeOut output pointer to magnitude
 * @returns status code - 0 for OK, 1 for size error
 */
int binaryToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut);
/**
 * converts an octal integer to a sign and a magnitude
 *
 * @param string string to convert (from lexer - assumes string is good)
 * @param sign output pointer to sign
 * @param magnitudeOut output pointer to magnitude
 * @returns status code - 0 for OK, 1 for size error
 */
int octalToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut);
/**
 * converts a decimal integer to a sign and a magnitude
 *
 * @param string string to convert (from lexer - assumes string is good)
 * @param sign output pointer to sign
 * @param magnitudeOut output pointer to magnitude
 * @returns status code - 0 for OK, 1 for size error
 */
int decimalToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut);
/**
 * converts a hexadecimal integer to a sign and a magnitude
 *
 * @param string string to convert (from lexer - assumes string is good)
 * @param sign output pointer to sign
 * @param magnitudeOut output pointer to magnitude
 * @returns status code - 0 for OK, 1 for size error
 */
int hexadecimalToInteger(char *string, int8_t *sign, uint64_t *magnitudeOut);

/**
 * converts a float to a set of bits
 *
 * @param string string to convert (from lexer - assumes string is good)
 * @returns converted number as a float
 */
uint32_t floatStringToBits(char const *string);
/**
 * converts a double to a set of bits
 *
 * @param string string to convert (from lexer - assumes string is good)
 * @returns converted number as a double
 */
uint64_t doubleStringToBits(char const *string);

#endif  // TLC_UTIL_CONVERSIONS_H_