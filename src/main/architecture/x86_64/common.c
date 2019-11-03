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

// implementation of common x86_64 functions

#include "architecture/x86_64/common.h"

X86_64Register x86_64RegNumToRegister(size_t reg) { return reg; }
bool x86_64RegIsSSE(X86_64Register reg) {
  return X86_64_XMM0 <= reg && reg <= X86_64_XMM15;
}