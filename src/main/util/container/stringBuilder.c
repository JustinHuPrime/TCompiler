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

// Implementation of the string builder

#include "util/container/stringBuilder.h"

#include <stdlib.h>
#include <string.h>

#include "optimization.h"

void stringBuilderInit(StringBuilder *sb) {
  sb->size = 0;
  sb->capacity = BYTE_VECTOR_INIT_CAPACITY;
  sb->string = malloc(sb->capacity * sizeof(char));
}
void stringBuilderPush(StringBuilder *sb, char c) {
  if (sb->size == sb->capacity) {
    sb->capacity *= VECTOR_GROWTH_FACTOR;
    sb->string = realloc(sb->string, sb->capacity * sizeof(char));
  }
  sb->string[sb->size++] = c;
}
char *stringBuilderData(StringBuilder const *sb) {
  char *string = malloc((sb->size + 1) * sizeof(char));
  memcpy(string, sb->string, sb->size * sizeof(char));
  string[sb->size] = '\0';
  return string;
}
void stringBuilderUninit(StringBuilder *sb) { free(sb->string); }

void tstringBuilderInit(TStringBuilder *sb) {
  sb->size = 0;
  sb->capacity = BYTE_VECTOR_INIT_CAPACITY;
  sb->string = malloc(sb->capacity * sizeof(uint8_t));
}
void tstringBuilderPush(TStringBuilder *sb, uint8_t c) {
  if (sb->size == sb->capacity) {
    sb->capacity *= VECTOR_GROWTH_FACTOR;
    sb->string = realloc(sb->string, sb->capacity * sizeof(char));
  }
  sb->string[sb->size++] = c;
}
uint8_t *tstringBuilderData(TStringBuilder const *sb) {
  uint8_t *string = malloc((sb->size + 1) * sizeof(uint8_t));
  memcpy(string, sb->string, sb->size * sizeof(uint8_t));
  string[sb->size] = '\0';
  return string;
}
void tstringBuilderUninit(TStringBuilder *sb) { free(sb->string); }

void twstringBuilderInit(TWStringBuilder *sb) {
  sb->size = 0;
  sb->capacity = INT_VECTOR_INIT_CAPACITY;
  sb->string = malloc(sb->capacity * sizeof(uint32_t));
}
void twstringBuilderPush(TWStringBuilder *sb, uint32_t c) {
  if (sb->size == sb->capacity) {
    sb->capacity *= VECTOR_GROWTH_FACTOR;
    sb->string = realloc(sb->string, sb->capacity * sizeof(uint32_t));
  }
  sb->string[sb->size++] = c;
}
uint32_t *twstringBuilderData(TWStringBuilder const *sb) {
  uint32_t *string = malloc((sb->size + 1) * sizeof(uint32_t));
  memcpy(string, sb->string, sb->size * sizeof(uint32_t));
  string[sb->size] = '\0';
  return string;
}
void twstringBuilderUninit(TWStringBuilder *sb) { free(sb->string); }