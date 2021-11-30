// Copyright 2019, 2021 Justin Hu
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
 * intermediate represnetation
 */

#ifndef TLC_IR_IR_H_
#define TLC_IR_IR_H_

#include <stddef.h>
#include <stdint.h>

#include "ast/type.h"
#include "util/container/linkedList.h"
#include "util/container/vector.h"

/** the type of a fragment name */
typedef enum {
  FNT_LOCAL,
  FNT_GLOBAL,
} FragmentNameType;
/** the type of a fragment */
typedef enum {
  FT_BSS,
  FT_RODATA,
  FT_DATA,
  FT_TEXT,
} FragmentType;
/** a fragment */
typedef struct {
  FragmentType type;
  FragmentNameType nameType;
  union {
    char *global;
    size_t local;
  } name;
  union {
    struct {
      size_t alignment;
      Vector data; /**< Vector of IRDatum */
    } data;
    struct {
      LinkedList blocks; /**< list of IRBlock - first one is the entry block */
    } text;
  } data;
} IRFrag;

/** ctors */
IRFrag *globalDataFragCreate(FragmentType type, char *name, size_t alignment);
IRFrag *localDataFragCreate(FragmentType type, size_t name, size_t alignment);
IRFrag *textFragCreate(char *name);
/**
 * given a list of frags, find the one with the label
 */
IRFrag *findFrag(Vector *frags, size_t label);
/** dtor */
void irFragFree(IRFrag *);
void irFragVectorUninit(Vector *);

/** the type of a datum */
typedef enum {
  DT_BYTE,
  DT_SHORT,
  DT_INT,
  DT_LONG,
  DT_PADDING,
  DT_STRING,
  DT_WSTRING,
  DT_LOCAL,
  DT_GLOBAL,
} DatumType;
/** a data element - handles endianness */
typedef struct {
  DatumType type;
  union {
    uint8_t byteVal;
    uint16_t shortVal;
    uint32_t intVal;
    uint64_t longVal;
    size_t paddingLength;
    uint8_t *string;
    uint32_t *wstring;
    size_t localLabel;
    char *globalLabel;
  } data;
} IRDatum;

/** ctors */
IRDatum *byteDatumCreate(uint8_t val);
IRDatum *shortDatumCreate(uint16_t val);
IRDatum *intDatumCreate(uint32_t val);
IRDatum *longDatumCreate(uint64_t val);
IRDatum *paddingDatumCreate(size_t len);
IRDatum *stringDatumCreate(uint8_t *string);
IRDatum *wstringDatumCreate(uint32_t *wstring);
IRDatum *localLabelDatumCreate(size_t label);
IRDatum *globalLabelDatumCreate(char *label);
/** copy */
IRDatum *irDatumCopy(IRDatum const *d);
/** equality */
bool irDatumEqual(IRDatum const *a, IRDatum const *b);
/** dtor */
void irDatumFree(IRDatum *);

/** the kind of an operand */
typedef enum {
  OK_TEMP,
  OK_REG,
  OK_CONSTANT,
} OperandKind;
/** an operand in an IR entry */
typedef struct IROperand {
  OperandKind kind;
  union {
    /**
     * temporary variable
     *
     * alignment is a power of two
     * size > POINTER_WIDTH ==> kind == MEM
     */
    struct {
      size_t name;
      size_t alignment;
      size_t size;
      AllocHint kind;
    } temp;
    /**
     * register - can be used whereever a temp can be
     */
    struct {
      size_t name;  // specific to the target architecture
      size_t size;
    } reg;
    /**
     * constant data
     *
     * allocation = MEM
     * alignment is a power of two
     */
    struct {
      size_t alignment;
      Vector data; /**< Vector of IRDatum */
    } constant;
  } data;
} IROperand;

/** ctors */
IROperand *tempOperandCreate(size_t name, size_t alignment, size_t size,
                             AllocHint kind);
IROperand *regOperandCreate(size_t name, size_t size);
IROperand *constantOperandCreate(size_t alignment);
IROperand *globalOperandCreate(char *name);
IROperand *localOperandCreate(size_t name);
IROperand *offsetOperandCreate(int64_t offset);
/** copy */
IROperand *irOperandCopy(IROperand const *o);
/** sizeof */
size_t irOperandSizeof(IROperand const *o);
/** alignof */
size_t irOperandAlignof(IROperand const *o);
/**
 * is this operand a label and only a label
 */
