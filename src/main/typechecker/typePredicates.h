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
 * abstract syntax tree definition
 */

#ifndef TLC_TYPECHECKER_TYPEPREDICATES_H_
#define TLC_TYPECHECKER_TYPEPREDICATES_H_

#include "ast/symbolTable.h"

/**
 * Produces true if given type is a boolean, ignoring CV qualification
 *
 * @param t type to query
 * @returns if type is a boolean
 */
bool typeIsBoolean(Type const *t);

/**
 * Produces true if given type is integral (bit pattern can be meaningfully
 * interpreted as an integer - excludes pointers)
 *
 * @param t type to query
 * @returns if type is integral
 */
bool typeIsIntegral(Type const *t);

/**
 * Produces true if given type can be used to initialize to the target type
 *
 * @param from type to query
 * @param to type to test against
 * @returns if from can be used to initialize to
 */
bool typeIsInitializable(Type const *to, Type const *from);

/**
 * Produces true if given type can be used to mutate a variable of the target
 * type
 *
 * @param from type to query
 * @param to type to test against
 * @returns if from can be used to assign to to
 */
bool typeIsAssignable(Type const *to, Type const *from);

#endif  // TLC_TYPECHECKER_TYPEPREDICATES_H_