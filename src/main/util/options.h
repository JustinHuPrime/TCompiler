// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Command line options manager

#ifndef TLC_UTIL_OPTIONS_H_
#define TLC_UTIL_OPTIONS_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  size_t flagSize;
  size_t flagCapacity;
  char const **flags;
  size_t optionSize;
  size_t optionCapacity;
  char const **keys;
  char const **values;
  size_t argSize;
  size_t argCapacity;
  char const **args;
} Options;

// constructor
// Expects a list of flags: -FLAGNAME
//             and options: -OPTIONNAME=OPTIONVALUE
//          and other args.
Options *optionsParseCmdlineArgs(int, char *[]);

// produces whether or not the flag with given name has been set.
bool optionsHasFlag(Options *, char const *);
// produces the value attached to an option, if one exists
// returns NULL if no such option exists
char const *optionsGetValue(Options *, char const *);

// destructor
void optionsDestroy(Options *);

#endif  // TLC_UTIL_OPTIONS_H_