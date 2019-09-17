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

// implementation of x86_64 function frames

#include "architecture/x86_64/frame.h"

#include "ir/frame.h"

typedef struct X86_64Frame {
  Frame frame;
} X86_64Frame;
FrameVTable *X86_64VTable = NULL;
Frame *x86_64FrameCtor(void) {
  return NULL;  // TODO: write this
}

typedef struct X86_64GlobalAccess {
  Access access;
} X86_64GlobalAccess;
AccessVTable *X86_64GlobalAccessVTable = NULL;
Access *x86_64GlobalAccessCtor(char *label) {
  return NULL;  // TODO: write this
}

typedef struct X86_64TempAccess {
  Access access;
} X86_64TempAccess;
AccessVTable *X86_64TempAccessVTable = NULL;

typedef struct X86_64MemoryAccess {
  Access access;
} X86_64MemoryAccess;
AccessVTable *X86_64MemoryAccessVTable = NULL;
