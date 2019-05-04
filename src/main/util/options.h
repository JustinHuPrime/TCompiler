// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// options storage POD object

#ifndef TLC_UTIL_OPTIONS_H_
#define TLC_UTIL_OPTIONS_H_

#include "util/errorReport.h"

#include <stdbool.h>
#include <stdint.h>

extern size_t const NUM_OPTS;
typedef enum {
  OPT_ARCH_X86,
  OPT_ARCH_SEP,
  OPT_WARN_DUPLICATE_FILE,
  OPT_WARN_DUPLICATE_FILE_ERROR,
  OPT_WARN_DUPLICATE_IMPORT,
  OPT_WARN_DUPLICATE_IMPORT_ERROR,
  OPT_WARN_UNRECOGNIZED_FILE,
  OPT_WARN_UNRECOGNIZED_FILE_ERROR,
} OptionIndex;

typedef uint64_t Options;

// ctor
Options *parseOptions(Report *report, size_t argc, char const *const *argv);

// gets the nth option
bool getOpt(Options const *, OptionIndex);

// dtor
void optionsDestroy(Options *);

#endif  // TLC_UTIL_OPTIONS_H_