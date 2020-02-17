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

// Implementation of the string builder

#include "util/container/stringBuilder.h"

#include "optimization.h"

#include <stdlib.h>
#include <string.h>

void stringBuilderInit(StringBuilder *sb) {
  sb->size = 0;
  sb->capacity = BYTE_VECTOR_INIT_CAPACITY;
  sb->string = malloc(sb->capacity * sizeof(char));
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
void stringBuilderUninit(StringBuilder *sb) { free(sb->string); }

void tstringBuilderInit(TStringBuilder *sb) {
  sb->size = 0;
  sb->capacity = BYTE_VECTOR_INIT_CAPACITY;
  sb->string = malloc(sb->capacity * sizeof(uint8_t));
}
void tstringBuilderPush(TStringBuilder *sb, uint8_t c) {
  if (sb->size == sb->capacity) {
    sb->capacity *= 2;
    sb->string = realloc(sb->string, sb->capacity * sizeof(uint8_t));
  }
  sb->string[sb->size++] = c;
}
void tstringBuilderPop(TStringBuilder *sb) { sb->size--; }
uint8_t *tstringBuilderData(TStringBuilder const *sb) {
  uint8_t *string = malloc((sb->size + 1) * sizeof(uint8_t));
  memcpy(string, sb->string, sb->size * sizeof(uint8_t));
  string[sb->size] = '\0';
  return string;
}
void tstringBuilderClear(TStringBuilder *sb) { sb->size = 0; }
void tstringBuilderUninit(TStringBuilder *sb) { free(sb->string); }

void twstringBuilderInit(TWStringBuilder *sb) {
  sb->size = 0;
  sb->capacity = PTR_VECTOR_INIT_CAPACITY;
  sb->string = malloc(sb->capacity * sizeof(uint32_t));
}
void twstringBuilderPush(TWStringBuilder *sb, uint32_t c) {
  if (sb->size == sb->capacity) {
    sb->capacity *= 2;
    sb->string = realloc(sb->string, sb->capacity * sizeof(uint32_t));
  }
  sb->string[sb->size++] = c;
}
void twstringBuilderPop(TWStringBuilder *sb) { sb->size--; }
uint32_t *twstringBuilderData(TWStringBuilder const *sb) {
  uint32_t *string = malloc((sb->size + 1) * sizeof(uint32_t));
  memcpy(string, sb->string, sb->size * sizeof(uint32_t));
  string[sb->size] = '\0';
  return string;
}
void twstringBuilderClear(TWStringBuilder *sb) { sb->size = 0; }
void twstringBuilderUninit(TWStringBuilder *sb) { free(sb->string); }