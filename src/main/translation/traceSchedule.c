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

#include "translation/traceSchedule.h"

#include "fileList.h"
#include "ir/ir.h"
#include "ir/shorthand.h"
#include "util/internalError.h"

static void copyOverLastInstruction(IRBlock *b, IRBlock *out) {
  // TODO: can make this more efficient by copying over the listnode
  // directly
  IR(out, removeNode(b->instructions.tail->prev));
  // ListNode *lastNode = b->instructions.tail->prev;

  // lastNode->prev->next = lastNode->next;
  // lastNode->next->prev = lastNode->prev;

  // lastNode->prev = out->instructions.tail->prev;
  // lastNode->prev->next = lastNode;
  // lastNode->next = out->instructions.tail;
  // lastNode->next->prev = lastNode;
}
static IRInstruction *oneArgCJumpCreate(IROperator op,
                                        IROperand const *trueLabel,
                                        IROperand const *lhs,
                                        IROperand const *rhs) {
  IRInstruction *retval = irInstructionCreate(op);
  retval->args[0] = irOperandCopy(trueLabel);
  retval->args[1] = irOperandCopy(lhs);
  retval->args[2] = irOperandCopy(rhs);
  return retval;
}
static IRInstruction *oneArgBJumpCreate(IROperator op,
                                        IROperand const *trueLabel,
                                        IROperand const *scrutinee) {
  IRInstruction *retval = irInstructionCreate(op);
  retval->args[0] = irOperandCopy(trueLabel);
  retval->args[1] = irOperandCopy(scrutinee);
  return retval;
}
static IRInstruction *oneArgJumpFromTwoArgJump(IRInstruction *i) {
  switch (i->op) {
    case IO_J2L: {
      return oneArgCJumpCreate(IO_J1L, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2LE: {
      return oneArgCJumpCreate(IO_J1LE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2E: {
      return oneArgCJumpCreate(IO_J1E, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2NE: {
      return oneArgCJumpCreate(IO_J1NE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2G: {
      return oneArgCJumpCreate(IO_J1G, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2GE: {
      return oneArgCJumpCreate(IO_J1GE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2A: {
      return oneArgCJumpCreate(IO_J1A, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2AE: {
      return oneArgCJumpCreate(IO_J1AE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2B: {
      return oneArgCJumpCreate(IO_J1B, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2BE: {
      return oneArgCJumpCreate(IO_J1BE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2FL: {
      return oneArgCJumpCreate(IO_J1FL, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2FLE: {
      return oneArgCJumpCreate(IO_J1FLE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2FE: {
      return oneArgCJumpCreate(IO_J1FE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2FNE: {
      return oneArgCJumpCreate(IO_J1FNE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2FG: {
      return oneArgCJumpCreate(IO_J1FG, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2FGE: {
      return oneArgCJumpCreate(IO_J1FGE, i->args[0], i->args[2], i->args[3]);
    }
    case IO_J2Z: {
      return oneArgBJumpCreate(IO_J1Z, i->args[0], i->args[2]);
    }
    case IO_J2NZ: {
      return oneArgBJumpCreate(IO_J1NZ, i->args[0], i->args[2]);
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid jump given to traceSchedule despite passing validation");
    }
  }
}
static void scheduleBlock(IRBlock *b, IRBlock *out, LinkedList *blocks) {
  for (ListNode *currBlock = blocks->head->next; currBlock != blocks->tail;
       currBlock = currBlock->next) {
    if (currBlock->data == b) {
      removeNode(currBlock);
      break;
    }
  }
  // add a label
  IR(out, LABEL(b->label));

  // if there's any content except for the last instruction move it into out
  while (b->instructions.head->next != b->instructions.tail->prev)
    IR(out, removeNode(b->instructions.head->next));
  // TODO: can make this more efficent by moving the list nodes directly
  // if (b->instructions.tail->prev->prev != b->instructions.head) {
  //   // note - this is a *move*, not a copy
  //   ListNode *blockStart = b->instructions.head->next;
  //   ListNode *blockEnd = b->instructions.tail->prev->prev;

  //   b->instructions.head->next = b->instructions.tail->prev;
  //   b->instructions.head->next->prev = b->instructions.head->next;

  //   blockStart->prev = out->instructions.tail->prev;
  //   blockStart->prev->next = blockStart;
  //   blockEnd->next = out->instructions.tail;
  //   blockEnd->next->prev = blockEnd;
  // }

  // look at the last instruction
  IRInstruction *last = b->instructions.tail->prev->data;
  switch (last->op) {
    case IO_JUMP: {
      // if it's a jump to a local, schedule that block and skip the jump,
      // otherwise, copy the jump verbatim
      if (last->args[0]->kind == OK_LOCAL) {
        IRBlock *found = findBlock(blocks, last->args[0]->data.local.name);
        if (found != NULL) scheduleBlock(found, out, blocks);
      } else {
        copyOverLastInstruction(b, out);
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
    case IO_J2NZ: {
      // both must be jumps to locals - assume falsehood is more likely
      IR(out, oneArgJumpFromTwoArgJump(last));
      IRBlock *found = findBlock(blocks, last->args[1]->data.local.name);
      if (found != NULL) {
        scheduleBlock(found, out, blocks);
      } else {
        IR(out, JUMP(LOCAL(last->args[1]->data.local.name)));
      }
      found = findBlock(blocks, last->args[0]->data.local.name);
      if (found != NULL) scheduleBlock(found, out, blocks);
      break;
    }
    case IO_RETURN: {
      // append it without modification
      copyOverLastInstruction(b, out);
      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "invalid terminating instruction encountered despite validation "
            "passing");
    }
  }
  irBlockFree(b);
}

void traceSchedule(void) {
  for (size_t fileIdx = 0; fileIdx < fileList.size; ++fileIdx) {
    if (fileList.entries[fileIdx].isCode) {
      FileListEntry *file = &fileList.entries[fileIdx];
      for (size_t fragIdx = 0; fragIdx < file->irFrags.size; ++fragIdx) {
        IRFrag *frag = file->irFrags.elements[fragIdx];
        if (frag->type == FT_TEXT) {
          LinkedList blocks;
          blocks.head = frag->data.text.blocks.head;
          blocks.tail = frag->data.text.blocks.tail;
          linkedListInit(&frag->data.text.blocks);
          IRBlock *out = BLOCK(0, &frag->data.text.blocks);
          scheduleBlock(blocks.head->next->data, out, &blocks);
          linkedListUninit(&blocks, (void (*)(void *))irBlockFree);
        }
      }
    }
  }
}