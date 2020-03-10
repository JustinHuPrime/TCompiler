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

#ifndef TLC_UTIL_STRING_H_
#define TLC_UTIL_STRING_H_

/**
 * @file
 * additional c-string utilities
 */

/**
 * produces a character escape sequence for a character
 *
 * @param c character to escape
 */
char *escapeChar(char c);

/**
 * produces a string escape sequence for a string
 *
 * @param s string to escape
 */
char *escapeString(char const *s);

#endif  // TLC_UTIL_STRING_H_