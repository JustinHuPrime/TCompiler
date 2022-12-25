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

/**
 * @file
 * arch-specific IR validation
 */

#include "arch/x86_64-linux/irValidation.h"

#include "fileList.h"
#include "ir/ir.h"

int x86_64LinuxValidateIRArchSpecific(char const *phase, bool blocked) {
  bool errored = false;
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
      IRFrag const *frag = file->irFrags.elements[fragIdx];
      if (frag->type == FT_TEXT) {
        LinkedList const *blocks = &frag->data.text.blocks;
        for (ListNode *currBlock = blocks->head->next;
             currBlock != blocks->tail; currBlock = currBlock->next) {
          IRBlock *block = currBlock->data;
          for (ListNode *currInst = block->instructions.head->next;
               currInst != block->instructions.tail;
               currInst = currInst->next) {
            IRInstruction const *i = currInst->data;
            for (size_t argIdx = 0; argIdx < irOperatorArity(i->op); ++argIdx) {
              IROperand const *arg = i->args[argIdx];
              switch (arg->kind) {
                case OK_REG: {
                  if (arg->data.reg.size != 1 && arg->data.reg.size != 2 &&
                      arg->data.reg.size != 4 && arg->data.reg.size != 8) {
                    fprintf(
                        stderr,
                        "%s: internal compiler error: x86_64-linux specific IR "
                        "validation after %s failed - invalid register size "
                        "(%zu) encountered\n",
                        file->inputFilename, phase, arg->data.reg.size);
                    file->errored = true;
                  }
                  break;
                }
                case OK_TEMP: {
                  switch (arg->data.temp.kind) {
                    case AH_GP: {
                      if (arg->data.temp.size != 1 &&
                          arg->data.temp.size != 2 &&
                          arg->data.temp.size != 4 &&
                          arg->data.temp.size != 8) {
                        fprintf(
                            stderr,
                            "%s: internal compiler error: x86_64-linux "
                            "specific IR validation after %s failed - invalid "
                            "temp size for GP temp (%zu) encountered\n",
                            file->inputFilename, phase, arg->data.temp.size);
                        file->errored = true;
                      }

                      if (arg->data.temp.alignment != arg->data.temp.size) {
                        fprintf(
                            stderr,
                            "%s: internal compiler error: x86_64-linux "
                            "specific IR validation after %s failed - invalid "
                            "temp alignment for GP temp (%zu) encountered\n",
                            file->inputFilename, phase,
                            arg->data.temp.alignment);
                        file->errored = true;
                      }
                      break;
                    }
                    case AH_FP: {
                      if (arg->data.temp.size != 4 &&
                          arg->data.temp.size != 8) {
                        fprintf(
                            stderr,
                            "%s: internal compiler error: x86_64-linux "
                            "specific IR validation after %s failed - invalid "
                            "temp size for FP temp (%zu) encountered\n",
                            file->inputFilename, phase, arg->data.temp.size);
                        file->errored = true;
                      }

                      if (arg->data.temp.alignment != arg->data.temp.size) {
                        fprintf(
                            stderr,
                            "%s: internal compiler error: x86_64-linux "
                            "specific IR validation after %s failed - invalid "
                            "temp alignment for FP temp (%zu) encountered",
                            file->inputFilename, phase,
                            arg->data.temp.alignment);
                        file->errored = true;
                      }
                      break;
                    }
                    case AH_MEM: {
                      if (arg->data.temp.alignment > 16) {
                        fprintf(
                            stderr,
                            "%s: internal compiler error: x86_64-linux "
                            "specific IR validation after %s failed - invalid "
                            "mem temp alignment (%zu) encountered\n",
                            file->inputFilename, phase,
                            arg->data.temp.alignment);
                        file->errored = true;
                      }
                      break;
                    }
                  }
                  break;
                }
                default: {
                  break;  // nothing to check
                }
              }
            }
          }
        }
      }
    }

    errored = errored || file->errored;
  }

  if (errored) return -1;

  return 0;
}