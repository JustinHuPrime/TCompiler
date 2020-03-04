// Copyright 2020 Justin Hu
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

#include "fileList.h"

#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FileList fileList;

void fileListEntryInit(FileListEntry *entry, char const *inputName,
                       bool isCode) {
  entry->inputFile = inputName;
  entry->isCode = isCode;
}

int parseFiles(size_t argc, char const *const *argv, size_t numFiles) {
  int err = 0;

  // setup the fileList
  fileList.size = 0;  // eventually going to be at most numFiles long
  fileList.entries = malloc(sizeof(FileListEntry) * numFiles);

  // read the args
  bool allFiles = false;
  for (size_t idx = 1; idx < argc; idx++) {
    if (argv[idx][0] != '-' || allFiles) {
      // not an option, deal with the file
      size_t length = strlen(argv[idx]);
      bool recognized = false;
      bool isCode;
      if (length > 3 && strcmp(argv[idx] + (length - 3), ".tc") == 0) {
        // is a code file
        recognized = true;
        isCode = true;
      } else if (length > 3 && strcmp(argv[idx] + (length - 3), ".td") == 0) {
        // is a decl file
        recognized = true;
        isCode = false;
      } else {
        // unrecognized
        switch (options.unrecognizedFile) {
          case OPTION_W_ERROR: {
            fprintf(stderr, "%s: error: unrecognized extension\n", argv[idx]);
            err = -1;
            break;
          }
          case OPTION_W_WARN: {
            fprintf(stderr, "%s: warning: unrecognized extension\n", argv[idx]);
            break;
          }
          case OPTION_W_IGNORE: {
            break;
          }
        }
      }
      if (recognized) {
        // search for duplicates
        bool duplicate = false;
        for (size_t searchIdx = 0; searchIdx < fileList.size; searchIdx++) {
          if (strcmp(argv[idx], fileList.entries[searchIdx].inputFile) == 0) {
            duplicate = true;
            break;
          }
        }
        if (duplicate) {
          switch (options.duplicateFile) {
            case OPTION_W_ERROR: {
              fprintf(stderr, "%s: error: duplicated file\n", argv[idx]);
              err = -1;
              break;
            }
            case OPTION_W_WARN: {
              fprintf(stderr, "%s: warning: duplicated file\n", argv[idx]);
              break;
            }
            case OPTION_W_IGNORE: {
              break;
            }
          }
        } else {
          fileListEntryInit(fileList.entries + fileList.size, argv[idx],
                            isCode);
          fileList.size++;
        }
      }
    } else if (strcmp(argv[idx], "--") == 0) {
      allFiles = true;
    }
  }

  // shrink down to size
  fileList.entries =
      realloc(fileList.entries, sizeof(FileListEntry) * fileList.size);

  // need at least one code file
  bool noCodes = true;
  for (size_t idx = 0; idx < fileList.size; idx++) {
    if (fileList.entries[idx].isCode) {
      noCodes = false;
      break;
    }
  }
  if (noCodes) {
    fprintf(stderr, "tlc: error: no code files provided\n");
    err = -1;
  }

  return err;
}