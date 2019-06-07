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

// implemetation for FileList

#include "util/fileList.h"

#include "util/functional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FileList *fileListCreate(void) {
  FileList *list = malloc(sizeof(FileList));
  fileListInit(list);
  return list;
}
void fileListInit(FileList *list) {
  vectorInit(&list->decls);
  vectorInit(&list->codes);
}
void fileListUninit(FileList *list) {
  vectorUninit(&list->decls, nullDtor);
  vectorUninit(&list->codes, nullDtor);
}
void fileListDestroy(FileList *list) {
  fileListUninit(list);
  free(list);
}
void parseFiles(FileList *list, Report *report, Options *options, size_t argc,
                char const *const *argv) {
  fileListInit(list);

  for (size_t idx = 1; idx < argc; idx++) {
    // skip - this is an option
    if (argv[idx][0] == '-') continue;

    size_t length = strlen(argv[idx]);
    if (length > 3 && argv[idx][length - 3] == '.' &&
        argv[idx][length - 2] == 't' && argv[idx][length - 1] == 'c') {
      // this is a code file
      bool duplicated = false;
      for (size_t cidx = 0; cidx < list->codes.size; cidx++) {
        if (strcmp(list->codes.elements[cidx], argv[idx]) == 0) {
          duplicated = true;
          break;
        }
      }
      if (duplicated) {
        switch (optionsGet(options, optionWDuplicateFile)) {
          case O_WT_ERROR: {
            reportError(report, "%s: error: duplicated file\n", argv[idx]);
            break;
          }
          case O_WT_WARN: {
            reportWarning(report, "%s: warning: duplicated file\n", argv[idx]);
          }
          case O_WT_IGNORE: {
            break;
          }
        }
      } else {
#pragma GCC diagnostic push  // generic code demands an unsafe cast
#pragma GCC diagnostic ignored "-Wcast-qual"
        vectorInsert(&list->codes, (char *)argv[idx]);
#pragma GCC diagnostic pop
      }
    } else if (length > 3 && argv[idx][length - 3] == '.' &&
               argv[idx][length - 2] == 't' && argv[idx][length - 1] == 'd') {
      // this is a decl file
      bool duplicated = false;
      for (size_t cidx = 0; cidx < list->decls.size; cidx++) {
        if (strcmp(list->decls.elements[cidx], argv[idx]) == 0) {
          duplicated = true;
          break;
        }
      }
      if (duplicated) {
        switch (optionsGet(options, optionWDuplicateFile)) {
          case O_WT_ERROR: {
            reportError(report, "%s: error: duplicated file\n", argv[idx]);
            break;
          }
          case O_WT_WARN: {
            reportWarning(report, "%s: warning: duplicated file\n", argv[idx]);
          }
          case O_WT_IGNORE: {
            break;
          }
        }
      } else {
#pragma GCC diagnostic push  // generic code demands an unsafe cast
#pragma GCC diagnostic ignored "-Wcast-qual"
        vectorInsert(&list->decls, (char *)argv[idx]);
#pragma GCC diagnostic pop
      }
    } else {
      switch (optionsGet(options, optionWUnrecognizedFile)) {
        case O_WT_ERROR: {
          reportError(report, "%s: error: unrecogized extension\n", argv[idx]);
          break;
        }
        case O_WT_WARN: {
          reportWarning(report, "%s: warning: unrecogized extension\n",
                        argv[idx]);
        }
        case O_WT_IGNORE: {
          break;
        }
      }
    }
  }

  if (list->codes.size == 0)
    reportError(report, "tlc: error: no input code files\n");
}