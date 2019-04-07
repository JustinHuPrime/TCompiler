// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of error message handler

#include "util/errorReport.h"

#include <stdio.h>
#include <stdlib.h>

Report *reportCreate(void) {
  Report *report = malloc(sizeof(Report));
  report->messagesSize = 0;
  report->messagesCapacity = 1;
  report->messages = malloc(sizeof(char *));
  report->errors = 0;
  report->warnings = 0;
  return report;
}

void reportMessage(Report *report, char *message) {
  if (report->messagesSize == report->messagesCapacity) {
    report->messagesCapacity *= 2;
    report->messages =
        realloc(report->messages, report->messagesCapacity * sizeof(char *));
  }
  report->messages[report->messagesSize++] = message;
}

void reportError(Report *report, char *message) {
  reportMessage(report, message);
  report->errors++;
}

void reportWarning(Report *report, char *message) {
  reportMessage(report, message);
  report->warnings++;
}

void reportDisplay(Report *report) {
  for (size_t idx = 0; idx < report->messagesSize; idx++)
    fprintf(stderr, "%s\n", report->messages[idx]);
}

int const RPT_OK = 0;
int const RPT_ERR = -1;
int const RPT_WARN = -2;
int reportState(Report *report) {
  if (report->errors != 0) {
    return RPT_ERR;
  } else if (report->warnings != 0) {
    return RPT_WARN;
  } else {
    return RPT_OK;
  }
}

void reportDestroy(Report *report) {
  for (size_t idx = 0; idx < report->messagesSize; idx++)
    free(report->messages[idx]);
  free(report->messages);
  free(report);
}