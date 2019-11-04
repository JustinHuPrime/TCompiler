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

// implementation of options storage

#include "util/options.h"

#include "util/functional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Options *optionsCreate(void) { return hashMapCreate(); }
void optionsInit(Options *options) { hashMapInit(options); }
intptr_t optionsGet(Options const *options, char const *key) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbad-function-cast"
  return (intptr_t)hashMapGet(options, key);
#pragma GCC diagnostic pop
}
void optionsSet(Options *options, char const *key, intptr_t value) {
  hashMapSet(options, key, (void *)value);
}
void optionsUninit(Options *options) { hashMapUninit(options, nullDtor); }
void optionsDestroy(Options *options) { hashMapDestroy(options, nullDtor); }

char const *optionArch = "arch";
char const *optionPositionIndependence = "position-independence";
char const *optionWConstReturn = "const-return";
char const *optionWDuplicateDeclSpecifier = "duplicate-decl-specifier";
char const *optionWDuplicateDeclaration = "duplicate-declaration";
char const *optionWDuplicateFile = "duplicate-file";
char const *optionWDuplicateImport = "duplicate-import";
char const *optionWOverloadAmbiguity = "overload-ambiguity";
char const *optionWReservedId = "reserved-id";
char const *optionWVoidReturn = "void-return";
char const *optionWUnreachable = "unreachable";
char const *optionWUnrecognizedFile = "unrecognized-file";
char const *optionDebugDump = "debug-dump";
void parseOptions(Options *options, Report *report, size_t argc,
                  char const *const *argv) {
  optionsInit(options);

  // default settings
  optionsSet(options, optionArch, O_AT_X86);
  optionsSet(options, optionPositionIndependence, O_PI_NONE);
  optionsSet(options, optionWConstReturn, O_WT_WARN);
  optionsSet(options, optionWDuplicateDeclSpecifier, O_WT_WARN);
  optionsSet(options, optionWDuplicateDeclaration, O_WT_IGNORE);
  optionsSet(options, optionWDuplicateFile, O_WT_ERROR);
  optionsSet(options, optionWDuplicateImport, O_WT_IGNORE);
  optionsSet(options, optionWOverloadAmbiguity, O_WT_ERROR);
  optionsSet(options, optionWReservedId, O_WT_ERROR);
  optionsSet(options, optionWVoidReturn, O_WT_ERROR);
  optionsSet(options, optionWUnreachable, O_WT_WARN);
  optionsSet(options, optionWUnrecognizedFile, O_WT_ERROR);
  optionsSet(options, optionDebugDump, O_DD_NONE);

  // parse
  for (size_t idx = 1; idx < argc; idx++) {
    if (argv[idx][0] != '-') {  // not an option
      continue;
    } else if (strcmp(argv[idx], "--arch=x86_64") == 0) {
      optionsSet(options, optionArch, O_AT_X86);
      // } else if (strcmp(argv[idx], "--arch=sep") == 0) {
      //   optionsSet(options, optionArch, O_AT_SEP);
    } else if (strcmp(argv[idx], "-fPDC") == 0) {
      optionsSet(options, optionPositionIndependence, O_PI_NONE);
    } else if (strcmp(argv[idx], "-fPIE") == 0) {
      optionsSet(options, optionPositionIndependence, O_PI_PIE);
    } else if (strcmp(argv[idx], "-fPIC") == 0) {
      optionsSet(options, optionPositionIndependence, O_PI_PIC);
    } else if (strcmp(argv[idx], "-Wconst-return=error") == 0) {
      optionsSet(options, optionWConstReturn, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wconst-return=warn") == 0) {
      optionsSet(options, optionWConstReturn, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wconst-return=ignore") == 0) {
      optionsSet(options, optionWConstReturn, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wduplicate-decl-specifier=error") == 0) {
      optionsSet(options, optionWDuplicateDeclSpecifier, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wduplicate-decl-specifier=warn") == 0) {
      optionsSet(options, optionWDuplicateDeclSpecifier, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wduplicate-decl-specifier=ignore") == 0) {
      optionsSet(options, optionWDuplicateDeclSpecifier, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wduplicate-declaration=error") == 0) {
      optionsSet(options, optionWDuplicateDeclaration, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wduplicate-declaration=warn") == 0) {
      optionsSet(options, optionWDuplicateDeclaration, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wduplicate-declaration=ignore") == 0) {
      optionsSet(options, optionWDuplicateDeclaration, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wduplicate-file=error") == 0) {
      optionsSet(options, optionWDuplicateFile, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wduplicate-file=warn") == 0) {
      optionsSet(options, optionWDuplicateFile, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wduplicate-file=ignore") == 0) {
      optionsSet(options, optionWDuplicateFile, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wduplicate-import=error") == 0) {
      optionsSet(options, optionWDuplicateImport, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wduplicate-import=warn") == 0) {
      optionsSet(options, optionWDuplicateImport, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wduplicate-import=ignore") == 0) {
      optionsSet(options, optionWDuplicateImport, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Woverload-ambiguity=error") == 0) {
      optionsSet(options, optionWOverloadAmbiguity, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Woverload-ambiguity=warn") == 0) {
      optionsSet(options, optionWOverloadAmbiguity, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Woverload-ambiguity=ignore") == 0) {
      optionsSet(options, optionWOverloadAmbiguity, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wreserved-id=error") == 0) {
      optionsSet(options, optionWReservedId, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wreserved-id=warn") == 0) {
      optionsSet(options, optionWReservedId, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wreserved-id=ignore") == 0) {
      optionsSet(options, optionWReservedId, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wvoid-return=error") == 0) {
      optionsSet(options, optionWVoidReturn, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wvoid-return=warn") == 0) {
      optionsSet(options, optionWVoidReturn, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wvoid-return=ignore") == 0) {
      optionsSet(options, optionWVoidReturn, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wunreachable=error") == 0) {
      optionsSet(options, optionWUnreachable, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wunreachable=warn") == 0) {
      optionsSet(options, optionWUnreachable, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wunreachable=ignore") == 0) {
      optionsSet(options, optionWUnreachable, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "-Wunrecognized-file=error") == 0) {
      optionsSet(options, optionWUnrecognizedFile, O_WT_ERROR);
    } else if (strcmp(argv[idx], "-Wunrecognized-file=warn") == 0) {
      optionsSet(options, optionWUnrecognizedFile, O_WT_WARN);
    } else if (strcmp(argv[idx], "-Wunrecognized-file=ignore") == 0) {
      optionsSet(options, optionWUnrecognizedFile, O_WT_IGNORE);
    } else if (strcmp(argv[idx], "--debug-dump=none") == 0) {
      optionsSet(options, optionDebugDump, O_DD_NONE);
    } else if (strcmp(argv[idx], "--debug-dump=lex") == 0) {
      optionsSet(options, optionDebugDump, O_DD_LEX);
    } else if (strcmp(argv[idx], "--debug-dump=parse-structure") == 0) {
      optionsSet(options, optionDebugDump, O_DD_PARSE_STRUCTURE);
    } else if (strcmp(argv[idx], "--debug-dump=parse-pretty") == 0) {
      optionsSet(options, optionDebugDump, O_DD_PARSE_PRETTY);
    } else if (strcmp(argv[idx], "--debug-dump=ir") == 0) {
      optionsSet(options, optionDebugDump, O_DD_IR);
    } else if (strcmp(argv[idx], "--debug-dump=asm-1") == 0) {
      optionsSet(options, optionDebugDump, O_DD_ASM_1);
    } else if (strcmp(argv[idx], "--debug-dump=asm-2") == 0) {
      optionsSet(options, optionDebugDump, O_DD_ASM_2);
    } else {
      reportError(report, "tlc: error: option '%s' not recognized", argv[idx]);
    }
  }
}