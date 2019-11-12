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

// implementation of x86_64 register allocation

#include "architecture/x86_64/registerAllocate.h"

#include "architecture/x86_64/assembly.h"
#include "architecture/x86_64/frame.h"
#include "architecture/x86_64/shorthand.h"
#include "util/format.h"
#include "util/internalError.h"

#include <stdlib.h>
#include <string.h>

// list of registers, in order, to use when allocating temps
static X86_64Register const GP_REGISTER_ALLOC_LIST[] = {
    X86_64_RAX, X86_64_R11, X86_64_R10, X86_64_RBX, X86_64_R12,
    X86_64_R13, X86_64_R14, X86_64_R15, X86_64_R9,  X86_64_R8,
    X86_64_RCX, X86_64_RDX, X86_64_RSI, X86_64_RDI,
};
static size_t const GP_REGISTER_ALLOC_LIST_SIZE = 14;
static X86_64Register const SSE_REGISTER_ALLOC_LIST[] = {
    X86_64_XMM0,  X86_64_XMM1,  X86_64_XMM8,  X86_64_XMM9,
    X86_64_XMM10, X86_64_XMM11, X86_64_XMM12, X86_64_XMM13,
    X86_64_XMM14, X86_64_XMM15, X86_64_XMM7,  X86_64_XMM6,
    X86_64_XMM5,  X86_64_XMM4,  X86_64_XMM3,  X86_64_XMM2,
};
static size_t const SSE_REGISTER_ALLOC_LIST_SIZE = 16;

typedef struct {
  X86_64Instruction const *thisInstr;
  SizeVector nextInstrs;
  X86_64OperandVector liveTemps;
} FlowGraphNode;
static FlowGraphNode *flowGraphNodeCreate(X86_64Instruction const *thisInstr) {
  FlowGraphNode *n = malloc(sizeof(FlowGraphNode));
  n->thisInstr = thisInstr;
  sizeVectorInit(&n->nextInstrs);
  x86_64OperandVectorInit(&n->liveTemps);
  return n;
}
static void flowGraphNodeDestroy(FlowGraphNode *n) {
  sizeVectorUninit(&n->nextInstrs);
  x86_64OperandVectorUninit(&n->liveTemps);
  free(n);
}

typedef Vector FlowGraph;
static void flowGraphInit(FlowGraph *g) { vectorInit(g); }
static void flowGraphInsert(FlowGraph *g, FlowGraphNode *n) {
  vectorInsert(g, n);
}
static void flowGraphUninit(FlowGraph *g) {
  vectorUninit(g, (void (*)(void *))flowGraphNodeDestroy);
}

typedef struct {
  X86_64Operand *operand;
  X86_64Operand *paintAs;
  X86_64OperandVector interferedTemps;  // temp number that is interfered with
  X86_64OperandVector interferedRegs;   // reg number that is interfered with
} TempInterferenceGraphNode;
static TempInterferenceGraphNode *tempInterferenceGraphNodeCreate(
    X86_64Operand *operand) {
  TempInterferenceGraphNode *node = malloc(sizeof(TempInterferenceGraphNode));
  node->operand = operand;
  node->paintAs = NULL;
  x86_64OperandVectorInit(&node->interferedTemps);
  x86_64OperandVectorInit(&node->interferedRegs);
  return node;
}
static bool tempInterferenceGraphNodeInterferesWith(
    TempInterferenceGraphNode *node, X86_64Register reg) {
  for (size_t idx = 0; idx < node->interferedRegs.size; idx++) {
    X86_64Operand *interfering = node->interferedRegs.elements[idx];
    if (interfering->data.reg.reg == reg) {
      return true;
    }
  }
  return false;
}
static void tempInterferenceGraphNodeDestroy(TempInterferenceGraphNode *node) {
  x86_64OperandDestroy(node->operand);
  if (node->paintAs != NULL) {
    x86_64OperandDestroy(node->paintAs);
  }
  x86_64OperandVectorUninit(&node->interferedTemps);
  x86_64OperandVectorUninit(&node->interferedRegs);
  free(node);
}

