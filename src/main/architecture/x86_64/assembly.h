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

// x86_64 assembly representation

#ifndef TLC_ARCHITECTURE_X86_64_64_ASSEMBLY_H_
#define TLC_ARCHITECTURE_X86_64_64_ASSEMBLY_H_

#include "util/container/vector.h"

typedef struct {
  void *todo;  // TODO: define this
} X86_64Asm;

typedef Vector X86_64AsmVector;

#endif  // TLC_ARCHITECTURE_X86_64_64_ASSEMBLY_H_