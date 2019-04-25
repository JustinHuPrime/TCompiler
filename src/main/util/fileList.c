// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// implemetation for FileList

#include "util/fileList.h"

#include "util/format.h"
#include "util/functional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ctor
FileList *parseFiles(Report *report, Options *options, size_t argc,
                     char const *const *argv) {
  FileList *list = malloc(sizeof(FileList));

  list->decls = vectorCreate();
  list->codes = vectorCreate();

  for (size_t idx = 1; idx < argc; idx++) {
    // skip - this is an option
    if (argv[idx][0] == '-') continue;

    size_t length = strlen(argv[idx]);
    if (length > 3 && argv[idx][length - 3] == '.' &&
        argv[idx][length - 2] == 't' && argv[idx][length - 1] == 'c') {
      // this is a code file
      bool duplicated = false;
      for (size_t cidx = 0; cidx < list->codes->size; cidx++) {
        if (strcmp(list->codes->elements[cidx], argv[idx]) == 0) {
          duplicated = true;
          break;
        }
      }
      if (duplicated && getOpt(options, OPT_WARN_DUPLICATE_FILE)) {
        if (getOpt(options, OPT_WARN_DUPLICATE_FILE_ERROR)) {
          reportError(report, format("%s: error: duplicated file", argv[idx]));
        } else {
          reportWarning(report,
                        format("%s: warning: duplicated file", argv[idx]));
        }
      } else {
#pragma GCC diagnostic push  // generic code demands an unsafe cast
#pragma GCC diagnostic ignored "-Wcast-qual"
        vectorInsert(list->codes, (char *)argv[idx]);
#pragma GCC diagnostic pop
      }
    } else if (length > 3 && argv[idx][length - 3] == '.' &&
               argv[idx][length - 2] == 't' && argv[idx][length - 1] == 'd') {
      // this is a decl file
      bool duplicated = false;
      for (size_t cidx = 0; cidx < list->decls->size; cidx++) {
        if (strcmp(list->decls->elements[cidx], argv[idx]) == 0) {
          duplicated = true;
          break;
        }
      }
      if (duplicated && getOpt(options, OPT_WARN_DUPLICATE_FILE)) {
        if (getOpt(options, OPT_WARN_DUPLICATE_FILE_ERROR)) {
          reportError(report, format("%s: error: duplicated file", argv[idx]));
        } else {
          reportWarning(report,
                        format("%s: warning: duplicated file", argv[idx]));
        }
      } else {
#pragma GCC diagnostic push  // generic code demands an unsafe cast
#pragma GCC diagnostic ignored "-Wcast-qual"
        vectorInsert(list->decls, (char *)argv[idx]);
#pragma GCC diagnostic pop
      }
    } else {
      if (getOpt(options, OPT_WARN_UNRECOGNIZED_FILE)) {
        if (getOpt(options, OPT_WARN_UNRECOGNIZED_FILE_ERROR)) {
          reportError(report,
                      format("%s: error: unrecogized extension", argv[idx]));
        } else {
          reportWarning(
              report, format("%s: warning: unrecogized extension", argv[idx]));
        }
      }
    }
  }

  if (list->codes == 0)
    reportError(report, format("tlc: error: no input code files"));

  return list;
}
// dtor
void fileListDestroy(FileList *list) {
  vectorDestroy(list->decls, nullDtor);
  vectorDestroy(list->codes, nullDtor);
  free(list);
}