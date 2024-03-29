// Copyright 2020-2021 Justin Hu
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
 * tests for command line arguments
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "fileList.h"
#include "options.h"
#include "tests.h"

static void testNumFilesCounting(void) {
  size_t argc;
  size_t numFiles;
  int retval;

  // only files
  argc = 4;
  char const *const argv1[] = {
      "./tlc",
      "foo.tc",
      "foo.td",
      "folder/bar.tc",
  };
  retval = parseArgs(argc, argv1, &numFiles);

  test("command line with only files passes", retval == 0);
  test("number of files with only files is correct", numFiles == 3);

  // files with options
  argc = 5;
  char const *const argv2[] = {"./tlc", "foo.tc", "foo.td", "folder/bar.tc",
                               "-Wduplicate-file=error"};
  retval = parseArgs(argc, argv2, &numFiles);

  test("command line with files and options passes", retval == 0);
  test("number of files with files and options is correct", numFiles == 3);

  // files with dash and options
  argc = 7;
  char const *const argv3[] = {
      "./tlc",         "foo.tc", "foo.td",    "-Wduplicate-file=error",
      "folder/bar.tc", "--",     "-other.td",
  };
  retval = parseArgs(argc, argv3, &numFiles);

  test("command line with files, options, and dashes passes", retval == 0);
  test("number of files with files, options, and dashes is correct",
       numFiles == 4);

  // no files - not yet an error
  argc = 1;
  char const *const argv4[] = {
      "./tlc",
  };
  retval = parseArgs(argc, argv4, &numFiles);

  test("command line with nothing passes", retval == 0);
  test("number of files with nothing is correct", numFiles == 0);
}

