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
 * Container optimization constants
 */

#ifndef TLC_UTIL_CONTAINER_OPTIMIZATION_H_
#define TLC_UTIL_CONTAINER_OPTIMIZATION_H_

#include <stddef.h>

/** starting capacity of a vector of pointers */
extern size_t const PTR_VECTOR_INIT_CAPACITY;
/** starting capacity of a vector of bytes */
extern size_t const BYTE_VECTOR_INIT_CAPACITY;
/** growth factor of a vector */
extern size_t const VECTOR_GROWTH_FACTOR;

#endif  // TLC_UTIL_CONTAINER_OPTIMIZATION_H_