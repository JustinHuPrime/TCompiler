// Copyright 2020-2021 Justin Hu
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
 * additional c-string utilities
 */

#ifndef TLC_UTIL_STRING_H_
#define TLC_UTIL_STRING_H_

#include <stddef.h>
#include <stdint.h>

/**
 * produces a character escape sequence for a character
 *
 * @param c character to escape
 */
char *escapeChar(char c);

/**
 * produces a string escape sequence for a string
 *
 * @param s string to escape
 */
char *escapeString(char const *s);

/**
 * produces a character escape sequence for a T character
 *
 * @param c character to escape
 */
char *escapeTChar(uint8_t c);

/**
 * produces a character escape sequence for a T string
 *
 * @param s string to escape
 */
char *escapeTString(uint8_t const *s);

/**
 * produces a character escape sequence for a wide T character
 *
 * @param c character to escape
 */
char *escapeTWChar(uint32_t c);

/**
 * produces a character escape sequence for a wide T string
 *
 * @param s string to escape
 */
char *escapeTWString(uint32_t const *s);

/**
 * length of a T string
 *
 * @param s string to query
 */
size_t tstrlen(uint8_t const *s);
/**
 * length of a wide T string
 *
 * @param s string to query
 */
size_t twstrlen(uint32_t const *s);

/**
 * duplicate a T string
 *
 * @param s string to duplciate
 */
uint8_t *tstrdup(uint8_t const *s);
/**
 * duplicate a wide T string
 *
 * @param s string to duplciate
 */
uint32_t *twstrdup(uint32_t const *s);

/**
 * compare two T strings
 */
int tstrcmp(uint8_t const *a, uint8_t const *b);
/**
 * compare two wide T strings
 */
long twstrcmp(uint32_t const *a, uint32_t const *b);

#endif  // TLC_UTIL_STRING_H_