static void testOptions(void) {
  size_t argc;
  size_t numFiles;
  int retval;

  // bad option
  argc = 2;
  char const *const argv0[] = {
      "./tlc",
      "--__BAD_OPTION__",
  };
  retval = parseArgs(argc, argv0, &numFiles);
  test("command line with bad option fails", retval != 0);

  // --arch=x86_64-linux
  argc = 3;
  char const *const argv1[] = {
      "./tlc",
      "--arch=x86_64-linux",
      "foo.tc",
  };
  retval = parseArgs(argc, argv1, &numFiles);

  test("command line with arch=x86_64-linux passes", retval == 0);
  test("arch=x86_64-linux option is correctly set",
       options.arch == OPTION_A_X86_64_LINUX);

  // // -fPDC
  // argc = 3;
  // char const *const argv2[] = {
  //     "./tlc",
  //     "-fPDC",
  //     "foo.tc",
  // };
  // retval = parseArgs(argc, argv2, &numFiles);

  // test("command line with PDC passes", retval == 0);
  // test("PDC option is correctly set",
  //      options.positionDependence == OPTION_PD_PDC);

  // // -fPIE
  // argc = 3;
  // char const *const argv3[] = {
  //     "./tlc",
  //     "-fPIE",
  //     "foo.tc",
  // };
  // retval = parseArgs(argc, argv3, &numFiles);

  // test("command line with PIE passes", retval == 0);
  // test("PIE option is correctly set",
  //      options.positionDependence == OPTION_PD_PIE);

  // // -fPIC
  // argc = 3;
  // char const *const argv4[] = {
  //     "./tlc",
  //     "-fPIC",
  //     "foo.tc",
  // };
  // retval = parseArgs(argc, argv4, &numFiles);

  // test("command line with PIC passes", retval == 0);
  // test("PIE option is correctly set",
  //      options.positionDependence == OPTION_PD_PIC);

  // -Wduplicate-file=error
  argc = 3;
  char const *const argv5[] = {
      "./tlc",
      "-Wduplicate-file=error",
      "foo.tc",
  };
  retval = parseArgs(argc, argv5, &numFiles);

  test("command line with duplicate-file=error passes", retval == 0);
  test("duplicate-file error option is correctly set",
       options.duplicateFile == OPTION_W_ERROR);

  // -Wduplicate-file=warn
  argc = 3;
  char const *const argv6[] = {
      "./tlc",
      "-Wduplicate-file=warn",
      "foo.tc",
  };
  retval = parseArgs(argc, argv6, &numFiles);

  test("command line with duplicate-file=warn passes", retval == 0);
  test("duplicate-file warn option is correctly set",
       options.duplicateFile == OPTION_W_WARN);

  // -Wduplicate-file=ignore
  argc = 3;
  char const *const argv7[] = {
      "./tlc",
      "-Wduplicate-file=ignore",
      "foo.tc",
  };
  retval = parseArgs(argc, argv7, &numFiles);

  test("command line with duplicate-file=ignore passes", retval == 0);
  test("duplicate-file ignore option is correctly set",
       options.duplicateFile == OPTION_W_IGNORE);

  // -Wunrecognized-file=error
  argc = 3;
  char const *const argv8[] = {
      "./tlc",
      "-Wunrecognized-file=error",
      "foo.tc",
  };
  retval = parseArgs(argc, argv8, &numFiles);

  test("command line with unrecognized-file=error passes", retval == 0);
  test("unrecognized-file error option is correctly set",
       options.unrecognizedFile == OPTION_W_ERROR);

  // -Wunrecognized-file=warn
  argc = 3;
  char const *const argv9[] = {
      "./tlc",
      "-Wunrecognized-file=warn",
      "foo.tc",
  };
  retval = parseArgs(argc, argv9, &numFiles);

  test("command line with unrecognized-file=warn passes", retval == 0);
  test("unrecognized-file warn option is correctly set",
       options.unrecognizedFile == OPTION_W_WARN);

  // -Wunrecognized-file=ignore
  argc = 3;
  char const *const argv10[] = {
      "./tlc",
      "-Wunrecognized-file=ignore",
      "foo.tc",
  };
  retval = parseArgs(argc, argv10, &numFiles);

  test("command line with unrecognized-file=ignore passes", retval == 0);
  test("unrecognized-file ignore option is correctly set",
       options.unrecognizedFile == OPTION_W_IGNORE);

  // --debug-dump=none
  argc = 3;
  char const *const argv11[] = {
      "./tlc",
      "--debug-dump=none",
      "foo.tc",
  };
  retval = parseArgs(argc, argv11, &numFiles);

  test("command line with debug-dump=none passes", retval == 0);
  test("debug-dump none option is correctly set",
       options.dump == OPTION_DD_NONE);

  // --debug-dump=lex
  argc = 3;
  char const *const argv12[] = {
      "./tlc",
      "--debug-dump=lex",
      "foo.tc",
  };
  retval = parseArgs(argc, argv12, &numFiles);

  test("command line with debug-dump=lex passes", retval == 0);
  test("debug-dump lex option is correctly set", options.dump == OPTION_DD_LEX);

  // --debug-dump=parse
  argc = 3;
  char const *const argv13[] = {
      "./tlc",
      "--debug-dump=parse",
      "foo.tc",
  };
  retval = parseArgs(argc, argv13, &numFiles);

  test("command line with debug-dump=parse passes", retval == 0);
  test("debug-dump parse option is correctly set",
       options.dump == OPTION_DD_PARSE);

  // --debug-dump=translation
  argc = 3;
  char const *const argv14[] = {
      "./tlc",
      "--debug-dump=translation",
      "foo.tc",
  };
  retval = parseArgs(argc, argv14, &numFiles);

  test("command line with debug-dump=translation passes", retval == 0);
  test("debug-dump ir option is correctly set",
       options.dump == OPTION_DD_TRANSLATION);

  // --debug-validate-ir
  argc = 3;
  char const *const argv15[] = {
      "./tlc",
      "--debug-validate-ir",
      "foo.tc",
  };
  retval = parseArgs(argc, argv15, &numFiles);

  test("command line with debug-validate-ir passes", retval == 0);
  test("debug-validate-ir option is correctly set",
       options.debugValidateIr == true);

  argc = 3;
  char const *const argv16[] = {
      "./tlc",
      "--no-debug-validate-ir",
      "foo.tc",
  };
  retval = parseArgs(argc, argv16, &numFiles);

  test("command line with no-debug-validate-ir passes", retval == 0);
  test("debug-validate-ir option is correctly unset",
       options.debugValidateIr == false);

  // --debug-dump=blocked-optimization
  argc = 3;
  char const *const argv17[] = {
      "./tlc",
      "--debug-dump=blocked-optimization",
      "foo.tc",
  };
  retval = parseArgs(argc, argv17, &numFiles);

  test("command line with debug-dump=blocked-optimization passes", retval == 0);
  test("debug-dump ir option is correctly set",
       options.dump == OPTION_DD_BLOCKED_OPTIMIZATION);

  // --debug-dump=trace-scheduling
  argc = 3;
  char const *const argv18[] = {
      "./tlc",
      "--debug-dump=trace-scheduling",
      "foo.tc",
  };
  retval = parseArgs(argc, argv18, &numFiles);

  test("command line with debug-dump=trace-scheduling passes", retval == 0);
  test("debug-dump ir option is correctly set",
       options.dump == OPTION_DD_TRACE_SCHEDULING);

  // --debug-dump=scheduled-optimization
  argc = 3;
  char const *const argv19[] = {
      "./tlc",
      "--debug-dump=scheduled-optimization",
      "foo.tc",
  };
  retval = parseArgs(argc, argv19, &numFiles);

  test("command line with debug-dump=scheduled-optimization passes",
       retval == 0);
  test("debug-dump ir option is correctly set",
       options.dump == OPTION_DD_SCHEDULED_OPTIMIZATION);
}

void testCommandLineArgs(void) {
  assert("can't bless commandLineArgs tests" && !status.bless);

  Options original;
  memcpy(&original, &options, sizeof(Options));

  testNumFilesCounting();
  testOptions();

  memcpy(&options, &original, sizeof(Options));
}