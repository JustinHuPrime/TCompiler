// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

#include "engine.h"

#include <stdlib.h>

TestStatus *testStatusCreate(void) {
  TestStatus *status = malloc(sizeof(TestStatus));
  status->numTests = 0;
  status->numPassed = 0;
  status->numMessages = 0;
  status->messagesCapacity = 1;
  status->messages = malloc(sizeof(char const *));
  return status;
}
void testStatusPass(TestStatus *status) {
  status->numTests++;
  status->numPassed++;
}
void testStatusFail(TestStatus *status, char const *msg) {
  status->numTests++;
  if (status->messagesCapacity == status->numMessages) {
    status->messages = realloc(
        status->messages, status->messagesCapacity * 2 * sizeof(char const *));
    status->messagesCapacity *= 2;
  }
  status->messages[status->numMessages++] = msg;
}
void testStatusDisplay(TestStatus *status) {
  if (status->numPassed == status->numTests) {
    printf(
        "\x1B[1;32m============================================================"
        "====================\n\nAll %zd tests "
        "passed!\n\n==========================================================="
        "=====================\n\x1B[m",
        status->numTests);
  } else {
    printf(
        "\x1B[1;91m============================================================"
        "====================\n\n%zd out of %zd tests passed.\n%zd tests "
        "failed.\n\n",
        status->numPassed, status->numTests,
        status->numTests - status->numPassed);
    if (status->numMessages != 0) {
      printf("\x1B[4mFailed Tests:\n\x1B[m");
      for (size_t idx = 0; idx < status->numMessages; idx++)
        printf("%s\n", status->messages[idx]);
    }
    printf(
        "\x1B[1;91m\n=========================================================="
        "======================\n\x1B[m");
  }
}
int testStatusStatus(TestStatus *status) {
  return status->numTests != status->numPassed;
}
void testStatusDestroy(TestStatus *status) {
  free(status->messages);
  free(status);
}
void test(TestStatus *status, char const *name, bool condition) {
  if (condition) {
    testStatusPass(status);
  } else {
    testStatusFail(status, name);
  }
}