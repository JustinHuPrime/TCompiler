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

#include "util/container/linkedList.h"
#include "util/container/vector.h"

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
  char *name;
  union {
    struct {
      size_t alignment;
      Vector data; /**< Vector of IRDatum */
    } data;
    struct {
      Vector blocks; /**< vector of IRBlock - first one is the entry block */
    } text;
  } data;
} IRFrag;

/** ctors */
IRFrag *dataFragCreate(FragmentType type, char *name, size_t alignment);
IRFrag *textFragCreate(char *name);
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
  DT_LABEL,
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
    size_t label;
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
IRDatum *labelDatumCreate(size_t label);
/** dtor */
void irDatumFree(IRDatum *);

/** allocation hints for temps */
typedef enum {
  AH_GP,  /** integer-like things */
  AH_MEM, /** structs, arrays, unions */
  AH_FP,  /** floating-points */
} AllocHint;
/** the kind of an operand */
typedef enum {
  OK_TEMP,
  OK_REG,
  OK_CONSTANT,
  OK_LABEL,
  OK_ASM,
} OperandKind;
/** an operand in an IR entry */
typedef struct IROperand {
  OperandKind kind;
  union {
    struct {
      size_t name;
      size_t alignment;
      size_t size;
      AllocHint kind;
    } temp;
    struct {
      size_t name;  // specific to the target architecture
    } reg;
    struct {
      IRDatum *value;
    } constant;
    struct {
      char *name;
    } label;
    struct {
      char *assembly;
    } assembly;
  } data;
} IROperand;

/** ctors */
IROperand *tempOperandCreate(size_t name, size_t alignment, size_t size,
                             AllocHint kind);
IROperand *regOperandCreate(size_t name);
IROperand *constantOperandCreate(IRDatum *value);
IROperand *labelOperandCreate(char *name);
IROperand *assemblyOperandCreate(char *name);
/** dtor */
void irOperandFree(IROperand *);

/** an ir operator */
typedef enum IROperator {
  // miscellaneous
  IO_ASM,    // inline asm
  IO_LABEL,  // label

  // data transfer
  IO_MOVE,
  IO_MEM_STORE,
  IO_MEM_LOAD,
  IO_STK_STORE,
  IO_STK_LOAD,
  IO_OFFSET_STORE,
  IO_OFFSET_LOAD,

  // arithmetic
  IO_ADD,
  IO_FADD,
  IO_SUB,
  IO_FSUB,
  IO_SMUL,
  IO_UMUL,
  IO_FMUL,
  IO_SDIV,
  IO_UDIV,
  IO_FDIV,
  IO_SMOD,
  IO_UMOD,
  IO_FMOD,
  IO_NEG,
  IO_FNEG,

  // bit-twiddling
  IO_SLL,
  IO_SLR,
  OP_SAR,
  IO_AND,
  IO_XOR,
  IO_OR,
  IO_NOT,

  // comparisons, logic
  IO_L,
  IO_LE,
  IO_E,
  IO_NE,
  IO_GE,
  IO_G,
  IO_A,
  IO_AE,
  IO_B,
  IO_BE,
  IO_FL,
  IO_FLE,
  IO_FE,
  IO_FNE,
  IO_FGE,
  IO_FG,
  IO_LNOT,

  // conversion
  IO_SX_SHORT,
  IO_SX_INT,
  IO_SX_LONG,
  IO_ZX_SHORT,
  IO_ZX_INT,
  IO_ZX_LONG,
  IO_TRUNC_BYTE,
  IO_TRUNC_SHORT,
  IO_TRUNC_INT,
  IO_U2FLOAT,
  IO_U2DOUBLE,
  IO_S2FLOAT,
  IO_S2DOUBLE,
  IO_F2FLOAT,
  IO_F2DOUBLE,
  IO_F2BYTE,
  IO_F2SHORT,
  IO_F2INT,
  IO_F2LONG,

  // jumps
  IO_JUMP,
  IO_JL,
  IO_JLE,
  IO_JE,
  IO_JNE,
  IO_JGE,
  IO_JG,
  IO_JA,
  IO_JAE,
  IO_JB,
  IO_JBE,
  IO_JFL,
  IO_JFLE,
  IO_JFE,
  IO_JFNE,
  IO_JFGE,
  IO_JFG,

  // function calling
  IO_CALL,
  IO_RETURN,  // no args
} IROperator;
/** ir instruction */
typedef struct {
  IROperator op;
  size_t size;
  IROperand *dest;
  IROperand *arg1;
  IROperand *arg2;
} IRInstruction;

/** generic ctor */
IRInstruction *irInstructionCreate(IROperator op, size_t size, IROperand *dest,
                                   IROperand *arg1, IROperand *arg2);
/** dtor */
void irInstructionFree(IRInstruction *);

typedef struct {
  size_t label;
  LinkedList instructions;
} IRBlock;

/** ctor */
IRBlock *irBlockCreate(size_t label);
/** dtor */
void irBlockFree(IRBlock *);

#endif  // TLC_IR_IR_H_