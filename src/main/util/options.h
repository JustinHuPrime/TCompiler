// Copyright 2019 Justin Hu
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

// options storage POD object

#ifndef TLC_UTIL_OPTIONS_H_
#define TLC_UTIL_OPTIONS_H_

#include "util/errorReport.h"

#include "util/container/hashMap.h"

#include <stdint.h>

// a hashmap between a string and an intptr_t
typedef HashMap Options;

// ctor
Options *optionsCreate(void);
// in-place ctor
void optionsInit(Options *);
// gets the option with the given key
// key must be valid, or zero will be returned
intptr_t optionsGet(Options const *, char const *);
// sets the option with the given key to the given value
// key should be valid
void optionsSet(Options *, char const *, intptr_t);
// in-place dtor
void optionsUninit(Options *);
// dtor
void optionsDestroy(Options *);

typedef enum {
  O_AT_X86 = 1,
  // O_AT_SEP,
} OptionsArchType;
extern char const *optionArch;

typedef enum {
  O_WT_ERROR = 1,
  O_WT_WARN,
  O_WT_IGNORE,
} OptionsWarningType;
extern char const *optionWConstReturn;
extern char const *optionWDuplicateDeclSpecifier;
extern char const *optionWDuplicateDeclaration;
extern char const *optionWDuplicateFile;
extern char const *optionWDuplicateImport;
extern char const *optionWOverloadAmbiguity;
extern char const *optionWReservedId;
extern char const *optionWVoidReturn;
extern char const *optionWUnreachable;
extern char const *optionWUnrecognizedFile;

typedef enum {
  O_DD_NONE = 1,
  O_DD_LEX,
  O_DD_PARSE_STRUCTURE,
  O_DD_PARSE_PRETTY,
  O_DD_IR,
} DebugDumpMode;
extern char const *optionDebugDump;

// parser
void parseOptions(Options *, Report *report, size_t argc,
                  char const *const *argv);

#endif  // TLC_UTIL_OPTIONS_H_