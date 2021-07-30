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
/** copy */
IRDatum *irDatumCopy(IRDatum const *d);
/** dtor */
void irDatumFree(IRDatum *);

/** the kind of an operand */
typedef enum {
  OK_TEMP,
  OK_REG,
  OK_CONSTANT,
  OK_LABEL,
  OK_OFFSET,
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
      size_t size;
    } reg;
    struct {
      size_t alignment;
      Vector data; /**< Vector of IRDatum */
    } constant;
    struct {
      char *name;
    } label;
    struct {
      int64_t offset;
    } offset;
    struct {
      char *assembly;
    } assembly;
  } data;
} IROperand;

/** ctors */
IROperand *tempOperandCreate(size_t name, size_t alignment, size_t size,
                             AllocHint kind);
IROperand *regOperandCreate(size_t name, size_t size);
IROperand *constantOperandCreate(size_t alignment);
IROperand *labelOperandCreate(char *name);
IROperand *offsetOperandCreate(int64_t offset);
IROperand *assemblyOperandCreate(char *assembly);
/** copy */
IROperand *irOperandCopy(IROperand const *o);
/** dtor */
void irOperandFree(IROperand *);

/** an ir operator */
typedef enum IROperator {
  // miscellaneous
  IO_ASM,
  IO_LABEL,
  IO_VOLATILE,  // mark temp as being volatilely used
  IO_ADDROF,    // get address of a mem temp

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
  IO_SAR,
  IO_AND,
  IO_XOR,
  IO_OR,
  IO_NOT,

  // comparisons, logic
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
  IO_FL,
  IO_FLE,
  IO_FE,
  IO_FNE,
  IO_FG,
  IO_FGE,
  IO_Z,
  IO_NZ,
  IO_LNOT,

  // conversion
  IO_SX,
  IO_ZX,
  IO_TRUNC,
  IO_UNSIGNED2FLOATING,
  IO_SIGNED2FLOATING,
  IO_RESIZEFLOATING,
  IO_FLOATING2INTEGRAL,

  // jumps
  IO_JUMP,
  IO_JL,
  IO_JLE,
  IO_JE,
  IO_JNE,
  IO_JG,
  IO_JGE,
  IO_JA,
  IO_JAE,
  IO_JB,
  IO_JBE,
  IO_JFL,
  IO_JFLE,
  IO_JFE,
  IO_JFNE,
  IO_JFG,
  IO_JFGE,
  IO_JZ,
  IO_JNZ,

  // function calling
  IO_CALL,
  IO_RETURN,  // no args
} IROperator;
/** ir instruction */
typedef struct {
  IROperator op;
  IROperand *dest;
  IROperand *arg1;
  IROperand *arg2;
} IRInstruction;

/** generic ctor */
IRInstruction *irInstructionCreate(IROperator op, IROperand *dest,
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