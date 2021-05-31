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
      Vector instructions; /**< vector of IRInstruction */
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
    size_t label; /** gets formatted as ".L%zu" */
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
      char *assembly;
    } assembly;
  } data;
} IROperand;

/** dtor */
void irOperandFree(IROperand *);

/** an ir operator */
typedef enum IROperator {
  IO_ASM,    // inline assembly: arg1 = assembly (constant)
  IO_LABEL,  // label the next entry: arg1 = label name (constant)
  IO_MOVE,   // move to temp or reg: dest = target reg or temp, arg1 = source
} IROperator;
/** ir instruction */
typedef struct {
  IROperator op;
  IROperand *dest;
  IROperand *arg1;
  IROperand *arg2;
} IRInstruction;

/** dtor */
void irInstructionFree(IRInstruction *);

#endif  // TLC_IR_IR_H_