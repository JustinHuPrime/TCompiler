// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Formatted string building

#ifndef TLC_UTIL_FORMAT_H_
#define TLC_UTIL_FORMAT_H_

#include <stdio.h>

char *format(char const *format, ...) __attribute__((format(printf, 1, 2)));

#endif  // TLC_UTIL_FORMAT_H_