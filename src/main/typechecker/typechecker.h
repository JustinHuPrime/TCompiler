// Copyright 2019, 2021 Justin Hu
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
 * type checker
 */

#ifndef TLC_TYPECHECKER_TYPECHECKER_
#define TLC_TYPECHECKER_TYPECHECKER_

/**
 * typechecks all of the files in the file list
 *
 * must have valid ASTs
 *
 * @returns status code (0 = OK, -1 = fatal error)
 */
int typecheck(void);

#endif  // TLC_TYPECHECKER_TYPECHECKER_