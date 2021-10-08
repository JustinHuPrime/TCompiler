// Copyright 2021 Justin Hu
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

#ifndef TLC_ARCH_X86_64_LINUX_IRVALIDATION_H_
#define TLC_ARCH_X86_64_LINUX_IRVALIDATION_H_

#include <stdbool.h>

/**
 * checks that all files in the file list have valid IR
 *
 * This checks that
 *  - all registers referenced are of normal size
 *
 * @param phase phase to blame for errors
 * @param blocked is the IR in blocks
 * @returns -1 on failure, 0 on success
 */
int x86_64LinuxValidateIRArchSpecific(char const *phase, bool blocked);

#endif  // TLC_ARCH_X86_64_LINUX_IRVALIDATION_H_