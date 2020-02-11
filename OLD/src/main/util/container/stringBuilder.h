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
#include <stdint.h>

typedef struct {
  size_t size;
  size_t capacity;
  char *string;
} StringBuilder;

// ctor
StringBuilder *stringBuilderCreate(void);
// in-place ctor
void stringBuilderInit(StringBuilder *);
// adds a character to the end of the string
void stringBuilderPush(StringBuilder *, char);
// deletes a character from the end of the string
void stringBuilderPop(StringBuilder *);
// produces a new null terminated c-string that copies current data
char *stringBuilderData(StringBuilder const *);
// clears the current string
void stringBuilderClear(StringBuilder *);
// in-place dtor
void stringBuilderUninit(StringBuilder *);
// dtor; also deletes the current string
void stringBuilderDestroy(StringBuilder *);

typedef struct {
  size_t size;
  size_t capacity;
  uint8_t *string;
} TStringBuilder;

// ctor
TStringBuilder *tstringBuilderCreate(void);
// in-place ctor
void tstringBuilderInit(TStringBuilder *);
// adds a character to the end of the string
void tstringBuilderPush(TStringBuilder *, uint8_t);
// deletes a character from the end of the string
void tstringBuilderPop(TStringBuilder *);
// produces a new null terminated unsigned char string that copies current data
uint8_t *tstringBuilderData(TStringBuilder const *);
// clears the current string
void tstringBuilderClear(TStringBuilder *);
// in-place dtor
void tstringBuilderUninit(TStringBuilder *);
// dtor; also deletes the current string
void tstringBuilderDestroy(TStringBuilder *);

typedef struct {
  size_t size;
  size_t capacity;
  uint32_t *string;
} TWStringBuilder;

// ctor
TWStringBuilder *twstringBuilderCreate(void);
// in-place ctor
void twstringBuilderInit(TWStringBuilder *);
// adds a character to the end of the string
void twstringBuilderPush(TWStringBuilder *, uint32_t);
// deletes a character from the end of the string
void twstringBuilderPop(TWStringBuilder *);
// produces a new null terminated unsigned wchar string that copies current data
uint32_t *twstringBuilderData(TWStringBuilder const *);
// clears the current string
void twstringBuilderClear(TWStringBuilder *);
// in-place dtor
void twstringBuilderUninit(TWStringBuilder *);
// dtor; also deletes the current string
void twstringBuilderDestroy(TWStringBuilder *);

#endif  // TLC_UTIL_CONTAINER_STRINGBUILDER_H_