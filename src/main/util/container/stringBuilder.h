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

/**
 * @file
 * Dynamically sized string builders
 */

#ifndef TLC_UTIL_CONTAINER_STRINGBUILDER_H_
#define TLC_UTIL_CONTAINER_STRINGBUILDER_H_

#include <stddef.h>
#include <stdint.h>

/** a string builder for c-strings */
typedef struct {
  size_t size;
  size_t capacity;
  char *string;
} StringBuilder;

/**
 * in-place ctor
 *
 * @param sb StringBuilder to initialize
 */
void stringBuilderInit(StringBuilder *sb);
/**
 * adds a character to the end of the string
 *
 * @param sb builder to add on to
 * @param c character to add
 */
void stringBuilderPush(StringBuilder *sb, char c);
/**
 * produces a new null terminated c-string that copies current data
 *
 * @param sb builder to copy from
 * @returns a c-string copy of the current contents of the builder
 */
char *stringBuilderData(StringBuilder const *sb);
/**
 * in-place dtor
 *
 * @param sb builder to destroy
 */
void stringBuilderUninit(StringBuilder *sb);

/** a string builder for t-strings */
typedef struct {
  size_t size;
  size_t capacity;
  uint8_t *string;
} TStringBuilder;

/**
 * in-place ctor
 *
 * @param sb TStringBuilder to initialize
 */
void tstringBuilderInit(TStringBuilder *sb);
/**
 * adds a character to the end of the string
 *
 * @param sb builder to add on to
 * @param c character to add
 */
void tstringBuilderPush(TStringBuilder *sb, uint8_t c);
/**
 * produces a new null terminated c-string that copies current data
 *
 * @param sb builder to copy from
 * @returns a c-string copy of the current contents of the builder
 */
uint8_t *tstringBuilderData(TStringBuilder const *sb);
/**
 * in-place dtor
 *
 * @param sb builder to destroy
 */
void tstringBuilderUninit(TStringBuilder *sb);

/** a string builder for wide t-strings */
typedef struct {
  size_t size;
  size_t capacity;
  uint32_t *string;
} TWStringBuilder;

/**
 * in-place ctor
 *
 * @param sb TWStringBuilder to initialize
 */
void twstringBuilderInit(TWStringBuilder *sb);
/**
 * adds a character to the end of the string
 *
 * @param sb builder to add on to
 * @param c character to add
 */
void twstringBuilderPush(TWStringBuilder *sb, uint32_t c);
/**
 * produces a new null terminated c-string that copies current data
 *
 * @param sb builder to copy from
 * @returns a tw-string copy of the current contents of the builder
 */
uint32_t *twstringBuilderData(TWStringBuilder const *sb);
/**
 * in-place dtor
 *
 * @param sb builder to destroy
 */
void twstringBuilderUninit(TWStringBuilder *sb);

#endif  // TLC_UTIL_CONTAINER_STRINGBUILDER_H_