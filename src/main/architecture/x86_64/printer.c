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

// implementation of x86_64 assembly printer

#include "architecture/x86_64/printer.h"

#include "architecture/x86_64/assembly.h"
#include "util/internalError.h"

#include <stdio.h>

static char const *allocHintToTempType(AllocHint h) {
  switch (h) {
    case AH_GP: {
      return "general purpose";
    }
    case AH_SSE: {
      return "SSE";
    }
    case AH_MEM: {
      return "in memory";
    }
    default: { error(__FILE__, __LINE__, "invalid AllocHint enum"); }
  }
}
static char const *regNumToString(X86_64Register r) {
  switch (r) {
    case X86_64_RAX: {
      return "%rax";
    }
    case X86_64_RBX: {
      return "%rbx";
    }
    case X86_64_RCX: {
      return "%rcx";
    }
    case X86_64_RDX: {
      return "%rdx";
    }
    case X86_64_RSI: {
      return "%rsi";
    }
    case X86_64_RDI: {
      return "%rdi";
    }
    case X86_64_RSP: {
      return "%rsp";
    }
    case X86_64_RBP: {
      return "%rbp";
    }
    case X86_64_R8: {
      return "%r8";
    }
    case X86_64_R9: {
      return "%r9";
    }
    case X86_64_R10: {
      return "%r10";
    }
    case X86_64_R11: {
      return "%r11";
    }
    case X86_64_R12: {
      return "%r12";
    }
    case X86_64_R13: {
      return "%r13";
    }
    case X86_64_R14: {
      return "%r14";
    }
    case X86_64_R15: {
      return "%r15";
    }
    case X86_64_XMM0: {
      return "%xmm0";
    }
    case X86_64_XMM1: {
      return "%xmm1";
    }
    case X86_64_XMM2: {
      return "%xmm2";
    }
    case X86_64_XMM3: {
      return "%xmm3";
    }
    case X86_64_XMM4: {
      return "%xmm4";
    }
    case X86_64_XMM5: {
      return "%xmm5";
    }
    case X86_64_XMM6: {
      return "%xmm6";
    }
    case X86_64_XMM7: {
      return "%xmm7";
    }
    case X86_64_XMM8: {
      return "%xmm8";
    }
    case X86_64_XMM9: {
      return "%xmm9";
    }
    case X86_64_XMM10: {
      return "%xmm10";
    }
    case X86_64_XMM11: {
      return "%xmm11";
    }
    case X86_64_XMM12: {
      return "%xmm12";
    }
    case X86_64_XMM13: {
      return "%xmm13";
    }
    case X86_64_XMM14: {
      return "%xmm14";
    }
    case X86_64_XMM15: {
      return "%xmm15";
    }
    default: { error(__FILE__, __LINE__, "invalid X86_64Register enum"); }
  }
}
static void dumpOperand(X86_64Operand *o) {
  switch (o->kind) {
    case X86_64_OK_TEMP: {
      printf("TEMP(#%zu, %zu wide, %zu aligned, %s)", o->data.temp.n,
             o->data.temp.size, o->data.temp.alignment,
             allocHintToTempType(o->data.temp.kind));
      break;
    }
    case X86_64_OK_REG: {
      printf("REGISTER(%s, %zu wide)", regNumToString(o->data.reg.reg),
             o->operandSize);
      break;
    }
    case X86_64_OK_STACKOFFSET: {
      printf("OFFSET(%ld, %zu wide)", o->data.stackOffset.offset,
             o->operandSize);
      break;
    }
    case X86_64_OK_STACK: {
      printf("STACK(%ld, %zu wide)", o->data.stack.offset, o->operandSize);
      break;
    }
    default: { error(__FILE__, __LINE__, "invalid X86_64OperandKind enum"); }
  }
}
static void dumpInstruction(X86_64Instruction *i) {
  printf("%s", i->skeleton);
  if (i->uses.size != 0) {
    printf("\t# uses: ");
    for (size_t idx = 0; idx < i->uses.size; idx++) {
      dumpOperand(i->uses.elements[idx]);
      if (idx != i->uses.size - 1) {
        printf(", ");
      }
    }
    printf("\n");
  }
  if (i->defines.size != 0) {
    printf("\t# defines: ");
    for (size_t idx = 0; idx < i->defines.size; idx++) {
      dumpOperand(i->defines.elements[idx]);
      if (idx != i->defines.size - 1) {
        printf(", ");
      }
    }
    printf("\n");
  }
  if (i->other.size != 0) {
    printf("\t# other: ");
    for (size_t idx = 0; idx < i->other.size; idx++) {
      dumpOperand(i->other.elements[idx]);
      if (idx != i->other.size - 1) {
        printf(", ");
      }
    }
    printf("\n");
  }
}
static void dumpFragment(X86_64Fragment *f) {
  switch (f->kind) {
    case X86_64_FK_DATA: {
      printf("%s", f->data.data.data);
      break;
    }
    case X86_64_FK_TEXT: {
      printf("%s", f->data.text.header);
      for (size_t idx = 0; idx < f->data.text.body->size; idx++) {
        dumpInstruction(f->data.text.body->elements[idx]);
      }
      printf("%s", f->data.text.footer);
      break;
    }
    default: { error(__FILE__, __LINE__, "invalid X86_64FragmentKind enum"); }
  }
}
void dumpX86_64File(X86_64File *f) {
  printf("%s", f->header);
  for (size_t idx = 0; idx < f->fragments.size; idx++) {
    dumpFragment(f->fragments.elements[idx]);
  }
  printf("%s", f->footer);
}