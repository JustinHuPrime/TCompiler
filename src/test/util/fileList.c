// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Tests for the file list

#include "util/fileList.h"

#include "tests.h"

#include <stdlib.h>
#include <string.h>

void fileListTest(TestStatus *status) {
  Report *report = reportCreate();
  Options *options = parseOptions(report, 1, NULL);
  char const *good[] = {"./tlc", "foo.tc", "bar.td"};
  char const *badExt[] = {"./tlc", "bad.badext", "good.tc"};
  char const *badDup[] = {"./tlc", "dup.tc", "dup.tc"};
  FileList *fileList;

  // ctor
  fileList = parseFiles(report, options, 1, NULL);
  test(status,
       "[util] [errorReport] [fileList] empty list produces no code files",
       fileList->codes->size == 0);
  test(status,
       "[util] [errorReport] [fileList] empty list produces no decl files",
       fileList->decls->size == 0);
  fileListDestroy(fileList);

  fileList = parseFiles(report, options, 3, good);
  test(status,
       "[util] [errorReport] [fileList] code file is parsed as code file",
       fileList->codes->elements[0] == good[1]);
  test(status,
       "[util] [errorReport] [fileList] decl file is parsed as decl file",
       fileList->decls->elements[0] == good[2]);
  fileListDestroy(fileList);

  fileList = parseFiles(report, options, 3, badExt);
  test(status, "[util] [errorReport] [fileList] badExt is caught",
       reportState(report) == RPT_ERR);
  test(status,
       "[util] [errorReport] [fileList] further files are processed correctly",
       fileList->codes->elements[0] == badExt[2]);
  fileListDestroy(fileList);

  fileList = parseFiles(report, options, 3, badDup);
  test(status, "[util] [errorReport] [fileList] duplicate is caught",
       reportState(report) == RPT_ERR);
  test(status,
       "[util] [errorReport] [fileList] first file is processed correctly",
       fileList->codes->elements[0] == badDup[1]);
  fileListDestroy(fileList);

  optionsDestroy(options);
  reportDestroy(report);
}