bool irOperandIsLabel(IROperand const *o);
/**
 * is this operand a global and only a global
 */
bool irOperandIsGlobal(IROperand const *o);
/**
 * is this operand a local and only a local
 */
bool irOperandIsLocal(IROperand const *o);
/**
 * get the name of this label
 */
char const *globalOperandName(IROperand const *o);
/**
 * get the name of this label
 */
size_t localOperandName(IROperand const *o);
/**
 * are two operands equal
 */
bool irOperandEqual(IROperand const *a, IROperand const *b);
/** dtor */
void irOperandFree(IROperand *);

/** an ir operator */
typedef enum IROperator {
  // miscellaneous
  /**
   * local label
   *
   * one operand
   * 0: LOCAL - the label
   */
  IO_LABEL,
  /**
   * fake use of a temp to prevent dead-code elimination of volatile reads
   *
   * one operand
   * 0: TEMP, read - temp to mark as used
   */
  IO_VOLATILE,
  /**
   * fake write to a temp to mark it as explicitly uninitialized
   *
   * one operand
   * 0: TEMP, written - temp to mark as uninitialized
   */
  IO_UNINITIALIZED,
  /**
   * get the address of a mem temp
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (GP | MEM); size ==
   *    POINTER_WIDTH - destination pointer
   * 1: TEMP, read, allocation == MEM - source mem temp
   */
  IO_ADDROF,
  /**
   * no-op - removed during dead-code elimination
   *
   * no operands
   */
  IO_NOP,

  // data transfer
  /**
   * whole-datum move
   *
   * two operands
   * 0: REG | TEMP, written - desination
   * 1: REG | TEMP, read | CONST - source
   *
   * sizeof(0) == sizeof(1)
   */
  IO_MOVE,
  /**
   * store to memory
   *
   * three operands
   * 0: REG | TEMP, read, allocation == (GP | MEM) | CONST;
   * size == POINTER_WIDTH - destination pointer
   * 1: REG | TEMP, read | CONST - source data
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    POINTER_WIDTH - offset
   */
  IO_MEM_STORE,
  /**
   * load from memory
   *
   * three operands
   * 0: REG | TEMP, written - destination data
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST;
   * size == POINTER_WIDTH - source pointer
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    POINTER_WIDTH - offset
   */
  IO_MEM_LOAD,
  /**
   * store to stack relative to stack pointer
   *
   * two operands
   * 0: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    POINTER_WIDTH - stack offset
   * 1: REG | TEMP, read | CONST - source data
   */
  IO_STK_STORE,
  /**
   * load from stack relative to stack pointer
   *
   * two operands
   * 0: REG | TEMP, written - destination data
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    POINTER_WIDTH - stack offset
   */
  IO_STK_LOAD,
  /**
   * store to part of temp
   *
   * three operands
   * 0: mem TEMP, written - destination temp
   * 1: REG | TEMP, read | CONST - source data
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    POINTER_WIDTH - offset
   */
  IO_OFFSET_STORE,
  /**
   * load from part of temp
   *
   * three operands
   * 0: REG | TEMP, written - destination data
   * 1: mem TEMP, read - source temp
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    POINTER_WIDTH - offset
   */
  IO_OFFSET_LOAD,

  // arithmetic
  /**
   * integer binary arithmetic op
   *
   * three operands
   * 0: REG | TEMP, written, allocation == (GP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - lhs
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST - rhs
   *
   * sizeof(0) == sizeof(1) == sizeof(2)
   */
  IO_ADD,
  IO_SUB,
  IO_SMUL,
  IO_UMUL,
  IO_SDIV,
  IO_UDIV,
  IO_SMOD,
  IO_UMOD,
  /**
   * floating binary arithmetic op
   *
   * three operands
   * 0: REG | TEMP, written, allocation == (FP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (FP | MEM) | CONST - lhs
   * 2: REG | TEMP, read, allocation == (FP | MEM) | CONST - rhs
   *
   * sizeof(0) == sizeof(1) == sizeof(2)
   */
  IO_FADD,
  IO_FSUB,
  IO_FMUL,
  IO_FDIV,
  IO_FMOD,
  /**
   * integer unary arithmetic op
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (GP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - arg
   *
   * sizeof(0) == sizeof(1)
   */
  IO_NEG,
  /**
   * floating unary arithmetic op
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (FP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (FP | MEM) | CONST - arg
   *
   * sizeof(0) == sizeof(1)
   */
  IO_FNEG,

  // bit-twiddling
  /**
   * shift op
   *
   * three operands
   * 0: REG | TEMP, written, allocation == (GP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - arg
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    BYTE_WIDTH - amount
   *
   * sizeof(0) == sizeof(1)
   */
  IO_SLL,
  IO_SLR,
  IO_SAR,
  /**
   * binary bitwise op - identical to integer binary arithmetic op
   */
  IO_AND,
  IO_XOR,
  IO_OR,
  /**
   * unary bitwise op - identical to integer unary arithmetic op
   */
  IO_NOT,

  // comparisons, logic
  /**
   * integer binary comparison
   *
   * three operands
   * 0: REG | TEMP, written, allocation == (GP | MEM); size ==
   *    BYTE_WIDTH - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - lhs
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST - rhs
   *
   * sizeof(1) == sizeof(2)
   */
  IO_L,
  IO_LE,
  IO_E,
  IO_NE,
  IO_G,
  IO_GE,
  IO_A,
  IO_AE,
  IO_B,
  IO_BE,
  /**
   * floating binary comparison
   *
   * three operands
   * 0: REG | TEMP, written, allocation == (GP | MEM); size ==
   *    BYTE_WIDTH - destination
   * 1: REG | TEMP, read, allocation == (FP | MEM) | CONST - lhs
   * 2: REG | TEMP, read, allocation == (FP | MEM) | CONST - rhs
   *
   * sizeof(1) == sizeof(2)
   */
  IO_FL,
  IO_FLE,
  IO_FE,
  IO_FNE,
  IO_FG,
  IO_FGE,
  /**
   * unary comparison
   *
   * two operands
   * 0: REG | TEMP, written; size == BYTE_WIDTH - destination
   * 1: REG | TEMP, read | CONST - arg
   */
  IO_Z,
  IO_NZ,
  /**
   * logical not
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (GP | MEM); size ==
   *    BYTE_WIDTH - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST; size ==
   *    BYTE_WIDTH - arg
   */
  IO_LNOT,

  // conversion
  /**
   * extends
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (GP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - arg
   *
   * sizeof(0) > sizeof(1)
   */
  IO_SX,
  IO_ZX,
  /**
   * truncation
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (GP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - arg
   *
   * sizeof(0) < sizeof(1)
   */
  IO_TRUNC,
  /**
   * integer to floating
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (FP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - arg
   */
  IO_U2F,
  IO_S2F,
  /**
   * floating to floating
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (FP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - arg
   *
   * sizeof(0) != sizeof(1)
   */
  IO_FRESIZE,
  /**
   * floating to integral
   *
   * two operands
   * 0: REG | TEMP, written, allocation == (GP | MEM) - destination
   * 1: REG | TEMP, read, allocation == (FP | MEM) | CONST - arg
   */
  IO_F2I,

  // jumps
  /**
   * unconditional jump
   *
   * one operand
   * 0: LOCAL - target
   */
  IO_JUMP,
  /**
   * jump table jump
   *
   * two operands
   * 0: TEMP, read, allocation == (GP | MEM); size == POINTER_WIDTH - target
   * 1: LOCAL - reference to possible targets
   */
  IO_JUMPTABLE,
  /**
   * integer binary comparison conditional jump
   *
   * four operands
   * 0: LOCAL - true target
   * 1: LOCAL - false target
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST - lhs
   * 3: REG | TEMP, read, allocation == (GP | MEM) | CONST - rhs
   *
   * sizeof(2) == sizeof(3)
   */
  IO_J2L,
  IO_J2LE,
  IO_J2E,
  IO_J2NE,
  IO_J2G,
  IO_J2GE,
  IO_J2A,
  IO_J2AE,
  IO_J2B,
  IO_J2BE,
  /**
   * floating binary comparison conditional jump
   *
   * four operands
   * 0: LOCAL - true target
   * 1: LOCAL - false target
   * 2: REG | TEMP, read, allocation == (FP | MEM) | CONST - lhs
   * 3: REG | TEMP, read, allocation == (FP | MEM) | CONST - rhs
   *
   * sizeof(2) == sizeof(3)
   */
  IO_J2FL,
  IO_J2FLE,
  IO_J2FE,
  IO_J2FNE,
  IO_J2FG,
  IO_J2FGE,
  /**
   * unary comparison conditional jump
   *
   * three operands
   * 0: LOCAL - true target
   * 1: LOCAL - false target
   * 2: REG | TEMP, read | CONST - arg
   */
  IO_J2Z,
  IO_J2NZ,
  /**
   * integer binary comparison conditional jump (fallthrough version)
   *
   * four operands
   * 0: LOCAL - true target
   * 1: REG | TEMP, read, allocation == (GP | MEM) | CONST - lhs
   * 2: REG | TEMP, read, allocation == (GP | MEM) | CONST - rhs
   *
   * sizeof(2) == sizeof(3)
   */
  IO_J1L,
  IO_J1LE,
  IO_J1E,
  IO_J1NE,
  IO_J1G,
  IO_J1GE,
  IO_J1A,
  IO_J1AE,
  IO_J1B,
  IO_J1BE,
  /**
   * floating binary comparison conditional jump (fallthrough version)
   *
   * four operands
   * 0: LOCAL - true target
   * 1: REG | TEMP, read, allocation == (FP | MEM) | CONST - lhs
   * 2: REG | TEMP, read, allocation == (FP | MEM) | CONST - rhs
   *
   * sizeof(2) == sizeof(3)
   */
  IO_J1FL,
  IO_J1FLE,
  IO_J1FE,
  IO_J1FNE,
  IO_J1FG,
  IO_J1FGE,
  /**
   * unary comparison conditional jump (fallthrough version)
   *
   * three operands
   * 0: LOCAL - true target
   * 1: REG | TEMP, read | CONST - arg
   */
  IO_J1Z,
  IO_J1NZ,

  // function calling
  /**
   * function call
   *
   * one operand
   * 0: REG | TEMP, read, allocation == (GP | MEM) | GLOBAL | LOCAL; size ==
   *    POINTER_WIDTH - target
   */
  IO_CALL,
  /**
   * return from function
   *
   * no operands
   */
  IO_RETURN,  // no args
} IROperator;

