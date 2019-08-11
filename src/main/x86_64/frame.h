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

// x86_64 frame definitions

#ifndef TLC_X86_64_FRAME_H_
#define TLC_X86_64_FRAME_H_

#include "ir/ir.h"

Frame *x86_64FrameCtor(void);
Access *x86_64GlobalAccessCtor(char *label);
LabelGenerator *x86_64LabelGeneratorCtor(void);

#endif  // TLC_X86_64_FRAME_H_