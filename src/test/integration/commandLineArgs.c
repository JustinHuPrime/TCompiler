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
 * integration tests for command line arguments
 */

#include "engine.h"
#include "fileList.h"
#include "tests.h"
#include "util/options.h"

#include <string.h>

void integrationTestCommandLineArgs(void) {
  char const *const argv[] = {
      "./tlc", "foo.tc", "bar.td", "--debug-dump=lex", "--", "--baz.tc",
  };
  size_t argc = 6;

  size_t numFiles;

  test("[integration] [command line args] good arg string passes parseArgs",
       parseArgs(argc, argv, &numFiles) == 0);

  test("[integration] [command line args] debug dump option is set to lex",
       options.dump == OPTION_DD_LEX);

  test("[integration] [command line args] good arg string passes parseFiles",
       parseFiles(argc, argv, numFiles) == 0);

  test("[integration] [command line args] file list produced is 3 long",
       fileList.size == 3);

  test("[integration] [command line args] first file is foo.tc (code file)",
       strcmp(fileList.entries[0].inputFile, "foo.tc") == 0 &&
           fileList.entries[0].isCode);

  test(
      "[integration] [command line args] second file is bar.td (declaration "
      "file)",
      strcmp(fileList.entries[1].inputFile, "bar.td") == 0 &&
          !fileList.entries[1].isCode);

  test("[integration] [command line args] third file is --baz.tc (code file)",
       strcmp(fileList.entries[2].inputFile, "--baz.tc") == 0 &&
           fileList.entries[2].isCode);
}