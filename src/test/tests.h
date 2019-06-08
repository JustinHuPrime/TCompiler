// Copyright 2019 Justin Hu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file is part of the T Language Compiler.

// A list of all test modules

#ifndef TLC_TEST_TESTS_H_
#define TLC_TEST_TESTS_H_

#include "engine.h"

// in lexer/lexer.c
void keywordMapTest(TestStatus *);
void lexerTest(TestStatus *);

// in util/errorReport.c
void errorReportTest(TestStatus *);

// in util/fileList.c
void fileListTest(TestStatus *);

// in util/file.c
void fileTest(TestStatus *);

// in util/hash.c
void hashTest(TestStatus *);

// in util/hashMap.c
void hashMapTest(TestStatus *);

// in util/stringBuilder.c
void stringBuilderTest(TestStatus *);

// in util/vector.c
void vectorTest(TestStatus *);

#endif  // TLC_TEST_AST_CONSTEXPPARSETEST_H_