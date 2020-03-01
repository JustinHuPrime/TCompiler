// Copyright 2020 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

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