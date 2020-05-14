// Copyright 2020 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileList.h"

#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FileList fileList;

void fileListEntryInit(FileListEntry *entry, char const *inputName,
                       bool isCode) {
  entry->inputFilename = inputName;
  entry->isCode = isCode;
  entry->errored = false;
  entry->program = NULL;
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
          if (strcmp(argv[idx], fileList.entries[searchIdx].inputFilename) == 0) {
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