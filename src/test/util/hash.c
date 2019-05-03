// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for hashes

#include "util/hash.h"

#include "tests.h"

void hashTest(TestStatus *status) {
  test(status, "[util] [hash] [djb2] djb2 has produces correct value",
       djb2("a1b2") == 6382611557UL);
  test(status, "[util] [hash] [djb2add] djb2add has produces correct value",
       djb2add("a1b2") == 6384983435UL);
}