/**
 * get the arity of an ir operator
 */
size_t irOperatorArity(IROperator op);

/** ir instruction */
typedef struct {
  IROperator op;
  IROperand **args;
} IRInstruction;

/** generic ctor */
IRInstruction *irInstructionCreate(IROperator op);
IRInstruction *irInstructionCopy(IRInstruction const *i);
/** dtor */
void irInstructionFree(IRInstruction *);
void irInstructionMakeNop(IRInstruction *);

typedef struct {
  size_t label;
  LinkedList instructions;
} IRBlock;

/** ctor */
IRBlock *irBlockCreate(size_t label);
/**
 * find the index of a block given its label
 */
size_t indexOfBlock(LinkedList *blocks, size_t label);
/**
 * get a block given its label
 */
IRBlock *findBlock(LinkedList *blocks, size_t label);
/** dtor */
void irBlockFree(IRBlock *);

extern char const *const IROPERATOR_NAMES[];
extern char const *const IROPERAND_NAMES[];
extern char const *const ALLOCHINT_NAMES[];

/**
 * checks that all files in the file list have valid IR (while IR is in
 * basic blocks)
 *
 * This checks that
 *  - temps must have consistent sizing, alignment, and allocation
 *  - all operations have valid sizing
 *  - there are no non-terminal jumps or returns
 *  - there are no one-arg jumps
 *  - there are no labels
 *
 * @param phase phase to name as the one at fault
 * @returns -1 on failure, 0 on success
 */
int validateBlockedIr(char const *phase);

/**
 * checks that all files in the file list have valid IR (while IR is a single
 * block)
 *
 * This checks the same things as blocked IR except:
 *  - non-terminal jumps are allowed
 *  - there are no two-arg jumps
 *  - one-arg jumps are allowed
 *  - labels are allowed
 *  - nops are not allowed
 *
 * @param phase phase to name as the one at fault
 * @returns -1 on failure, 0 on success
 */
int validateScheduledIr(char const *phase);

#endif  // TLC_IR_IR_H_