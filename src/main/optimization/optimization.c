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

#include "optimization/optimization.h"

#include "fileList.h"
#include "ir/ir.h"
#include "util/internalError.h"

/**
 * short-circuit unconditional-jump-to-any-jump
 *
 * note: for the purposes of this optimization, RETURN is also a jump
 *
 * 1: {
 *   ...
 *   JUMP(_A_)
 * }
 *
 * 2: {
 *   JUMP(_B_) | BJUMP(_B_) | CJUMP(_B_)
 * }
 *
 * ==>
 *
 * 1: {
 *   ...
 *   JUMP(_B_) | BJUMP(_B_) | CJUMP(_B_)
 * }
 *
 * 2: {
 *   JUMP(_B_) | BJUMP(_B_) | CJUMP(_B_)
 * }
 *
 * @param blocks blocks to apply optimization to (mutated)
 */
static void shortCircuitJumps(LinkedList *blocks) {
  /**
   * mapping between block index and its single jump instruction
   */
  IRInstruction **shortCircuits =
      malloc(sizeof(IRInstruction *) * linkedListLength(blocks));

  size_t idx = 0;
  for (ListNode *curr = blocks->head->next; curr != blocks->tail;
       curr = curr->next) {
    // for each block, if it only contains a jump, note that down in
    // shortCircuits
    // NOTE: blocks must contain at least one instruction, and that one
    // instruction must be some sort of jump
    IRBlock *b = curr->data;
    if (b->instructions.head->next->next == b->instructions.tail)
      shortCircuits[idx] = b->instructions.head->next->data;
    else
      shortCircuits[idx] = NULL;
    ++idx;
  }

  // iterate until no more changes happen
  bool changed = true;
  while (changed) {
    changed = false;
    for (ListNode *curr = blocks->head->next; curr != blocks->tail;
         curr = curr->next) {
      // for each block, if it's last jump is an unconditional jump
      IRBlock *b = curr->data;
      IRInstruction *last = b->instructions.tail->prev->data;
      if (last->op == IO_JUMP) {
        IROperand *targetArg = last->args[0];
        if (irOperandIsLocal(targetArg)) {
          size_t target = indexOfBlock(blocks, localOperandName(targetArg));
          if (shortCircuits[target] != NULL) {
            irInstructionFree(last);
            b->instructions.tail->prev->data =
                irInstructionCopy(shortCircuits[target]);
            changed = true;
          }
        }
      }
    }
  }

  free(shortCircuits);
}

/**
 * mark this block and everything reachable from here as reachable
 */
static void markReachable(IRBlock *b, bool *seen, LinkedList *blocks,
                          Vector *frags) {
  if (seen[indexOfBlock(blocks, b->label)] == true)
    return;  // we've been here before - break cycle

  seen[indexOfBlock(blocks, b->label)] = true;
  IRInstruction *last = b->instructions.tail->prev->data;
  switch (last->op) {
    case IO_JUMP: {
      markReachable(findBlock(blocks, localOperandName(last->args[0])), seen,
                    blocks, frags);
      break;
    }
    case IO_JUMPTABLE: {
      IRFrag *table = findFrag(frags, localOperandName(last->args[1]));
      for (size_t idx = 0; idx < table->data.data.data.size; ++idx) {
        IRDatum *datum = table->data.data.data.elements[idx];
        markReachable(findBlock(blocks, datum->data.localLabel), seen, blocks,
                      frags);
      }
      break;
    }
    case IO_J2L:
    case IO_J2LE:
    case IO_J2E:
    case IO_J2NE:
    case IO_J2G:
    case IO_J2GE:
    case IO_J2A:
    case IO_J2AE:
    case IO_J2B:
    case IO_J2BE:
    case IO_J2FL:
    case IO_J2FLE:
    case IO_J2FE:
    case IO_J2FNE:
    case IO_J2FG:
    case IO_J2FGE:
    case IO_J2Z:
    case IO_J2NZ:
    case IO_J2FZ:
    case IO_J2FNZ: {
      markReachable(findBlock(blocks, localOperandName(last->args[0])), seen,
                    blocks, frags);
      markReachable(findBlock(blocks, localOperandName(last->args[1])), seen,
                    blocks, frags);
      break;
    }
    default: {
      // leaves the function - we're done here
      break;
    }
  }
}
/**
 * dead block elimination
 */
