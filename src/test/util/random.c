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

#include "util/random.h"

#include <stdlib.h>

uint64_t longRand(void) {
  uint64_t high = (uint32_t)rand();
  uint64_t mid = (uint32_t)rand();
  uint64_t low = (uint32_t)rand();
  return high << 62 | mid << 31 | low;
}

uint32_t intRand(void) {
  uint32_t high = (uint32_t)rand();
  uint32_t low = (uint32_t)rand();
  return high << 31 | low;
}