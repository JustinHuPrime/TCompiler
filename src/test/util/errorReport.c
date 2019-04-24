// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for the error report

#include "util/errorReport.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void errorReportTest(TestStatus *status) {
  Report *report;

  // ctor
  report = reportCreate();
  test(
      status,
      "[util] [errorReport] [constructor] error report created has no messages",
      report->messagesSize == 0);
  test(status,
       "[util] [errorReport] [constructor] error report created has capacity "
       "one",
       report->messagesCapacity == 1);
  test(status,
       "[util] [errorReport] [constructor] error report created does not have "
       "null pointer to messages",
       report->messages != NULL);
  test(status,
       "[util] [errorReport] [constructor] error report created has no errors",
       report->errors == 0);
  test(
      status,
      "[util] [errorReport] [constructor] error report created has no warnings",
      report->warnings == 0);

  // reportMessage
  char *message = strcpy(malloc(13), "test message");
  reportMessage(report, message);
  test(status,
       "[util] [errorReport] [reportMessage] adding a message adds to the size",
       report->messagesSize == 1);
  test(status,
       "[util] [errorReport] [reportMessage] message is added to messages",
       report->messages[0] == message);
  test(status,
       "[util] [errorReport] [reportMessage] adding a message does not add an "
       "error",
       report->errors == 0);
  test(status,
       "[util] [errorReport] [reportMessage] adding a message does not add a "
       "warning",
       report->warnings == 0);
  test(status,
       "[util] [errorReport] [reportMessage] capacity changes appropriately",
       report->messagesCapacity == 1);

  // reportError
  message = strcpy(malloc(11), "test error");
  reportError(report, message);
  test(status,
       "[util] [errorReport] [reportError] adding an error adds to the size",
       report->messagesSize == 2);
  test(status, "[util] [errorReport] [reportError] error is added to messages",
       report->messages[1] == message);
  test(status,
       "[util] [errorReport] [reportError] adding an error adds an "
       "error",
       report->errors == 1);
  test(status,
       "[util] [errorReport] [reportError] adding an error does not add a "
       "warning",
       report->warnings == 0);
  test(status,
       "[util] [errorReport] [reportError] capacity changes appropriately",
       report->messagesCapacity == 2);

  // reportWarning
  message = strcpy(malloc(13), "test warning");
  reportWarning(report, message);
  test(status,
       "[util] [errorReport] [reportWarning] adding a warning adds to the size",
       report->messagesSize == 3);
  test(status,
       "[util] [errorReport] [reportWarning] warning is added to messages",
       report->messages[2] == message);
  test(status,
       "[util] [errorReport] [reportWarning] adding an warning does not add an "
       "error",
       report->errors == 1);
  test(status,
       "[util] [errorReport] [reportWarning] adding a warning adds a warning",
       report->warnings == 1);
  test(status,
       "[util] [errorReport] [reportWarning] capacity changes appropriately",
       report->messagesCapacity == 4);

  // reportState
  report->errors = 0;
  report->warnings = 0;
  test(status,
       "[util] [errorReport] [reportState] no errors and no warnings gives "
       "RPT_OK",
       reportState(report) == RPT_OK);
  report->warnings = 2;
  test(status,
       "[util] [errorReport] [reportState] warning with no errors gives "
       "RPT_WARN",
       reportState(report) == RPT_WARN);
  report->errors = 2;
  report->warnings = 0;
  test(
      status,
      "[util] [errorReport] [reportState] error with no warnings gives RPT_ERR",
      reportState(report) == RPT_ERR);
  report->warnings = 2;
  test(status,
       "[util] [errorReport] [reportState] error with warnings gives RPT_ERR",
       reportState(report) == RPT_ERR);

  reportDestroy(report);
}