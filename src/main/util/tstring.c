// Copyright 2019 Justin Hu
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

// Implementation of T string utilities

#include "util/tstring.h"

#include <stdlib.h>
#include <string.h>

uint8_t *tstrdup(uint8_t const *string) {
  size_t nbytes = (tstrlen(string) + 1) * sizeof(uint8_t);
  uint8_t *out = malloc(nbytes);
  memcpy(out, string, nbytes);
  return out;
}
uint32_t *twstrdup(uint32_t const *string) {
  size_t nbytes = (twstrlen(string) + 1) * sizeof(uint32_t);
  uint32_t *out = malloc(nbytes);
  memcpy(out, string, nbytes);
  return out;
}
size_t tstrlen(uint8_t const *string) {
  size_t count = 0;
  while (string[count] != 0) {
    count++;
  }
  return count;
}
size_t twstrlen(uint32_t const *string) {
  size_t count = 0;
  while (string[count] != 0) {
    count++;
  }
  return count;
}