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

// Tests for the error report

#include "util/errorReport.h"

#include "unitTests/tests.h"

#include <stdlib.h>
#include <string.h>

void errorReportTest(TestStatus *status) {
  Report *report;

  // ctor
  report = reportCreate();
  test(status,
       "[util] [errorReport] [constructor] error report created has no errors",
       report->errors == 0);
  test(
      status,
      "[util] [errorReport] [constructor] error report created has no warnings",
      report->warnings == 0);

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