static void deadBlockElimination(LinkedList *blocks, Vector *frags) {
  // mark all of the blocks we jump to as seen
  bool *seen = calloc(linkedListLength(blocks), sizeof(bool));
  markReachable(blocks->head->next->data, seen, blocks, frags);

  // deal with jump tables
  // TODO: refactor out of per-frag loop
  for (size_t fragIdx = 0; fragIdx < frags->size; ++fragIdx) {
    IRFrag *f = frags->elements[fragIdx];
    switch (f->type) {
      case FT_RODATA: {
        Vector *data = &f->data.data.data;
        for (size_t datumIdx = 0; datumIdx < data->size; ++datumIdx) {
          IRDatum *datum = data->elements[datumIdx];
          if (datum->type == DT_LOCAL) {
            IRBlock *found = findBlock(blocks, datum->data.localLabel);
            if (found != NULL) markReachable(found, seen, blocks, frags);
          }
        }
      }
      default: {
        break;
      }
    }
  }

  size_t idx = 0;
  for (ListNode *curr = blocks->head->next; curr != blocks->tail;) {
    if (!seen[idx]) {
      ListNode *toRemove = curr;
      curr = curr->next;
      irBlockFree(removeNode(toRemove));
    } else {
      curr = curr->next;
    }
    ++idx;
  }

  free(seen);
}

static void markTempUse(bool *seen, IROperand *arg) {
  if (arg->kind == OK_TEMP) seen[arg->data.temp.name] = true;
}
static bool writesDeadTemp(bool *seen, IROperand *target) {
  return target->kind == OK_TEMP && !seen[target->data.temp.name];
}
/**
 * dead temp elimination
 */
