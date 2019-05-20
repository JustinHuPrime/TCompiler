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

// three-valued logic

#ifndef TLC_UTIL_TERNARY_H_
#define TLC_UTIL_TERNARY_H_

typedef enum {
  NO = 0,
  YES = 1,
  INDETERMINATE = -1,
} TernaryValue;

TernaryValue and (TernaryValue, TernaryValue);
TernaryValue or (TernaryValue, TernaryValue);
TernaryValue not(TernaryValue);

#endif  // TLC_UTIL_TERNARY_H_