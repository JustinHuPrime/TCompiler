// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for format

#include "util/format.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void formatTest(TestStatus *status) {
  char *str = format("%zd:%zd", (size_t)1, (size_t)2);
  test(status, "[util] [format] format produces printf'ed string",
       strcmp(str, "1:2") == 0);
  free(str);
}