static void deadTempElimination(LinkedList *blocks, size_t maxTemps) {
  bool changed = true;
  while (changed) {
    changed = false;
    bool *seen = calloc(maxTemps, sizeof(bool));

    for (ListNode *currBlock = blocks->head->next; currBlock != blocks->tail;
         currBlock = currBlock->next) {
      IRBlock *block = currBlock->data;
      for (ListNode *currInst = block->instructions.head->next;
           currInst != block->instructions.tail; currInst = currInst->next) {
        IRInstruction *i = currInst->data;
        switch (i->op) {
          case IO_VOLATILE: {
            markTempUse(seen, i->args[0]);
            break;
          }
          case IO_ADDROF: {
            // this means the value in the temp *might* be visible elsewhere
            markTempUse(seen, i->args[0]);
            break;
          }
          case IO_MOVE: {
            markTempUse(seen, i->args[1]);
            break;
          }
          case IO_MEM_STORE: {
            markTempUse(seen, i->args[0]);
            markTempUse(seen, i->args[1]);
            markTempUse(seen, i->args[2]);
            break;
          }
          case IO_MEM_LOAD: {
            markTempUse(seen, i->args[1]);
            markTempUse(seen, i->args[2]);
            break;
          }
          case IO_STK_STORE: {
            markTempUse(seen, i->args[0]);
            markTempUse(seen, i->args[1]);
            break;
          }
          case IO_STK_LOAD: {
            markTempUse(seen, i->args[1]);
            break;
          }
          case IO_OFFSET_STORE:
          case IO_OFFSET_LOAD: {
            markTempUse(seen, i->args[1]);
            markTempUse(seen, i->args[2]);
            break;
          }
          case IO_ADD:
          case IO_SUB:
          case IO_SMUL:
          case IO_UMUL:
          case IO_SDIV:
          case IO_UDIV:
          case IO_SMOD:
          case IO_UMOD:
          case IO_FADD:
          case IO_FSUB:
          case IO_FMUL:
          case IO_FDIV:
          case IO_FMOD:
          case IO_SLL:
          case IO_SLR:
          case IO_SAR:
          case IO_AND:
          case IO_XOR:
          case IO_OR:
          case IO_L:
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
          case IO_FGE: {
            markTempUse(seen, i->args[1]);
            markTempUse(seen, i->args[2]);
            break;
          }
          case IO_NEG:
          case IO_FNEG:
          case IO_NOT:
          case IO_Z:
          case IO_NZ:
          case IO_FZ:
          case IO_FNZ:
          case IO_LNOT:
          case IO_SX:
          case IO_ZX:
          case IO_TRUNC:
          case IO_U2F:
          case IO_S2F:
          case IO_FRESIZE:
          case IO_F2I: {
            markTempUse(seen, i->args[1]);
            break;
          }
          case IO_JUMP: {
            markTempUse(seen, i->args[0]);
            break;
          }
          case IO_J2L:
          case IO_J2LE:
          case IO_J2E:
          case IO_J2NE:
          case IO_J2G:
          case IO_J2GE:
          case IO_J2A:
          case IO_J2AE:
          case IO_J2B:
          case IO_J2BE:
          case IO_J2FL:
          case IO_J2FLE:
          case IO_J2FE:
          case IO_J2FNE:
          case IO_J2FG:
          case IO_J2FGE: {
            markTempUse(seen, i->args[2]);
            markTempUse(seen, i->args[3]);
            break;
          }
          case IO_J2Z:
          case IO_J2NZ:
          case IO_J2FZ:
          case IO_J2FNZ: {
            markTempUse(seen, i->args[1]);
            markTempUse(seen, i->args[2]);
            break;
          }
          case IO_CALL: {
            markTempUse(seen, i->args[0]);
            break;
          }
          default: {
            // no temp read
            break;
          }
        }
      }
    }

    for (ListNode *currBlock = blocks->head->next; currBlock != blocks->tail;
         currBlock = currBlock->next) {
      IRBlock *block = currBlock->data;
      for (ListNode *currInst = block->instructions.head->next;
           currInst != block->instructions.tail; currInst = currInst->next) {
        IRInstruction *i = currInst->data;
        switch (i->op) {
          case IO_UNINITIALIZED:
          case IO_ADDROF:
          case IO_MOVE:
          case IO_MEM_LOAD:
          case IO_STK_LOAD:
          case IO_OFFSET_STORE:
          case IO_OFFSET_LOAD:
          case IO_ADD:
          case IO_SUB:
          case IO_SMUL:
          case IO_UMUL:
          case IO_SDIV:
          case IO_UDIV:
          case IO_SMOD:
          case IO_UMOD:
          case IO_FADD:
          case IO_FSUB:
          case IO_FMUL:
          case IO_FDIV:
          case IO_FMOD:
          case IO_NEG:
          case IO_FNEG:
          case IO_SLL:
          case IO_SLR:
          case IO_SAR:
          case IO_AND:
          case IO_XOR:
          case IO_OR:
          case IO_NOT:
          case IO_L:
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
          case IO_FZ:
          case IO_FNZ:
          case IO_LNOT:
          case IO_SX:
          case IO_ZX:
          case IO_TRUNC:
          case IO_U2F:
          case IO_S2F:
          case IO_FRESIZE:
          case IO_F2I: {
            if (writesDeadTemp(seen, i->args[0])) {
              irInstructionMakeNop(i);
              changed = true;
            }
            break;
          }
          default: {
            // doesn't involve a write
            break;
          }
        }
      }
    }

    free(seen);
  }
}

