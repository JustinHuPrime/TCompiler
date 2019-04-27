// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A buffer for a string

#ifndef TLC_UTIL_STRINGBUILDER_H_
#define TLC_UTIL_STRINGBUILDER_H_

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
char *stringBuilderData(StringBuilder *);
// clears the current string
void stringBuilderClear(StringBuilder *);
// dtor; also deletes the current string
void stringBuilderDestroy(StringBuilder *);

#endif  // TLC_UTIL_STRINGBUILDER_H_