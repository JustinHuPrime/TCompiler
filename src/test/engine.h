#ifndef TLC_TEST_ENGINE_H_
#define TLC_TEST_ENGINE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
  size_t numTests;
  size_t numPassed;
  size_t numMessages;
  size_t messagesCapacity;
  char const **messages;
} TestStatus;

TestStatus *testStatusCreate(void);
void testStatusPass(TestStatus *);
void testStatusFail(TestStatus *, char const *);
void testStatusDisplay(TestStatus *, FILE *);
int testStatusStatus(TestStatus *);
void testStatusDestroy(TestStatus *);
void test(TestStatus *, char const *, bool);

#endif  // TLC_TEST_ENGINE_H_