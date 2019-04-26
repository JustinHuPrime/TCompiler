// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A list of all test modules

#ifndef TLC_TEST_TESTS_H_
#define TLC_TEST_TESTS_H_

#include "engine.h"

// in ast/astNodeList.c
// void astNodeListTest(TestStatus *);
// void astNodeListPairTest(TestStatus *);

// in util/errorReport.c
void errorReportTest(TestStatus *);

// in util/fileList.c
void fileListTest(TestStatus *);

// in util/fileStream.c
void fileStreamTest(TestStatus *);

#endif  // TLC_TEST_AST_CONSTEXPPARSETEST_H_