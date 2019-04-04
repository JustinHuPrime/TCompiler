#include "engine.h"

#include <stdlib.h>

TestStatus *testStatusCreate(void) {
  TestStatus *status = malloc(sizeof(TestStatus));
  status->numTests = 0;
  status->numPassed = 0;
  status->numMessages = 0;
  status->messagesCapacity = 0;
  status->messages = NULL;
  return status;
}
void testStatusPass(TestStatus *status) {
  status->numTests++;
  status->numPassed++;
}
void testStatusFail(TestStatus *status, char const *msg) {
  status->numTests++;
  if (status->messagesCapacity == status->numMessages) {
    if (status->messagesCapacity == 0) {
      status->messages = malloc(sizeof(char const *));
      status->messagesCapacity = 1;
    }
    status->messages = realloc(
        status->messages, status->messagesCapacity * 2 * sizeof(char const *));
    status->messagesCapacity *= 2;
  }
  status->messages[status->numMessages++] = msg;
}
void testStatusDisplay(TestStatus *status, FILE *where) {
  fprintf(where, "Test summary:\n%zd out of %zd tests passed.\n",
          status->numPassed, status->numTests);
  if (status->numMessages != 0) {
    fprintf(where, "Tests failed:\n");
    for (size_t idx = 0; idx < status->numMessages; idx++)
      fprintf(where, "%s\n", status->messages[idx]);
  }
}
int testStatusStatus(TestStatus *status) {
  return status->numTests != status->numPassed;
}
void testStatusDestroy(TestStatus *status) { free(status); }
void test(TestStatus *status, char const *name, bool condition) {
  if (condition) {
    testStatusPass(status);
  } else {
    testStatusFail(status, name);
  }
}