// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// implementation of options storage

#include "util/options.h"

#include "util/format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t const NUM_OPTS = 8;

static void setOpt(Options *options, OptionIndex optionIndex, bool value) {
  size_t idx = (size_t)optionIndex;
  if (value) {
    options[idx / 64] |= 0x1ul << idx % 64;
  } else {
    options[idx / 64] &= ~(0x1ul << idx % 64);
  }
}
Options *parseOptions(Report *report, size_t argc, char const *const *argv) {
  Options *options = calloc((1 + (NUM_OPTS - 1) / 64), sizeof(uint64_t));

  // set initial state (default is false)
  setOpt(options, OPT_WARN_DUPLICATE_FILE, true);
  setOpt(options, OPT_WARN_DUPLICATE_FILE_ERROR, true);
  setOpt(options, OPT_WARN_UNRECOGNIZED_FILE, true);
  setOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR, true);

  // parse
  for (size_t idx = 1; idx < argc; idx++) {
    if (argv[idx][0] != '-') {  // not an option
      continue;
    } else if (strcmp(argv[idx], "-Ax86") == 0) {
      setOpt(options, OPT_ARCH_X86, true);
    } else if (strcmp(argv[idx], "-Asep") == 0) {
      setOpt(options, OPT_ARCH_SEP, true);
    } else if (strcmp(argv[idx], "-Werror-duplicate-file")) {
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE, true);
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR, true);
    } else if (strcmp(argv[idx], "-Wduplicate-file")) {
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE, true);
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR, false);
    } else if (strcmp(argv[idx], "-Wno-duplicate-file")) {
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE, false);
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR, false);
    } else if (strcmp(argv[idx], "-Werror-duplicate-import")) {
      setOpt(options, OPT_WARN_DUPLICATE_IMPORT, true);
      setOpt(options, OPT_WARN_DUPLICATE_IMPORT_ERROR, true);
    } else if (strcmp(argv[idx], "-Wduplicate-import")) {
      setOpt(options, OPT_WARN_DUPLICATE_IMPORT, true);
      setOpt(options, OPT_WARN_DUPLICATE_IMPORT_ERROR, false);
    } else if (strcmp(argv[idx], "-Wno-duplicate-import")) {
      setOpt(options, OPT_WARN_DUPLICATE_IMPORT, false);
      setOpt(options, OPT_WARN_DUPLICATE_IMPORT_ERROR, false);
    } else if (strcmp(argv[idx], "-Werror-unrecognized-file")) {
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE, true);
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR, true);
    } else if (strcmp(argv[idx], "-Wunrecognized-file")) {
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE, true);
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR, false);
    } else if (strcmp(argv[idx], "-Wno-unrecognized-file")) {
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE, false);
      setOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR, false);
    } else {
      reportError(report,
                  format("tlc: error: option '%s' not recognized", argv[idx]));
    }
  }

  // validate
  if (!getOpt(options, OPT_ARCH_X86) && !getOpt(options, OPT_ARCH_SEP)) {
    reportError(report,
                strcpy(malloc(37), "tlc: error: no selected architecture"));
  } else if (getOpt(options, OPT_ARCH_X86) && getOpt(options, OPT_ARCH_SEP)) {
    reportError(report, strcpy(malloc(44),
                               "tlc: error: multiple selected architectures"));
  }
  return options;
}

bool getOpt(Options const *options, OptionIndex optionIndex) {
  size_t idx = (size_t)optionIndex;
  return ((options[idx / 64] >> idx % 64) & 0x1ul) == 0x1ul ? true : false;
}

void optionsDestroy(Options *options) { free(options); }