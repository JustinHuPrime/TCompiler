// Copyright 2020 Justin Hu
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

// Implementation of formatted string building

#include "util/format.h"
#include "internalError.h"

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
    error(__FILE__, __LINE__, "could not format error or warning message");
  }
  size_t bufferSize = 1 + (size_t)retVal;
  va_end(args1);
  char *buffer = malloc(bufferSize);
  vsnprintf(buffer, bufferSize, format, args2);
  va_end(args2);

  return buffer;
}