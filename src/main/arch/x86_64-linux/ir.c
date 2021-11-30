// Copyright 2021 Justin Hu
//
// This file is part of the T Language Compiler.
//
// The T Language Compiler is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// The T Language Compiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// the T Language Compiler. If not see <https://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "arch/x86_64-linux/ir.h"

#include "fileList.h"
#include "ir/ir.h"
#include "ir/shorthand.h"
#include "util/internalError.h"

void x86_64LinuxReduceOpArity(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
      IRFrag *frag = file->irFrags.elements[fragIdx];
      if (frag->type == FT_TEXT) {
        IRBlock *b = frag->data.text.blocks.head->next->data;
        for (ListNode *currInst = b->instructions.head->next;
             currInst != b->instructions.tail; currInst = currInst->next) {
          IRInstruction *ir = currInst->data;
          switch (ir->op) {
            case IO_LABEL:  // obviously no r/w registers
            case IO_VOLATILE:
            case IO_UNINITIALIZED:
            case IO_NOP:
            case IO_ADDROF:
            case IO_MOVE:
            case IO_MEM_STORE:
            case IO_MEM_LOAD:
            case IO_STK_STORE:
            case IO_STK_LOAD:
            case IO_OFFSET_STORE:
            case IO_OFFSET_LOAD:
            case IO_FMOD:  // fmod uses the x87 FPU, so it has separate
                           // source and first-arg registers
            case IO_L:     // comparisons and jumps do a cmp and then use the
                           // results of the cmp
            case IO_LE:
            case IO_E:
            case IO_NE:
            case IO_G:
            case IO_GE:
            case IO_A:
            case IO_AE:
            case IO_B:
            case IO_BE:
            case IO_FL:
            case IO_FLE:
            case IO_FE:
            case IO_FNE:
            case IO_FG:
            case IO_FGE:
            case IO_Z:
            case IO_NZ:
            case IO_SX:  // casts are just fancy moves
            case IO_ZX:
            case IO_TRUNC:
            case IO_U2F:
            case IO_S2F:
            case IO_FRESIZE:
            case IO_F2I:
            case IO_JUMP:  // also obviously no r/w registers
            case IO_JUMPTABLE:
            case IO_J1L:  // more jumps
            case IO_J1LE:
            case IO_J1E:
            case IO_J1NE:
            case IO_J1G:
            case IO_J1GE:
            case IO_J1A:
            case IO_J1AE:
            case IO_J1B:
            case IO_J1BE:
            case IO_J1FL:
            case IO_J1FLE:
            case IO_J1FE:
            case IO_J1FNE:
            case IO_J1FG:
            case IO_J1FGE:
            case IO_J1Z:
            case IO_J1NZ:
            case IO_CALL:  // also obviously no r/w registers
            case IO_RETURN: {
              break;  // nothing to do
            }
            case IO_ADD:
            case IO_SMUL:
            case IO_UMUL:
            case IO_FADD:
            case IO_FMUL:
            case IO_AND:
            case IO_XOR:
            case IO_OR: {
              // commutative 3-arg -> 2-arg
              // 0 = 1 op 2
              // ->
              // 0 = 1
              // 0 op= 2
              // OR
              // 0 = 2
              // 0 op= 1

              if (irOperandEqual(ir->args[0], ir->args[1])) {
                // 0 op= 2
                irOperandFree(ir->args[1]);
                ir->args[1] = ir->args[2];
                ir->args[2] = NULL;
              } else if (irOperandEqual(ir->args[0], ir->args[2])) {
                // 0 op= 1
                irOperandFree(ir->args[2]);
                ir->args[2] = NULL;
              } else {
                // 0 = 1
                // 0 op= 2
                insertNodeBefore(currInst,
                                 MOVE(irOperandCopy(ir->args[0]), ir->args[1]));
                ir->args[1] = ir->args[2];
                ir->args[2] = NULL;
              }
              break;
            }
            case IO_SUB:
            case IO_SDIV:
            case IO_UDIV:
            case IO_SMOD:
            case IO_UMOD:
            case IO_FSUB:
            case IO_FDIV:
            case IO_SLL:
            case IO_SLR:
            case IO_SAR: {
              // non-commutative 3-arg -> 2-arg
              // 0 = 1 op 2
              // ->
              // 0 = 1
              // 0 op= 2

              if (irOperandEqual(ir->args[0], ir->args[1])) {
                // 0 op= 2
                irOperandFree(ir->args[1]);
                ir->args[1] = ir->args[2];
                ir->args[2] = NULL;
              } else {
                // 0 = 1
                // 0 op= 2
                insertNodeBefore(currInst,
                                 MOVE(irOperandCopy(ir->args[0]), ir->args[1]));
                ir->args[1] = ir->args[2];
                ir->args[2] = NULL;
              }
              break;
            }
            case IO_NEG:
            case IO_FNEG:
            case IO_NOT:
            case IO_LNOT: {
              // 2 arg -> 1 arg
              // 0 = op 1
              // ->
              // 0 = 1
              // 0 =op
              // OR
              // 0 =op

              if (irOperandEqual(ir->args[0], ir->args[1])) {
                // 0 =op
                irOperandFree(ir->args[1]);
                ir->args[1] = NULL;
              } else {
                // 0 = 1
                // 0 =op
                insertNodeBefore(currInst,
                                 MOVE(irOperandCopy(ir->args[0]), ir->args[1]));
                ir->args[1] = NULL;
              }
              break;
            }
            default: {
              error(__FILE__, __LINE__, "invalid IR opcode");
            }
          }
        }
      }
    }
  }
}

void x86_64LinuxSatisfyAddressing(void) {}