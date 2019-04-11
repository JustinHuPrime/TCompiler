// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// lexer and parser utility functions

#ifndef TLC_PARSER_IMPL_UTIL_H_
#define TLC_PARSER_IMPL_UTIL_H_

#include <stdbool.h>

// Determines if an idnetifier is a type or not given the currently active
// symbol tables.
bool isType(char const *id);

#endif  // TLC_PARSER_IMPL_UTIL_H_