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

// implementation of character set operations

#include "util/charSet.h"

#include <ctype.h>

uint8_t hexToTChar(char c) {
  if (isdigit(c)) {
    return (uint8_t)(c - '0');
  } else if (isupper(c)) {
    return (uint8_t)(c - 'A' + 0xA);
  } else {
    return (uint8_t)(c - 'a' + 0xA);
  }
}

char hexToChar(char c) {
  if (isdigit(c)) {
    return (char)(c - '0');
  } else if (isupper(c)) {
    return (char)(c - 'A' + 0xA);
  } else {
    return (char)(c - 'a' + 0xA);
  }
}