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

// Implementation of the string builder

#include "util/container/stringBuilder.h"

#include <stdlib.h>
#include <string.h>

StringBuilder *stringBuilderCreate(void) {
  StringBuilder *sb = malloc(sizeof(StringBuilder));
  sb->size = 0;
  sb->capacity = 1;
  sb->string = malloc(sizeof(char));
  return sb;
}

void stringBuilderPush(StringBuilder *sb, char c) {
  if (sb->size == sb->capacity) {
    sb->capacity *= 2;
    sb->string = realloc(sb->string, sb->capacity * sizeof(char));
  }
  sb->string[sb->size++] = c;
}

void stringBuilderPop(StringBuilder *sb) { sb->size--; }

char *stringBuilderData(StringBuilder const *sb) {
  char *string = malloc((sb->size + 1) * sizeof(char));
  memcpy(string, sb->string, sb->size);
  string[sb->size] = '\0';
  return string;
}

void stringBuilderClear(StringBuilder *sb) { sb->size = 0; }

void stringBuilderDestroy(StringBuilder *sb) {
  free(sb->string);
  free(sb);
}