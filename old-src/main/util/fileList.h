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

// a pair of vectors of file names - POD object
// file names are not sorted - expecting that build order of names matters

#ifndef TLC_UTIL_FILELIST_H_
#define TLC_UTIL_FILELIST_H_

#include "ast/ast.h"
#include "util/container/vector.h"
#include "util/errorReport.h"
#include "util/options.h"

#include <stddef.h>

// vector of two categories of char const *
typedef struct {
  Vector decls;
  Vector codes;
} FileList;

// ctor
FileList *fileListCreate(void);
// in-place ctor
void fileListInit(FileList *);
// in-place dtor
void fileListUninit(FileList *);
// dtor
void fileListDestroy(FileList *);

// initializes a fileList object given the options, argc, and argv, reporting
// errors into report
void parseFiles(FileList *, Report *report, Options const *options, size_t argc,
                char const *const *argv);

#endif  // TLC_UTIL_FILELIST_H_