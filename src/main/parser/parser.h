#ifndef TCC_PARSER_PARSER_H_
#define TCC_PARSER_PARSER_H_

#include "parser/parser.tab.h"

#include <stdio.h>

// Parses the file at the given name into an abstract syntax tree.
// @param filename relative path to the file to parse
// @return  0 if successful,
//         -1 if the file couldn't be opened,
//         -2 if the file couldn't be parsed,
//         -3 if the file couldn't be closed.
int parse(char const *filename);

extern FILE *yyin;

#endif  // TCC_PARSER_PARSER_H_