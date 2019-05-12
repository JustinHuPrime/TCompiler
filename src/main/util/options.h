// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// options storage POD object

#ifndef TLC_UTIL_OPTIONS_H_
#define TLC_UTIL_OPTIONS_H_

#include "util/errorReport.h"

#include "util/hashMap.h"

#include <stdint.h>

// a hashmap between a string and an intptr_t
typedef HashMap Options;

// ctor
Options *optionsCreate(void);
// gets the option with the given key
// key must be valid, or zero will be returned
intptr_t optionsGet(Options const *, char const *);
// sets the option with the given key to the given value
// key should be valid
void optionsSet(Options *, char const *, intptr_t);
// dtor
void optionsDestroy(Options *);

typedef enum {
  O_AT_X86 = 1,
  O_AT_SEP,
} OptionsArchType;
extern char const *optionArch;

typedef enum {
  O_WT_ERROR = 1,
  O_WT_WARN,
  O_WT_IGNORE,
} OptionsWarningType;
extern char const *optionWDuplicateFile;
extern char const *optionWDuplicateImport;
extern char const *optionWUnrecognizedFile;

typedef enum {
  O_DD_NONE = 1,
  O_DD_LEX,
  O_DD_PARSE,
} DebugDumpMode;
extern char const *optionDebugDump;

// parser
Options *parseOptions(Report *report, size_t argc, char const *const *argv);

#endif  // TLC_UTIL_OPTIONS_H_