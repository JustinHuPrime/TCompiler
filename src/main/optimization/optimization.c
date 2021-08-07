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
    case IO_JL:
    case IO_JLE:
    case IO_JE:
    case IO_JNE:
    case IO_JG:
    case IO_JGE:
    case IO_JA:
    case IO_JAE:
    case IO_JB:
    case IO_JBE:
    case IO_JFL:
    case IO_JFLE:
    case IO_JFE:
    case IO_JFNE:
    case IO_JFG:
    case IO_JFGE:
    case IO_JZ:
    case IO_JNZ: {
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

void optimize(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
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
        // TODO: dead temp elimination
      }
    }
  }
}