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

// Implementation of error message handler

#include "util/errorReport.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

Report *reportCreate(void) {
  Report *report = malloc(sizeof(Report));
  reportInit(report);
  return report;
}
void reportInit(Report *report) {
  report->errors = 0;
  report->warnings = 0;
}

void reportMessage(Report *report, char const *format, ...) {
  (void)report;  // argument kept for consistency
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
}

void reportError(Report *report, char const *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
  report->errors++;
}

void reportWarning(Report *report, char const *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
  report->warnings++;
}

int const RPT_OK = 0;
int const RPT_ERR = -1;
int const RPT_WARN = -2;
int reportState(Report const *report) {
  if (report->errors != 0) {
    return RPT_ERR;
  } else if (report->warnings != 0) {
    return RPT_WARN;
  } else {
    return RPT_OK;
  }
}

void reportUninit(Report *report) {
  (void)report;  // null, kept for consistency
}
void reportDestroy(Report *report) {
  reportUninit(report);
  free(report);
}