void optimizeBlockedIr(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    Vector *irFrags = &fileList.entries[fileIdx].irFrags;
    for (size_t fragIdx = 0; fragIdx < irFrags->size; ++fragIdx) {
      IRFrag *frag = irFrags->elements[fragIdx];
      if (frag->type == FT_TEXT) {
        LinkedList *blocks = &frag->data.text.blocks;
        // TODO: (difficult) inlining
        // TODO: (difficult) constant propogation
        // (if only ever used in context where a constant can be used, may
        // replace temp with constant)
        // TODO: (difficult) loop-invariant hoisting
        // (if some expression doesn't change across loop iterations, compute it
        // outside of the loop)
        // TODO: (difficult) loop induction variables
        // (only keep one iteration count for the loop, or reduce for loops to
        // start and end pointer loops)
        // TODO: (difficult) common subexpression elimination
        // (if two expressions are the same, only compute them once)
        // TODO: (difficult) copy propagation
        // (if tempB is moved to tempA and tempB isn't changed afterwards,
        // replace all instances of tempA afterwards with tempB)
        // TODO: (difficult) tail call optimization
        shortCircuitJumps(blocks);
        deadBlockElimination(blocks, irFrags);
        // TODO: dead label elimination
        deadTempElimination(blocks, file->nextId);
      }
    }
  }
}

static void deadLabelElimination(LinkedList *instructions, Vector *frags,
                                 size_t maxLabels) {
  // mark all of the blocks we jump to as seen
  bool *seen = calloc(maxLabels, sizeof(bool));

  // deal with jump tables
  // TODO: refactor out of per-frag loop
  for (size_t fragIdx = 0; fragIdx < frags->size; ++fragIdx) {
    IRFrag *f = frags->elements[fragIdx];
    switch (f->type) {
      case FT_RODATA: {
        Vector *data = &f->data.data.data;
        for (size_t datumIdx = 0; datumIdx < data->size; ++datumIdx) {
          IRDatum *datum = data->elements[datumIdx];
          if (datum->type == DT_LOCAL) seen[datum->data.localLabel] = true;
        }
      }
      default: {
        break;
      }
    }
  }

  for (ListNode *curr = instructions->head->next; curr != instructions->tail;
       curr = curr->next) {
    IRInstruction *i = curr->data;
    switch (i->op) {
      case IO_JUMP: {
        seen[localOperandName(i->args[0])] = true;
        break;
      }
      case IO_JUMPTABLE: {
        IRFrag *table = findFrag(frags, localOperandName(i->args[1]));
        for (size_t idx = 0; idx < table->data.data.data.size; ++idx) {
          IRDatum *datum = table->data.data.data.elements[idx];
          seen[datum->data.localLabel] = true;
        }
        break;
      }
      case IO_J1L:
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
      case IO_J1FZ:
      case IO_J1FNZ: {
        seen[localOperandName(i->args[0])] = true;
        break;
      }
      default: {
        // not a jump
        break;
      }
    }
  }

  for (ListNode *curr = instructions->head->next; curr != instructions->tail;) {
    IRInstruction *i = curr->data;
    if (i->op == IO_LABEL && !seen[localOperandName(i->args[0])]) {
      ListNode *toRemove = curr;
      curr = curr->next;
      irInstructionFree(removeNode(toRemove));
    } else {
      curr = curr->next;
    }
  }

  free(seen);
}

void optimizeScheduledIr(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    FileListEntry *file = &fileList.entries[fileIdx];
    Vector *irFrags = &fileList.entries[fileIdx].irFrags;
    for (size_t fragIdx = 0; fragIdx < irFrags->size; ++fragIdx) {
      IRFrag *frag = irFrags->elements[fragIdx];
      if (frag->type == FT_TEXT) {
        IRBlock *block = frag->data.text.blocks.head->next->data;
        deadLabelElimination(&block->instructions, irFrags, file->nextId);
      }
    }
  }
}