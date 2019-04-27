// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of the string builder

#include "util/stringBuilder.h"

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

char *stringBuilderData(StringBuilder *sb) {
  char *string = malloc((sb->size + 1) * sizeof(char));
  memcpy(string, sb->string, sb->size);
  string[sb->size] = '\0';
  return string;
}

void stringBuilderDestroy(StringBuilder *sb) {
  free(sb->string);
  free(sb);
}