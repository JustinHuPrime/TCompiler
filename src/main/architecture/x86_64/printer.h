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

// x86_64 assembly printer

#ifndef TLC_ARCHITECTURE_X86_64_PRINTER_H_
#define TLC_ARCHITECTURE_X86_64_PRINTER_H_

struct X86_64File;

void dumpX86_64File(struct X86_64File *);

#endif  // TLC_ARCHITECTURE_X86_64_PRINTER_H_