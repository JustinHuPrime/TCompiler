// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Nice interface to call the parser from and necessary utilites for the parser.

#ifndef TLC_PARSER_PARSER_H_
#define TLC_PARSER_PARSER_H_

#include "ast/ast.h"
#include "parser/moduleNodeTable.h"
#include "util/errorReport.h"
#include "util/fileList.h"

// these includes must be in this order
// clang-format off
#include "parser/impl/parser.tab.h"
#include "parser/impl/lex.yy.h"
// clang-format on

#include <stdbool.h>
#include <stdio.h>

extern int const PARSE_OK;
extern int const PARSE_EIO;
extern int const PARSE_EPARSE;

// Parses the file at the given name into an abstract syntax tree.
// Returns: PARSE_OK if successful
//          PARSE_EIO if file couldn't be opened
//          PARSE_EPARSE if file has bad syntax
int parse(char const *filename, Node **astOut);

ModuleNodeTablePair *parseFiles(Report *report, FileList *files);

#endif  // TLC_PARSER_PARSER_H_