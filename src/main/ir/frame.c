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

// shorthands for frame.h vtable functions

#include "ir/frame.h"

#include "ir/ir.h"
#include "typecheck/symbolTable.h"

void accessDtor(Access *this) { this->vtable->dtor(this); }
IROperand *accessLoad(Access *this, IREntryVector *code,
                      TempAllocator *tempAllocator) {
  return this->vtable->load(this, code, tempAllocator);
}
void accessStore(Access *this, IREntryVector *code, IROperand *input,
                 TempAllocator *tempAllocator) {
  this->vtable->store(this, code, input, tempAllocator);
}
IROperand *accessAddrof(struct Access *this, IREntryVector *code,
                        TempAllocator *tempAllocator) {
  return this->vtable->addrof(this, code, tempAllocator);
}
char *accessGetLabel(Access *this) { return this->vtable->getLabel(this); }

void frameDtor(Frame *this) { this->vtable->dtor(this); }
Access *frameAllocArg(Frame *this, Type const *type, bool escapes,
                      TempAllocator *tempAllocator) {
  this->vtable->allocArg(this, type, escapes, tempAllocator);
}
Access *frameAllocLocal(Frame *this, Type const *type, bool escapes,
                        TempAllocator *tempAllocator) {
  return this->vtable->allocLocal(this, type, escapes, tempAllocator);
}
Access *frameAllocRetVal(Frame *this, Type const *type,
                         TempAllocator *tempAllocator) {
  return this->vtable->allocRetVal(this, type, tempAllocator);
}
void frameScopeStart(Frame *this) { this->vtable->scopeStart(this); }
IREntryVector *frameScopeEnd(Frame *this, IREntryVector *out,
                             TempAllocator *tempAllocator) {
  return this->vtable->scopeEnd(this, out, tempAllocator);
}
IROperand *frameIndirectCall(Frame *this, IROperand *who,
                             IROperandVector *actualArgs,
                             Type const *functionType, IREntryVector *out,
                             TempAllocator *tempAllocator) {
  return this->vtable->indirectCall(this, who, actualArgs, functionType, out,
                                    tempAllocator);
}
IROperand *frameDirectCall(Frame *this, char *who, IROperandVector *actualArgs,
                           OverloadSetElement const *function,
                           IREntryVector *out, TempAllocator *tempAllocator) {
  return this->vtable->directCall(this, who, actualArgs, function, out,
                                  tempAllocator);
}

void labelGeneratorDtor(LabelGenerator *this) { this->vtable->dtor(this); }
char *labelGeneratorGenerateCodeLabel(LabelGenerator *this) {
  return this->vtable->generateCodeLabel(this);
}
char *labelGeneratorGenerateDataLabel(LabelGenerator *this) {
  return this->vtable->generateDataLabel(this);
}