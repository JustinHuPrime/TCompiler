// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// The primary driver for the TLC.
// Currently just a boilerplate file.

#include "ast/ast.h"
#include "ast/printer.h"
#include "parser/parser.h"
#include "util/options.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  Options *options = optionsParseCmdlineArgs(argc, argv);

  if (options->argSize == 0) {
    fprintf(stderr, "Expected at least one argument - file to process.\n");
    return EXIT_FAILURE;
  }

  Node *ast;

  int parseStatus = parse(argv[1], &ast);
  if (parseStatus < 0) {
    fprintf(stderr, "Parsing error number %d.\n", -parseStatus);
    return EXIT_FAILURE;
  }

  printf("Structure:\n");
  printNodeStructure(ast);
  printf("\n");

  printf("Pretty print:\n");
  printNode(ast);

  nodeDestroy(ast);

  return EXIT_SUCCESS;
}