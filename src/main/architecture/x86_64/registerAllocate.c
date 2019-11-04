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

// implementation of x86_64 register allocation

#include "architecture/x86_64/registerAllocate.h"

#include "architecture/x86_64/assembly.h"

static void registerAllocateFragment(X86_64Fragment *frag) {
  // TODO: write this
}
void x86_64RegisterAllocate(FileX86_64FileMap *asmFileMap) {
  for (size_t fileIdx = 0; fileIdx < asmFileMap->capacity; fileIdx++) {
    if (asmFileMap->keys[fileIdx] != NULL) {
      X86_64File *file = asmFileMap->values[fileIdx];
      for (size_t fragmentIdx = 0; fragmentIdx < file->fragments.size;
           fragmentIdx++) {
        X86_64Fragment *frag = file->fragments.elements[fragmentIdx];
        if (frag->kind = X86_64_FK_TEXT) {
          registerAllocateFragment(frag);
        }
      }
    }
  }
}