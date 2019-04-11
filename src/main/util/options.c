// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// implementation of options storage

#include "util/options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t const NUM_OPTS = 2;

Options *parseArgs(Report *report, size_t argc, char **argv) {
  Options *options = calloc((1 + (NUM_OPTS - 1) / 64), sizeof(uint64_t));

  for (size_t idx = 1; idx < argc; idx++) {
    if (argv[idx][0] != '-') {  // not an option
      continue;
    } else if (strcmp(argv[idx], "-arch=x86") == 0) {  // option foo
      options[0] |= 1 << 0;
    } else if (strcmp(argv[idx], "-arch=sep") == 0) {
      options[0] |= 1 << 1;
    } else {
      size_t bufferSize = 37 + strlen(argv[idx]);
      char *buffer = malloc(bufferSize);
      snprintf(buffer, bufferSize, "tlc: error: option '%s' not recognized",
               argv[idx]);

      reportError(report, buffer);
    }
  }

  if (!getOpt(options, OPT_ARCH_X86) && !getOpt(options, OPT_ARCH_SEP)) {
    reportError(report,
                strcpy(malloc(37), "tlc: error: no selected architecture"));
  }
  return options;
}

bool getOpt(Options *options, OptionIndex optionIndex) {
  size_t idx = (size_t)optionIndex;
  return ((options[idx / 64] >> idx % 64) & 0x1) == 0x1 ? true : false;
}

void optionsDestroy(Options *options) { free(options); }