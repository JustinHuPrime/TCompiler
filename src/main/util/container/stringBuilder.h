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
 * Dynamically sized string builders
 */

#ifndef TLC_UTIL_CONTAINER_STRINGBUILDER_H_
#define TLC_UTIL_CONTAINER_STRINGBUILDER_H_

#include <stddef.h>
#include <stdint.h>

/** a string builder for c-strings */
typedef struct {
  size_t size;
  size_t capacity;
  char *string;
} StringBuilder;

/**
 * in-place ctor
 *
 * @param sb StringBuilder to initialize
 */
void stringBuilderInit(StringBuilder *sb);
/**
 * adds a character to the end of the string
 *
 * @param sb builder to add on to
 * @param c character to add
 */
void stringBuilderPush(StringBuilder *sb, char c);
/**
 * produces a new null terminated c-string that copies current data
 *
 * @param sb builder to copy from
 * @returns a c-string copy of the current contents of the builder
 */
char *stringBuilderData(StringBuilder const *sb);
/**
 * in-place dtor
 *
 * @param sb builder to destroy
 */
void stringBuilderUninit(StringBuilder *sb);

#endif  // TLC_UTIL_CONTAINER_STRINGBUILDER_H_