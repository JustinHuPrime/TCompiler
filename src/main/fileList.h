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

/**
 * @file
 * file/translation unit tracker
 */

#ifndef TLC_FILE_LIST_H_
#define TLC_FILE_LIST_H_

#include <stdbool.h>
#include <stddef.h>

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "util/container/hashMap.h"

/** an entry in the filelist */
typedef struct FileListEntry {
  bool errored;              /**< has an error been signaled for this entry? */
  char const *inputFilename; /**< path to the input file */
  bool isCode;           /**< does the input file path point to a code file */
  LexerState lexerState; /**< state of the lexer - cleaned up during parse */
  Node *ast; /**< AST for this file - cleaned up at entry to the middleend */
  size_t nextId;  /**< next IR id for this file */
  Vector irFrags; /**< vector of IRFrag - translated IR fragments - cleaned up
                     at entry to the backend */
  void *asmFile;  /**< architecture-specific ASM data - cleaned up after ir
                     output */
} FileListEntry;

/**
 * constructs a FileListEntry in-place
 *
 * @param entry entry to create
 * @param inputName name of input file
 * @param isCode is the file a code file?
 */
void fileListEntryInit(FileListEntry *entry, char const *inputName,
                       bool isCode);

/** global file list type */
typedef struct {
  size_t size;
  FileListEntry *entries;
} FileList;

/**
 * creates the global file list object from command line args
 *
 * @param argc number of arguments
 * @param argv argument list, as pointer to c-strings
 * @param numFiles maximum number of files (from parseArgs())
 * @returns status code (0 = OK)
 */
int parseFiles(size_t argc, char const *const *argv, size_t numFiles);

/**
 * finds the declaration file FileListEntry that matches the specified name node
 *
 * if no name was found, return NULL
 */
FileListEntry *fileListFindDeclName(Node *name);

/** global file list object */
extern FileList fileList;

#endif  // TLC_FILE_LIST_H_