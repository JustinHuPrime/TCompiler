// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// implementation of options storage

#include "util/options.h"

#include "util/format.h"
#include "util/functional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Options *optionsCreate(void) { return hashMapCreate(); }
intptr_t optionsGet(Options const *options, char const *key) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbad-function-cast"
  return (intptr_t)(hashMapGet(options, key));
#pragma GCC diagnostic pop
}
void optionsSet(Options *options, char const *key, intptr_t value) {
  hashMapSet(options, key, (void *)value);
}
void optionsDestroy(Options *options) { hashMapDestroy(options, nullDtor); }

char const *optionArch = "arch";
char const *optionWDuplicateFile = "duplicate-file";
char const *optionWDuplicateImport = "duplicate-import";
char const *optionWUnrecognizedFile = "unrecognized-file";
char const *optionDebugDump = "debug-dump";
Options *parseOptions(Report *report, size_t argc, char const *const *argv) {
  Options *options = optionsCreate();

  // default settings
  optionsSet(options, optionArch, O_AT_X86);
  optionsSet(options, optionWDuplicateFile, O_WT_ERROR);
  optionsSet(options, optionWDuplicateImport, O_WT_IGNORE);
  optionsSet(options, optionWUnrecognizedFile, O_WT_ERROR);
  optionsSet(options, optionDebugDump, O_DD_NONE);

  // parse
  for (size_t idx = 1; idx < argc; idx++) {
    if (argv[idx][0] != '-') {  // not an option
      continue;
    } else if (strcmp(argv[idx], "--arch=x86") == 0) {
      optionsSet(options, optionArch, O_AT_X86);
    } else if (strcmp(argv[idx], "--arch=sep") == 0) {
      optionsSet(options, optionArch, O_AT_SEP);
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
    } else if (strcmp(argv[idx], "--debug-dump=parse") == 0) {
      optionsSet(options, optionDebugDump, O_DD_PARSE);
    } else {
      reportError(report,
                  format("tlc: error: option '%s' not recognized", argv[idx]));
    }
  }

  return options;
}