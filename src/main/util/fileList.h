// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// a pair of vectors of file names - POD object
// file names are not sorted - expecting that build order of names matters

#ifndef TLC_UTIL_FILELIST_H_
#define TLC_UTIL_FILELIST_H_

#include "ast/ast.h"
#include "util/errorReport.h"
#include "util/options.h"

#include <stddef.h>

typedef struct {
  size_t numDecl;
  char const **decls;

  size_t numCode;
  char const **codes;
} FileList;

// ctor
FileList *sortFiles(Report *report, Options *options, size_t argc, char **argv);
// dtor
void fileListDestroy(FileList *);

#endif  // TLC_UTIL_FILELIST_H_