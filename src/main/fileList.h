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

/**
 * @file
 * file/translation unit tracker
 */

#ifndef TLC_FILE_LIST_H_
#define TLC_FILE_LIST_H_

#include <stdbool.h>
#include <stddef.h>

/** an entry in the filelist */
typedef struct {
  char const *inputName;
  bool isCode;
} FileListEntry;
/**
 * constructs a FileListEntry in-place
 * @param entry entry to create
 * @param inputName name of input file
 * @param isCode is the file a code file?
 */
void fileListEntryInit(FileListEntry *entry, char const *inputName,
                       bool isCode);
/**
 * destroys a FileListEntry in-place
 * @param entry entry to destroy
 */
void fileListEntryUninit(FileListEntry *entry);

/** global file list type */
typedef struct {
  size_t size;
  FileListEntry *entries;
} FileList;

/**
 * creates the global file list object from command line args
 * @param argc number of arguments
 * @param argv argument list, as pointer to c-strings
 * @param numFiles number of files (from parseArgs())
 * @returns status code (0 = OK)
 */
int parseFiles(size_t argc, char const *const *argv, size_t numFiles);

/** global file list object */
extern FileList fileList;

#endif  // TLC_FILE_LIST_H_