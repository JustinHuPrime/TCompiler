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

// Implementation of name utilities

#include "util/nameUtils.h"

#include "util/container/stringBuilder.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

bool isScoped(char const *name) {
  for (; *name != '\0'; name++) {
    if (*name == ':') {
      return true;
    }
  }
  return false;
}
void splitName(char const *fullName, char **module, char **shortName) {
  size_t moduleLength = 0;
  size_t shortLength = 0;

  for (size_t idx = strlen(fullName) + 1; idx-- > 0;) {
    if (fullName[idx] == ':') {
      moduleLength = idx;
      shortLength = strlen(fullName) - idx;
    }
  }

  *module = calloc(moduleLength, sizeof(char));
  *shortName = calloc(shortLength, sizeof(char));
  for (size_t idx = 0; idx < moduleLength; idx++) {
    (*module)[idx] = fullName[idx];
  }
  for (size_t idx = 0; idx < shortLength; idx++) {
    (*shortName)[idx] = fullName[moduleLength + 1 + idx];
  }
}
StringVector *explodeName(char const *fullName) {
  StringVector *parts = stringVectorCreate();
  StringBuilder buffer;
  stringBuilderInit(&buffer);

  while (*fullName != '\0') {
    if (*fullName == ':') {
      stringVectorInsert(parts, stringBuilderData(&buffer));
      stringBuilderClear(&buffer);
      fullName += 2;
    } else {
      stringBuilderPush(&buffer, *fullName);
      fullName++;
    }
  }

  stringVectorInsert(parts, stringBuilderData(&buffer));
  stringBuilderUninit(&buffer);
  return parts;
}