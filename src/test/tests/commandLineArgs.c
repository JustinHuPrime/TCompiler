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

  // // --arch=x86_64-linux
  // argc = 3;
  // char const *const argv1[] = {
  //     "./tlc",
  //     "--arch=x86_64-linux",
  //     "foo.tc",
  // };
  // retval = parseArgs(argc, argv1, &numFiles);

  // test("command line with arch=x86_64-linux passes", retval == 0);
  // test("arch=x86_64-linux option is correctly set",
  //      options.arch == OPTION_A_X86_64_LINUX);

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
  test("duplicate-file option is correctly set",
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
  test("duplicate-file option is correctly set",
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
  test("duplicate-file option is correctly set",
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
  test("unrecognized-file option is correctly set",
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
  test("unrecognized-file option is correctly set",
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
  test("unrecognized-file option is correctly set",
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
  test("debug-dump option is correctly set", options.dump == OPTION_DD_NONE);

  // --debug-dump=lex
  argc = 3;
  char const *const argv12[] = {
      "./tlc",
      "--debug-dump=lex",
      "foo.tc",
  };
  retval = parseArgs(argc, argv12, &numFiles);

  test("command line with debug-dump=lex passes", retval == 0);
  test("debug-dump option is correctly set", options.dump == OPTION_DD_LEX);

  // --debug-dump=parse-structure
  argc = 3;
  char const *const argv13[] = {
      "./tlc",
      "--debug-dump=parse",
      "foo.tc",
  };
  retval = parseArgs(argc, argv13, &numFiles);

  test("command line with debug-dump=parse passes", retval == 0);
  test("debug-dump option is correctly set", options.dump == OPTION_DD_PARSE);
}

void testCommandLineArgs(void) {
  testNumFilesCounting();
  testOptions();
}