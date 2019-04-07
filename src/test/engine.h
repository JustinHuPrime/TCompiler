// Copyright 2019 Justin Hu
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

// constructor
TestStatus *testStatusCreate(void);

// adds a test pass
void testStatusPass(TestStatus *);
// adds a test failure, with the name of the failed test
void testStatusFail(TestStatus *, char const *name);

// displays the test status to stdout
void testStatusDisplay(TestStatus *);

// return status for the testing process
int testStatusStatus(TestStatus *);

// destructor
void testStatusDestroy(TestStatus *);

// wrapper function that passes or fails a test depending on the condition
void test(TestStatus *, char const *name, bool condition);

#endif  // TLC_TEST_ENGINE_H_