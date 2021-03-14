// Copyright 2019-2020 Justin Hu
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
 * reports and aborts on interal compiler errors
 */

#ifndef TLC_INTERNALERROR_H_
#define TLC_INTERNALERROR_H_

#include <stddef.h>

/**
 * Report an internal compiler error. Error is specified as coming from the
 * given file and line, with the given message
 *
 * @param file should be the macro \_\_FILE\_\_
 * @param line should be the macro \_\_LINE\_\_
 * @param message message to display
 */
void error(char const *file, size_t line, char const *message)
    __attribute__((noreturn));

#endif  // TLC_INTERNALERROR_H_