typedef Vector TempInterferenceGraph;
static void tempInterferenceGraphInit(TempInterferenceGraph *g) {
  vectorInit(g);
}
static TempInterferenceGraphNode *tempInterferenceGraphGetNode(
    TempInterferenceGraph *g, X86_64Operand const *rand) {
  for (size_t idx = 0; idx < g->size; idx++) {
    TempInterferenceGraphNode *node = g->elements[idx];
    if (node->operand->data.temp.n == rand->data.temp.n) {
      return node;
    }
  }
  TempInterferenceGraphNode *node =
      tempInterferenceGraphNodeCreate(x86_64OperandCopy(rand));
  vectorInsert(g, node);
  return node;
}
static bool tempInterferenceGraphContains(TempInterferenceGraph *g, size_t n) {
  for (size_t idx = 0; idx < g->size; idx++) {
    TempInterferenceGraphNode *node = g->elements[idx];
    if (node->operand->data.temp.n == n) {
      return true;
    }
  }
  return false;
}
static void tempInterferenceGraphUninit(TempInterferenceGraph *g) {
  vectorUninit(g, (void (*)(void *))tempInterferenceGraphNodeDestroy);
}

typedef struct {
  size_t size;
  X86_64Register *elements;
} CalleeSaveSet;
static void calleeSaveSetInit(CalleeSaveSet *set) {
  set->size = 0;
  set->elements = malloc(sizeof(X86_64Register *) * X86_64_NUM_CALLEE_SAVE);
}
// adds only if not present, size is never to be more than
// X86_64_NUM_CALLEE_SAVE
static void calleeSaveSetAdd(CalleeSaveSet *set, X86_64Register r) {
  for (size_t idx = 0; idx < set->size; idx++) {
    if (set->elements[idx] == r) {
      return;
    }
  }
  set->elements[set->size++] = r;
}
static void calleeSaveSetUninit(CalleeSaveSet *set) { free(set->elements); }

