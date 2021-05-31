// Copyright 2021 Justin Hu
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
 * translation into IR
 */

#ifndef TLC_TRANSLATION_TRANSLATION_H_
#define TLC_TRANSLATION_TRANSLATION_H_

#include <stddef.h>

/**
 * generate a fresh numeric identifier
 * note - these are unique within a translation unit, and should only be called
 * within the dynamic extent of a translate call
 */
size_t fresh(void);
/**
 * translates all of the files in the file list into IR
 *
 * must have valid typechecked ASTs, should always succeed
 */
void translate(void);

#endif  // TLC_TRANSLATION_TRANSLATION_H_