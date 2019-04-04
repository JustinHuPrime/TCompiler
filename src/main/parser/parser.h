// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#ifndef TLC_PARSER_PARSER_H_
#define TLC_PARSER_PARSER_H_

#include "ast/ast.h"
#include "parser/parser.tab.h"

#include <stdio.h>

// Parses the file at the given name into an abstract syntax tree.
// @param filename relative path to the file to parse
// @return  0 if successful,
//         -1 if the file couldn't be opened,
//         -2 if the file couldn't be parsed,
//         -3 if the file couldn't be closed.
int parse(char const *filename, Node **);

extern FILE *yyin;

#endif  // TLC_PARSER_PARSER_H_