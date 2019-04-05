// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#ifndef TLC_TEST_TESTS_H_
#define TLC_TEST_TESTS_H_

#include "engine.h"

void constExpParseIntTest(TestStatus *);
void constExpParseFloatTest(TestStatus *);
void constExpParseStringTest(TestStatus *);
void constExpParseCharTest(TestStatus *);
void constExpParseWStringTest(TestStatus *);
void constExpParseWCharTest(TestStatus *);

#endif  // TLC_TEST_AST_CONSTEXPPARSETEST_H_