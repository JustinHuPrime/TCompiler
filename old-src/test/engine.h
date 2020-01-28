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

// The test engine status object

#ifndef TLC_TEST_ENGINE_H_
#define TLC_TEST_ENGINE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// summary of test status, alongside a vector of failed test names (messages)
typedef struct {
  size_t numTests;
  size_t numPassed;
  size_t numMessages;
  size_t messagesCapacity;
  char const **messages;
} TestStatus;

// in-place constructor
void testStatusInit(TestStatus *);
// adds a test pass
void testStatusPass(TestStatus *);
// adds a test failure, with the name of the failed test
// Note that the test name should be a constant c-string
void testStatusFail(TestStatus *, char const *name);
// displays the test status to stdout
void testStatusDisplay(TestStatus *);
// return status for the testing process
int testStatusStatus(TestStatus *);
// in-place destructor
void testStatusUninit(TestStatus *);

// wrapper function that passes or fails a test depending on the condition
// Note that the test name should be a constant c-string
void test(TestStatus *, char const *name, bool condition);
// prints ANSI escape to suppress previous line of output
void dropLine(void);

#endif  // TLC_TEST_ENGINE_H_