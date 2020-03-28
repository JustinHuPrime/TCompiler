// Copyright 2020 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

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
 * @param c char to convert
 */
uint8_t charToU8(char c);

/**
 * converts an unsigned byte to a hex nybble character
 *
 * @param n value to convert - must be in the range [0, 15]
 */
char u8ToNybble(uint8_t n);

/**
 * is the character a hex nybble? (i.e. is c in /[0-9a-fA-F]/ ?)
 *
 * @param c character to query
 */
bool isNybble(char c);

#endif  // TLC_UTIL_CONVERSIONS_H_