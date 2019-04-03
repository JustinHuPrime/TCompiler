// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Expected one argument - file to process.\n");
    exit(EXIT_FAILURE);
  }

  int parseStatus = parse(argv[1]);
  if (parseStatus < 0) {
    fprintf(stderr, "Parsing error number %d.\n", -parseStatus);
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}