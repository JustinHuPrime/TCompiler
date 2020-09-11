// Copyright 2020 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Implementation of formatted string building

#include "util/format.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internalError.h"

char *format(char const *format, ...) {
  va_list args1, args2;
  va_start(args1, format);
  va_copy(args2, args1);

  int retval = vsnprintf(NULL, 0, format, args1);
  if (retval < 0) error(__FILE__, __LINE__, "could not format string");

  size_t bufferSize = 1 + (size_t)retval;
  va_end(args1);

  char *buffer = malloc(bufferSize);
  vsnprintf(buffer, bufferSize, format, args2);
  va_end(args2);

  return buffer;
}