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

// Utilities for names, scoped or otherwise

#ifndef TLC_UTIL_NAME_UTILS_H_
#define TLC_UTIL_NAME_UTILS_H_

#include <stdbool.h>

// produces whether or not the name has a colon (if it has one, it has to be
// scoped)
bool isScoped(char const *);
// splits a name into the module and the short name (must be scoped)
void splitName(char const *fullName, char **module, char **shortName);

#endif  // TLC_UTIL_NAME_UTILS_H_