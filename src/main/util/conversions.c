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

#include "util/conversions.h"

#include "internalError.h"

uint8_t charToU8(char c) {
  union {
    char c;
    uint8_t u;
  } u;
  u.c = c;
  return u.u;
}

char u8ToNybble(uint8_t n) {
  if (n <= 9) {
    return (char)('0' + n);
  } else if (n <= 15) {
    return (char)('a' + n - 10);
  } else {
    error(__FILE__, __LINE__, "non-nybble value given");
  }
}

bool isNybble(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}