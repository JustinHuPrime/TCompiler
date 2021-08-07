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
static size_t indexOfBlock(Vector *blocks, size_t label) {
  for (size_t idx = 0; idx < blocks->size; ++idx) {
    IRBlock *b = blocks->elements[idx];
    if (b->label == label) return idx;
  }
  error(__FILE__, __LINE__, "given block index doesn't exist");
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
static void shortCircuitJumps(Vector *blocks) {
  /**
   * mapping between block index and its single jump instruction
   */
  IRInstruction **shortCircuits =
      malloc(sizeof(IRInstruction *) * blocks->size);

  for (size_t idx = 0; idx < blocks->size; ++idx) {
    // for each block, if it only contains a jump, note that down in
    // shortCircuits
    // NOTE: blocks must contain at least one instruction, and that one
    // instruction must be some sort of jump
    IRBlock *b = blocks->elements[idx];
    if (b->instructions.head->next->next == b->instructions.tail)
      shortCircuits[idx] = b->instructions.head->next->data;
    else
      shortCircuits[idx] = NULL;
  }

  // iterate until no more changes happen
  bool changed = true;
  while (changed) {
    changed = false;
    for (size_t idx = 0; idx < blocks->size; ++idx) {
      // for each block, if it's last jump is an unconditional jump
      IRBlock *b = blocks->elements[idx];
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

void optimize(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    Vector *irFrags = &fileList.entries[fileIdx].irFrags;
    for (size_t fragIdx = 0; fragIdx < irFrags->size; ++fragIdx) {
      IRFrag *frag = irFrags->elements[fragIdx];
      if (frag->type == FT_TEXT) {
        Vector *blocks = &frag->data.text.blocks;
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
        // TODO: dead block elimination
        // TODO: dead temp elimination
      }
    }
  }
}