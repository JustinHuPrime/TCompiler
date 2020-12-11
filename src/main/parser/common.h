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
 * common parsing functions
 */

#ifndef TLC_PARSER_COMMON_H_
#define TLC_PARSER_COMMON_H_

#include "ast/ast.h"

/**
 * prints an error complaining about a wrong token, specifying what it should
 * have been, as a string
 *
 * @param entry entry to attribute the error to
 * @param expected string describing the expected tokens
 * @param actual actual token
 */
void errorExpectedString(FileListEntry *entry, char const *expected,
                         Token *actual);
/**
 * prints an error complaining about a wrong token,
 * specifying what token it should have been
 *
 * @param entry entry to attribute the error to
 * @param expected TokenType expected
 * @param actual actual token
 */
void errorExpectedToken(FileListEntry *entry, TokenType expected,
                        Token *actual);

#endif  // TLC_PARSER_COMMON_H_