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

// values of numerical constants used

#include "constants.h"

size_t const BYTE_WIDTH = 1;
size_t const SHORT_WIDTH = 2;
size_t const INT_WIDTH = 4;
size_t const LONG_WIDTH = 8;
size_t const FLOAT_WIDTH = 4;
size_t const DOUBLE_WIDTH = 8;
size_t const POINTER_WIDTH = 8;
size_t const CHAR_WIDTH = 1;
size_t const WCHAR_WIDTH = 4;
size_t const REGISTER_WIDTH = 8;

uint64_t const UBYTE_MAX = 255;
uint64_t const BYTE_MAX = 127;
uint64_t const BYTE_MIN = 128;
uint64_t const USHORT_MAX = 65535;
uint64_t const SHORT_MAX = 32767;
uint64_t const SHORT_MIN = 32768;
uint64_t const UINT_MAX = 4294967295;
uint64_t const INT_MAX = 2147483647;
uint64_t const INT_MIN = 2147483648;
uint64_t const ULONG_MAX = 18446744073709551615UL;
uint64_t const LONG_MAX = 9223372036854775807;
uint64_t const LONG_MIN = 9223372036854775808UL;

char const *VERSION_STRING = "T Language Compiler (tlc) version 0.2.0";