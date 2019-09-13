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

// Implementation of IR

#include "ir/ir.h"

#include <stdlib.h>

void irOperandDestroy(IROperand *o) {
  switch (o->kind) {
    case OK_TEMP: {
      break;
    }
    case OK_REG: {
      break;
    }
    case OK_CONSTANT: {
      break;
    }
    case OK_LABEL: {
      free(o->operand.label.name);
      break;
    }
    case OK_ASM: {
      free(o->operand.assembly.assembly);
      break;
    }
  }
  free(o);
}

void irEntryDestroy(IREntry *e) {
  if (e->dest != NULL) {
    irOperandDestroy(e->dest);
  }
  if (e->arg1 != NULL) {
    irOperandDestroy(e->arg1);
  }
  if (e->arg2 != NULL) {
    irOperandDestroy(e->arg2);
  }
  free(e);
}

IRVector *irVectorCreate(void) { return vectorCreate(); }
void irVectorInsert(IRVector *v, IREntry *e) { vectorInsert(v, e); }
void irVectorDestroy(IRVector *v) {
  vectorDestroy(v, (void (*)(void *))irEntryDestroy);
}