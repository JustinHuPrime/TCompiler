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
 * Formatted string building
 */

#ifndef TLC_UTIL_FORMAT_H_
#define TLC_UTIL_FORMAT_H_

#include <stdio.h>

/**
 * create a string (caller owns the memory) from a printf format string
 *
 * @param format printf format string
 */
char *format(char const *format, ...) __attribute__((format(printf, 1, 2)));

#endif  // TLC_UTIL_FORMAT_H_