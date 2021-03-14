// Copyright 2019-2020 Justin Hu
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
 * string hash functions
 */

#ifndef TLC_UTIL_HASH_H_
#define TLC_UTIL_HASH_H_

#include <stdint.h>

/**
 * hash a string using djb2xor xor variant
 *
 * @param s string
 * @returns djb-xor hash of the string
 */
uint64_t djb2xor(char const *s);

/**
 * hash a string using djb2xor add variant
 *
 * @param s string
 * @returns djb-add hash of the string
 */
uint64_t djb2add(char const *s);

#endif  // TLC_UTIL_HASH_H_