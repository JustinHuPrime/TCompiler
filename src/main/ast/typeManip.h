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
 * typecheck type predicates
 */

#ifndef TLC_AST_TYPEMANIP_H_
#define TLC_AST_TYPEMANIP_H_

#include "ast/symbolTable.h"

/**
 * Produces true if given type is a boolean, ignoring CV qualification
 *
 * @param t type to query
 * @returns if type is a boolean
 */
bool typeIsBoolean(Type const *t);

/**
 * Produces true if given type may be switched on
 *
 * @param t type to query
 * @returns if type may be switched on
 */
bool typeIsSwitchable(Type const *t);

/**
 * Produces true if given type is numeric (i.e. multiplication is sensible)
 *
 * @param t type to query
 * @returns if type is numeric
 */
bool typeIsNumeric(Type const *t);

/**
 * Produces true if given type is integral (i.e. l/r shift is sensible)
 *
 * @param t type to query
 * @returns if type is integral
 */
bool typeIsIntegral(Type const *t);

/**
 * Produces true if given type is an unsigned integral
 *
 * @param t type to query
 * @returns if type is unsigned
 */
bool typeIsUnsignedIntegral(Type const *t);

/**
 * Produces true if given type is an unsigned integral
 *
 * @param t type to query
 * @returns if type is unsigned
 */
bool typeIsSignedIntegral(Type const *t);

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

/**
 * Produce true if t, ignoring cv-qualification, is a value pointer
 *
 * @param t type to query
 * @returns if t is a value pointer
 */
bool typeIsValuePointer(Type const *t);

/**
 * Produces true if you can compare two values of the given type
 *
 * @param lhs lhs type
 * @param rhs rhs type
 * @returns if you can meaninfully compare two values of the given type
 */
bool typeIsComparable(Type const *lhs, Type const *rhs);

/**
 * Produces true if the given type can have a '.' applied to it
 *
 * @param t type
 * @returns if type is struct or union, after cv-qualification
 */
bool typeIsCompound(Type const *t);

/**
 * Produce the result of a merging of these two types
 *
 * merging only happens for ternary and arithmetic expressions
 *
 * @param lhs lhs of op
 * @param rhs rhs of op
 * @returns type of op - nullable if no merge is possible
 */
Type *typeMerge(Type const *lhs, Type const *rhs);

/**
 * Produce the result of dereferencing the given type
 *
 * expects given type to be a value pointer
 *
 * @param t type to dereference
 * @returns new type from dereferencing given type
 */
Type *typeGetDereferenced(Type const *t);

/**
 * Copy CV-qualification from 'from' onto 'to'
 *
 * @param from type to get CV qualfiication from
 * @param to type write CV qualification to
 * @returns to, but possibly wrapped with CV qualification
 */
Type *typeCopyCV(Type *to, Type const *from);

/**
 * gets the result of calling sizeof on a type
 *
 * @param t type to get size of
 * @returns size of type
 */
size_t typeSizeof(Type const *t);

#endif  // TLC_AST_TYPEMANIP_H_