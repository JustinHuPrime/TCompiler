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
 * find the index of a block given its label
 */
static size_t indexOfBlock(LinkedList *blocks, size_t label) {
  size_t idx = 0;
  for (ListNode *curr = blocks->head->next; curr != blocks->tail;
       curr = curr->next) {
    IRBlock *b = curr->data;
    if (b->label == label) return idx;
    ++idx;
  }
  error(__FILE__, __LINE__, "given block label doesn't exist");
}
/**
 * get a block given its label
 */
static IRBlock *findBlock(LinkedList *blocks, size_t label) {
  for (ListNode *curr = blocks->head->next; curr != blocks->tail;
       curr = curr->next) {
    IRBlock *b = curr->data;
    if (b->label == label) return b;
  }
  error(__FILE__, __LINE__, "given block label doesn't exist");
}

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
        if (targetArg->kind == OK_LOCAL) {
          size_t target = indexOfBlock(blocks, targetArg->data.local.name);
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
static void markReachable(IRBlock *b, bool *seen, LinkedList *blocks) {
  if (seen[indexOfBlock(blocks, b->label)] == true)
    return;  // we've been here before - break cycle

  seen[indexOfBlock(blocks, b->label)] = true;
  IRInstruction *last = b->instructions.tail->prev->data;
  switch (last->op) {
    case IO_JUMP: {
      if (last->args[0]->kind == OK_LOCAL)
        markReachable(findBlock(blocks, last->args[0]->data.local.name), seen,
                      blocks);
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
    case IO_J2NZ: {
      markReachable(findBlock(blocks, last->args[0]->data.local.name), seen,
                    blocks);
      markReachable(findBlock(blocks, last->args[1]->data.local.name), seen,
                    blocks);
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
static void deadBlockElimination(LinkedList *blocks) {
  bool *seen = calloc(linkedListLength(blocks), sizeof(bool));
  markReachable(blocks->head->next->data, seen, blocks);

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
          case IO_J2NZ: {
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

/**
 * nop elimination
 */
static void nopElimination(LinkedList *blocks) {
  for (ListNode *currBlock = blocks->head->next; currBlock != blocks->tail;
       currBlock = currBlock->next) {
    IRBlock *block = currBlock->data;
    for (ListNode *currInst = block->instructions.head->next;
         currInst != block->instructions.tail;) {
      IRInstruction *i = currInst->data;
      if (i->op == IO_NOP) {
        ListNode *toRemove = currInst;
        currInst = currInst->next;
        irInstructionFree(removeNode(toRemove));
      } else {
        currInst = currInst->next;
      }
    }
  }
}

void optimize(void) {
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
        deadBlockElimination(blocks);
        deadTempElimination(blocks, file->nextId);
        nopElimination(blocks);
      }
    }
  }
}