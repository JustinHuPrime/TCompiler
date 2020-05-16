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
 * A set of strings
 */

#ifndef TLC_UTIL_CONTAINER_HASHSET_H_
#define TLC_UTIL_CONTAINER_HASHSET_H_

#include <stdbool.h>
#include <stddef.h>

/** A set of strings, not owned by this */
typedef struct {
  size_t size;
  size_t capacity;
  char const **elements;
} HashSet;

/**
 * initialize set in-place
 *
 * @param set set to initialize
 */
void hashSetInit(HashSet *set);

/**
 * Returns whether the hash set contains the value
 *
 * @param set set to search in
 * @param s string to search for
 */
bool hashSetContains(HashSet const *set, char const *s);

/**
 * Tries to insert a key into the table. Note that key is not owned by the
 * table, but the node is. Amortized constant time operation
 *
 * @param set set to insert into
 * @param s string to insert
 * @returns 0 if inserertion is successful, -1 if the key exists
 */
int hashSetPut(HashSet *set, char const *s);

/**
 * deinitialize set in-place
 *
 * @param set set to deinitialize
 */
void hashSetUninit(HashSet *set);

#endif  // TLC_UTIL_CONTAINER_HASHSET_H_