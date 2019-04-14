// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of formatted string building

#include "util/format.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *format(char const *format, ...) {
  va_list args1, args2;
  va_start(args1, format);
  va_copy(args2, args1);
  int retVal = vsnprintf(NULL, 0, format, args1);
  if (retVal < 0) {
    return strcpy(malloc(54),
                  "tlc: error: could not format error or warning message");
  }
  size_t bufferSize = 1 + (size_t)retVal;
  va_end(args1);
  char *buffer = malloc(bufferSize);
  vsnprintf(buffer, bufferSize, format, args2);
  va_end(args2);

  return buffer;
}