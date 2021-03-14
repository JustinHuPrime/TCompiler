// Copyright 2020-2021 Justin Hu
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
 * options object and command line argument parser
 */

#ifndef TLC_OPTIONS_H_
#define TLC_OPTIONS_H_

#include <stddef.h>

/** Warning levels */
typedef enum {
  OPTION_W_IGNORE,
  OPTION_W_WARN,
  OPTION_W_ERROR,
} WarningOption;
/** Debug display points */
typedef enum {
  OPTION_DD_NONE,
  OPTION_DD_LEX,
  OPTION_DD_PARSE,
} DebugDumpOption;
/** Holds options */
typedef struct {
  WarningOption duplicateFile;
  WarningOption duplicateImport;
  WarningOption unrecognizedFile;
  DebugDumpOption dump;
} Options;

/**
 * Parses arguments into global options object. Counts number of files as a
 * side effect
 *
 * @param argc number of arguments
 * @param argv argument list, as pointer to c-strings
 * @param numFiles output parameter for number of files
 * @returns status code (0 = OK)
 */
int parseArgs(size_t argc, char const *const *argv, size_t *numFiles);

/** global options object - initialized with defaults */
extern Options options;

#endif  // TLC_OPTIONS_H_