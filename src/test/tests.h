// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// A list of all test modules

#ifndef TLC_TEST_TESTS_H_
#define TLC_TEST_TESTS_H_

#include "engine.h"

// in ast/constExpParseTest.c
void constExpParseIntTest(TestStatus *);
void constExpParseFloatTest(TestStatus *);
void constExpParseStringTest(TestStatus *);
void constExpParseCharTest(TestStatus *);
void constExpParseWStringTest(TestStatus *);
void constExpParseWCharTest(TestStatus *);

// in ast/nodeListTest.c
void nodeListTest(TestStatus *);
void nodeListPairTest(TestStatus *);

// in util/symbolTableTest/c
void symbolTableTest(TestStatus *);

#endif  // TLC_TEST_AST_CONSTEXPPARSETEST_H_