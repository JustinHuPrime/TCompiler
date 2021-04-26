// Copyright 2019-2020 Justin Hu
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
 * test status engine
 */

#ifndef TLC_TEST_ENGINE_H_
#define TLC_TEST_ENGINE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/**
 * summary of test status, alongside a vector of failed test names (messages)
 */
typedef struct {
  size_t numTests;
  size_t numPassed;
} TestStatus;

/** global status object */
extern TestStatus status;

/** initializes status */
void testStatusInit(void);
/**
 * return status for the testing process.
 * @returns status: 0 = OK, -1 = failed
 */
int testStatusStatus(void);

/**
 * wrapper function that passes or fails a test depending on the condition. Note
 * that the test name should be a constant c-string
 *
 * @param name name of test
 * @param condition did the test pass?
 */
void test(char const *name, bool condition);
void testDynamic(char *name, bool condition);

#endif  // TLC_TEST_ENGINE_H_