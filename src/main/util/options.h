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
 * options object and command line argument parser
 */

#ifndef TLC_OPTIONS_H_
#define TLC_OPTIONS_H_

#include <stddef.h>

/** Possible architectures to target */
typedef enum {
  OPTION_A_X86_64,
} ArchOption;
/** Position dependence options */
typedef enum {
  OPTION_PD_PDC,
  OPTION_PD_PIE,
  OPTION_PD_PIC,
} PositionDependenceOption;
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
} DebugDumpOption;
/** Holds options */
typedef struct {
  ArchOption arch;
  PositionDependenceOption positionDependence;
  WarningOption duplicateFile;
  WarningOption unrecognizedFile;
  DebugDumpOption dump;
} Options;

/**
 * Parses arguments into global options object. Counts number of files as a
 * side effect
 * @param argc number of arguments
 * @param argv argument list, as pointer to c-strings
 * @param numFiles output parameter for number of files
 * @returns status code (0 = OK)
 */
int parseArgs(size_t argc, char const *const *argv, size_t *numFiles);

/** global options object - initialized with defaults */
extern Options options;

#endif  // TLC_OPTIONS_H_