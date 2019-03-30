#include "parser/parser.h"

int parse(char const *filename) {
  FILE *inFile = fopen(filename, "r");

  if (inFile == NULL) return -1;

  yyin = inFile;
  if (yyparse(NULL) == 1) return -2;

  if (fclose(inFile) == EOF) return -3;

  return 0;
}