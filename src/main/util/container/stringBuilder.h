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

// A buffer for a string

#ifndef TLC_UTIL_CONTAINER_STRINGBUILDER_H_
#define TLC_UTIL_CONTAINER_STRINGBUILDER_H_

#include <stddef.h>

typedef struct {
  size_t size;
  size_t capacity;
  char *string;
} StringBuilder;

// ctor
StringBuilder *stringBuilderCreate(void);
// adds a character to the end of the string
void stringBuilderPush(StringBuilder *, char);
// deletes a character from the end of the string
void stringBuilderPop(StringBuilder *);
// produces a new null terminated c-string that copies current data
char *stringBuilderData(StringBuilder const *);
// clears the current string
void stringBuilderClear(StringBuilder *);
// dtor; also deletes the current string
void stringBuilderDestroy(StringBuilder *);

#endif  // TLC_UTIL_CONTAINER_STRINGBUILDER_H_