static size_t getLabelIndex(X86_64InstructionVector const *instrs,
                            char const *labelName) {
  for (size_t idx = 0; idx < instrs->size; idx++) {
    X86_64Instruction const *instr = instrs->elements[idx];
    if (instr->kind == X86_64_IK_LABEL &&
        strcmp(labelName, instr->data.labelName) == 0) {
      return idx;
    }
  }
  error(__FILE__, __LINE__,
        "unexpected jump to label outside of function encountered (should be "
        "leave annotated instruction instead)");
}
static bool hasFutureUse(size_t temp, size_t currInstr, FlowGraph *graph,
                         X86_64InstructionVector const *instrs,
                         SizeVector *visited) {
  if (sizeVectorContains(visited, currInstr)) {
    // already been here, and didn't contain the first time around
    return false;
  } else {
    sizeVectorInsert(visited, currInstr);

    // is it used here? (present use counts as future use)
    X86_64Instruction *instr = instrs->elements[currInstr];
    for (size_t idx = 0; idx < instr->uses.size; idx++) {
      X86_64Operand *rand = instr->uses.elements[idx];
      if (rand->kind == X86_64_OK_TEMP && rand->data.temp.n == temp) {
        return true;
      }
    }

    // if it's used in any future instruction, ever, return true
    FlowGraphNode *thisNode = graph->elements[currInstr];
    for (size_t idx = 0; idx < thisNode->nextInstrs.size; idx++) {
      if (hasFutureUse(temp, thisNode->nextInstrs.elements[idx], graph, instrs,
                       visited)) {
        return true;
      }
    }
    return false;
  }
}
static void markAsLive(X86_64Operand const *temp, size_t currInstr,
                       FlowGraph *graph, X86_64InstructionVector const *instrs,
                       SizeVector *visited) {
  if (sizeVectorContains(visited, currInstr)) {
    // already marked
    return;
  } else {
    sizeVectorInsert(visited, currInstr);

    SizeVector futureUseVisited;
    sizeVectorInit(&futureUseVisited);
    bool usedInFuture = hasFutureUse(temp->data.temp.n, currInstr, graph,
                                     instrs, &futureUseVisited);
    sizeVectorUninit(&futureUseVisited);
    if (usedInFuture) {
      FlowGraphNode *thisNode = graph->elements[currInstr];
      x86_64OperandVectorInsert(&thisNode->liveTemps, x86_64OperandCopy(temp));

      for (size_t idx = 0; idx < thisNode->nextInstrs.size; idx++) {
        markAsLive(temp, thisNode->nextInstrs.elements[idx], graph, instrs,
                   visited);
      }
    }
  }
}
static void replaceTemps(X86_64OperandVector *temps,
                         TempInterferenceGraph *paintedGraph) {
  for (size_t idx = 0; idx < temps->size; idx++) {
    X86_64Operand *oldTemp = temps->elements[idx];
    if (oldTemp->kind == X86_64_OK_TEMP) {
      X86_64Operand const *replaceWith =
          tempInterferenceGraphGetNode(paintedGraph, oldTemp)->paintAs;
      temps->elements[idx] = x86_64OperandCopy(replaceWith);
      x86_64OperandDestroy(oldTemp);
    }
  }
}
static char const *calleeSaveNumberToName(X86_64Register reg) {
  switch (reg) {
    case X86_64_RBX: {
      return "%rbx";
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
    default: {
      error(__FILE__, __LINE__, "invalid callee save register given");
    }
  }
}
static void registerAllocateFragment(X86_64Fragment *frag) {
  X86_64Frame *frame = frag->data.text.frame;
  X86_64InstructionVector *instrs = frag->data.text.body;

  size_t frameSize = frame->frameSize;
  size_t tempAreaSize = 0;

  CalleeSaveSet toSave;
  calleeSaveSetInit(&toSave);

  bool hasSpill = true;
  TempInterferenceGraph interferenceGraph;
  while (hasSpill) {
    hasSpill = false;
    toSave.size = 0;

    // generate control flow graph
    FlowGraph flowGraph;
    flowGraphInit(&flowGraph);
    for (size_t idx = 0; idx < instrs->size; idx++) {
      X86_64Instruction const *instr = instrs->elements[idx];
      FlowGraphNode *node = flowGraphNodeCreate(instr);
      switch (instr->kind) {
        case X86_64_IK_REGULAR:
        case X86_64_IK_MOVE:
        case X86_64_IK_LABEL: {
          if (idx + 1 < instrs->size) {
            // no return at the end yet - inserted after this
            sizeVectorInsert(&node->nextInstrs, idx + 1);
          }
          break;
        }
        case X86_64_IK_JUMP: {
          sizeVectorInsert(&node->nextInstrs,
                           getLabelIndex(instrs, instr->data.jumpTarget));
          break;
        }
        case X86_64_IK_CJUMP: {
          if (idx + 1 < instrs->size) {
            sizeVectorInsert(&node->nextInstrs, idx + 1);
          }
          sizeVectorInsert(&node->nextInstrs,
                           getLabelIndex(instrs, instr->data.jumpTarget));
          break;
        }
        case X86_64_IK_LEAVE: {
          break;
        }
        case X86_64_IK_SWITCH: {
          StringVector const *switchTargets = &instr->data.switchTargets;
          for (size_t targetIdx = 0; targetIdx < switchTargets->size; idx++) {
            sizeVectorInsert(
                &node->nextInstrs,
                getLabelIndex(instrs, switchTargets->elements[idx]));
          }
          break;
        }
      }
      flowGraphInsert(&flowGraph, node);
    }

    // generate temp liveness ranges
    // note - a temp is live from a def to the last use before a def or the end
    // of a flow
    for (size_t idx = 0; idx < flowGraph.size; idx++) {
      // for each instruction
      X86_64Instruction *instr = instrs->elements[idx];
      X86_64OperandVector *rands = &instr->defines;
      for (size_t randIdx = 0; randIdx < rands->size; randIdx++) {
        // for each define
        X86_64Operand *rand = rands->elements[randIdx];
        if (rand->kind == X86_64_OK_TEMP) {
          // if it's a temp - mark it as live from here to the first use, if any
          SizeVector visited;
          sizeVectorInit(&visited);
          markAsLive(rand, idx, &flowGraph, instrs, &visited);
          sizeVectorUninit(&visited);
        }
      }
    }

    // generate temp interference graph - doesn't yet do move elision
    tempInterferenceGraphInit(&interferenceGraph);
    for (size_t idx = 0; idx < flowGraph.size; idx++) {
      // for each instruction
      FlowGraphNode *node = flowGraph.elements[idx];
      X86_64OperandVector *liveTemps = &node->liveTemps;
      for (size_t thisIdx = 0; thisIdx < liveTemps->size; thisIdx++) {
        // for each live temp
        TempInterferenceGraphNode *tempNode = tempInterferenceGraphGetNode(
            &interferenceGraph, liveTemps->elements[thisIdx]);
        for (size_t otherIdx = 0; otherIdx < liveTemps->size; otherIdx++) {
          if (thisIdx != otherIdx) {
            // for each temp it is live with, record that
            x86_64OperandVectorInsert(
                &tempNode->interferedTemps,
                x86_64OperandCopy(liveTemps->elements[otherIdx]));
          }
        }
        X86_64Instruction *instr = instrs->elements[idx];
        X86_64OperandVector *rands = &instr->defines;
        for (size_t otherIdx = 0; otherIdx < rands->size; otherIdx++) {
          // for each reg it is live with, record that
          X86_64Operand *rand = rands->elements[otherIdx];
          if (rand->kind == X86_64_OK_REG) {
            x86_64OperandVectorInsert(&tempNode->interferedRegs,
                                      x86_64OperandCopy(rand));
          }
        }
      }
    }

    flowGraphUninit(&flowGraph);

    // colour graph for non-mem temp nodes, re-write spills
    for (size_t idx = 0; idx < interferenceGraph.size; idx++) {
      // for each temp
      TempInterferenceGraphNode *node = interferenceGraph.elements[idx];
      AllocHint kind = node->operand->data.temp.kind;
      switch (kind) {
        case AH_SSE:
        case AH_GP: {
          bool assigned = false;
          for (size_t tryIdx = 0;
               tryIdx < (kind == AH_GP ? GP_REGISTER_ALLOC_LIST_SIZE
                                       : SSE_REGISTER_ALLOC_LIST_SIZE);
               tryIdx++) {
            X86_64Register tryReg =
                (kind == AH_GP ? GP_REGISTER_ALLOC_LIST
                               : SSE_REGISTER_ALLOC_LIST)[tryIdx];
            if (!tempInterferenceGraphNodeInterferesWith(node, tryReg)) {
              // assign it to this one
              node->paintAs =
                  x86_64RegOperandCreate(tryReg, node->operand->operandSize);

              // if it's a callee-save register, note that down
              if (x86_64RegIsCalleeSave(tryReg)) {
                calleeSaveSetAdd(&toSave, tryReg);
              }

              // tell every joining node that it's painted as this
              for (size_t otherIdx = 0; otherIdx < node->interferedTemps.size;
                   otherIdx++) {
                X86_64Operand *interfering =
                    node->interferedTemps.elements[otherIdx];
                TempInterferenceGraphNode *interferingNode =
                    tempInterferenceGraphGetNode(&interferenceGraph,
                                                 interfering);
                x86_64OperandVectorInsert(&interferingNode->interferedRegs,
                                          x86_64OperandCopy(node->paintAs));
              }
              assigned = true;
              break;
            }
          }
          if (!assigned) {
            // spilled, do a re-write
            hasSpill = true;
            error(__FILE__, __LINE__, "not yet implemented!");
          }
          break;
        }
        default: {
          // skip mem temps
          break;
        }
      }
      // chose a possible colouring
    }
  }

  frameSize += 8 * toSave.size;
  tempAreaSize += 8 * toSave.size;

  // TODO: group mem temps

  // TODO: lay out mem temp groups - convert groups to rbp offsets

  // TODO: add stack offset to stack offsets

  // replace
  for (size_t idx = 0; idx < instrs->size; idx++) {
    // for every instruction
    X86_64Instruction *instr = instrs->elements[idx];

    bool partiallyDead = false;
    bool partiallyLive = false;
    for (size_t defIdx = 0; defIdx < instr->defines.size; defIdx++) {
      // look through the temp defines
      X86_64Operand *def = instr->defines.elements[defIdx];
      if (def->kind == X86_64_OK_TEMP) {
        if (tempInterferenceGraphContains(&interferenceGraph,
                                          def->data.temp.n)) {
          partiallyLive = true;
        } else {
          partiallyDead = true;
        }
      }
    }

    if (partiallyDead && !partiallyLive) {
      // if it's completely dead, delete it
      x86_64InstructionVectorErase(instrs, idx);
      idx--;
    } else if ((partiallyLive && !partiallyDead) ||
               (!partiallyLive && !partiallyDead)) {
      // if it's completely live, replace stuff in it
      replaceTemps(&instr->defines, &interferenceGraph);
      replaceTemps(&instr->uses, &interferenceGraph);
      replaceTemps(&instr->other, &interferenceGraph);
    } else {
      // partially live and dead
      error(__FILE__, __LINE__, "part-live, part-dead instruction encountered");
    }
  }

  tempInterferenceGraphUninit(&interferenceGraph);

  // fill in prologue and epilogue
  // Prologue:
  X86_64InstructionVector *prologue = x86_64InstructionVectorCreate();
  // push rbp, move rsp to rbp
  X86_64INSERT(prologue, X86_64INSTR(strdup("\tpushq\t%rbp\n")));
  X86_64INSERT(prologue, X86_64INSTR(strdup("\tmovq\t%rsp, %rbp\n")));
  // push all callee saves to save
  for (size_t idx = 0; idx < toSave.size; idx++) {
    X86_64INSERT(prologue, X86_64INSTR(format(
                               "\tpushq\t%s\n",
                               calleeSaveNumberToName(toSave.elements[idx]))));
  }
  // subtract remaining size from stack pointer
  X86_64INSERT(prologue, X86_64INSTR(format("\tsubq\t$%zu, %%rsp\n",
                                            frameSize - (8 * toSave.size))));

  // Epilogue:
  X86_64InstructionVector *epilogue = x86_64InstructionVectorCreate();
  // add remaining size to stack pointer
  X86_64INSERT(epilogue, X86_64INSTR(format("\taddq\t$%zu, %%rsp\n",
                                            frameSize - (8 * toSave.size))));
  // pop all callee saves to save
  for (size_t idx = 1; idx <= toSave.size; idx++) {
    X86_64INSERT(epilogue,
                 X86_64INSTR(format("\tpopq\t%s\n",
                                    calleeSaveNumberToName(
                                        toSave.elements[toSave.size - idx]))));
  }
  // pop rbp
  X86_64INSERT(epilogue, X86_64INSTR(strdup("\tpopq\t%rbp\n")));
  // ret
  X86_64INSERT(epilogue, X86_64INSTR(strdup("\tret\n")));

  frag->data.text.body = x86_64InstructionVectorMerge(
      x86_64InstructionVectorMerge(prologue, instrs), epilogue);

  calleeSaveSetUninit(&toSave);
}
void x86_64RegisterAllocate(FileX86_64FileMap *asmFileMap) {
  for (size_t fileIdx = 0; fileIdx < asmFileMap->capacity; fileIdx++) {
    if (asmFileMap->keys[fileIdx] != NULL) {
      X86_64File *file = asmFileMap->values[fileIdx];
      for (size_t fragmentIdx = 0; fragmentIdx < file->fragments.size;
           fragmentIdx++) {
        X86_64Fragment *frag = file->fragments.elements[fragmentIdx];
        if (frag->kind == X86_64_FK_TEXT) {
          registerAllocateFragment(frag);
        }
      }
    }
  }
}