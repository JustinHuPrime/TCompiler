// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation of error message handler

#include "util/errorReport.h"

#include <stdio.h>
#include <stdlib.h>

Report *reportCreate(void) {
  Report *report = malloc(sizeof(Report));
  report->messages = vectorCreate();
  report->errors = 0;
  report->warnings = 0;
  return report;
}

void reportMessage(Report *report, char *message) {
  vectorInsert(report->messages, message);
}

void reportError(Report *report, char *message) {
  vectorInsert(report->messages, message);
  report->errors++;
}

void reportWarning(Report *report, char *message) {
  vectorInsert(report->messages, message);
  report->warnings++;
}

void reportDisplay(Report *report) {
  for (size_t idx = 0; idx < report->messages->size; idx++)
    fprintf(stderr, "%s\n", (char *)report->messages->elements[idx]);
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
  vectorDestroy(report->messages, free);
  free(report);
}