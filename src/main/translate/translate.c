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

// Implementation of translation

#include "translate/translate.h"

#include "constants.h"
#include "ir/frame.h"
#include "ir/ir.h"
#include "ir/shorthand.h"
#include "parser/parser.h"
#include "typecheck/symbolTable.h"
#include "util/format.h"
#include "util/internalError.h"
#include "util/nameUtils.h"
#include "util/numeric.h"
#include "util/tstring.h"

#include <stdlib.h>
#include <string.h>

// fragments
static Fragment *fragmentCreate(FragmentKind kind, char *label) {
  Fragment *f = malloc(sizeof(Fragment));
  f->kind = kind;
  f->label = label;
  return f;
}
Fragment *bssFragmentCreate(char *label, size_t size, size_t alignment) {
  Fragment *f = fragmentCreate(FK_BSS, label);
  f->data.bss.size = size;
  f->data.bss.alignment = alignment;
  return f;
}
Fragment *rodataFragmentCreate(char *label, size_t alignment) {
  Fragment *f = fragmentCreate(FK_RODATA, label);
  f->data.rodata.ir = irEntryVectorCreate();
  f->data.rodata.alignment = alignment;
  return f;
}
Fragment *dataFragmentCreate(char *label, size_t alignment) {
  Fragment *f = fragmentCreate(FK_DATA, label);
  f->data.data.ir = irEntryVectorCreate();
  f->data.data.alignment = alignment;
  return f;
}
Fragment *textFragmentCreate(char *label, Frame *frame,
                             TempAllocator *tempAllocator) {
  Fragment *f = fragmentCreate(FK_TEXT, label);
  f->data.text.frame = frame;
  f->data.text.ir = irEntryVectorCreate();
  f->data.text.tempAllocator = tempAllocator;
  return f;
}
void fragmentDestroy(Fragment *f) {
  switch (f->kind) {
    case FK_BSS: {
      break;
    }
    case FK_RODATA: {
      irEntryVectorDestroy(f->data.rodata.ir);
      break;
    }
    case FK_DATA: {
      irEntryVectorDestroy(f->data.data.ir);
      break;
    }
    case FK_TEXT: {
      irEntryVectorDestroy(f->data.text.ir);
      frameDtor(f->data.text.frame);
      tempAllocatorDestroy(f->data.text.tempAllocator);
      break;
    }
  }
  free(f->label);
  free(f);
}

// fragment vector
FragmentVector *fragmentVectorCreate(void) { return vectorCreate(); }
void fragmentVectorInit(FragmentVector *v) { vectorInit(v); }
void fragmentVectorInsert(FragmentVector *v, Fragment *f) {
  vectorInsert(v, f);
}
void fragmentVectorUninit(FragmentVector *v) {
  vectorUninit(v, (void (*)(void *))fragmentDestroy);
}
void fragmentVectorDestroy(FragmentVector *v) {
  vectorDestroy(v, (void (*)(void *))fragmentDestroy);
}

IRFile *irFileCreate(char *filename, LabelGenerator *labelGenerator) {
  IRFile *file = malloc(sizeof(IRFile));
  fragmentVectorInit(&file->fragments);
  file->filename = filename;
  file->labelGenerator = labelGenerator;
  return file;
}
void irFileDestroy(IRFile *file) {
  fragmentVectorUninit(&file->fragments);
  free(file->filename);
  labelGeneratorDtor(file->labelGenerator);
  free(file);
}

void fileIRFileMapInit(FileIRFileMap *map) { hashMapInit(map); }
IRFile *fileIRFileMapGet(FileIRFileMap *map, char const *key) {
  return hashMapGet(map, key);
}
int fileIRFileMapPut(FileIRFileMap *map, char const *key, IRFile *file) {
  return hashMapPut(map, key, file, (void (*)(void *))irFileDestroy);
}
void fileIRFileMapUninit(FileIRFileMap *map) {
  hashMapUninit(map, (void (*)(void *))irFileDestroy);
}

// typeKindof
static AllocHint typeKindof(Type const *type) {
  switch (type->kind) {
    case K_UBYTE:
    case K_BYTE:
    case K_BOOL:
    case K_CHAR:
    case K_USHORT:
    case K_SHORT:
    case K_UINT:
    case K_INT:
    case K_WCHAR:
    case K_ULONG:
    case K_LONG:
    case K_PTR:
    case K_FUNCTION_PTR:
    case K_ENUM: {
      return AH_GP;
    }
    case K_FLOAT:
    case K_DOUBLE: {
      return AH_SSE;
    }
    case K_STRUCT:
    case K_UNION:
    case K_ARRAY:
    case K_AGGREGATE_INIT: {
      return AH_MEM;
    }
    case K_CONST: {
      return typeKindof(type->data.modifier.type);
    }
    case K_TYPEDEF: {
      return typeKindof(
          type->data.reference.referenced->data.type.data.typedefType.type);
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered an invalid TypeKind enum constant");
    }
  }
}

// name stuff
static char *codeFilenameToAssembyFilename(char const *codeFilename) {
  size_t len = strlen(codeFilename);
  char *assemblyFilename = strncpy(malloc(len), codeFilename, len);
  assemblyFilename[len - 2] = 's';
  assemblyFilename[len - 1] = '\0';
  return assemblyFilename;
}
static char *mangleModuleName(char const *moduleName) {
  char *buffer = strdup("__Z");
  StringVector *exploded = explodeName(moduleName);
  for (size_t idx = 0; idx < exploded->size; idx++) {
    char *newBuffer = format("%s%zu%s", buffer, strlen(exploded->elements[idx]),
                             (char const *)exploded->elements[idx]);
    free(buffer);
    buffer = newBuffer;
  }
  stringVectorDestroy(exploded, true);
  return buffer;
}
static char *mangleTypeName(const char *moduleName, const char *typeName) {
  return format("%s%zu%s", mangleModuleName(moduleName), strlen(typeName),
                typeName);
}
static char *mangleTypeString(TypeVector const *);
static char *mangleType(Type const *type) {
  switch (type->kind) {
    case K_VOID: {
      return strdup("v");
    }
    case K_UBYTE: {
      return strdup("ub");
    }
    case K_BYTE: {
      return strdup("sb");
    }
    case K_CHAR: {
      return strdup("c");
    }
    case K_USHORT: {
      return strdup("us");
    }
    case K_SHORT: {
      return strdup("ss");
    }
    case K_UINT: {
      return strdup("ui");
    }
    case K_INT: {
      return strdup("si");
    }
    case K_WCHAR: {
      return strdup("w");
    }
    case K_ULONG: {
      return strdup("ul");
    }
    case K_LONG: {
      return strdup("sl");
    }
    case K_FLOAT: {
      return strdup("f");
    }
    case K_DOUBLE: {
      return strdup("d");
    }
    case K_BOOL: {
      return strdup("B");
    }
    case K_STRUCT:
    case K_UNION:
    case K_ENUM:
    case K_TYPEDEF: {
      char *mangledTypeName =
          mangleTypeName(type->data.reference.referenced->module,
                         type->data.reference.referenced->data.type.name);
      char *buffer = format("T%zu%s", strlen(mangledTypeName), mangledTypeName);
      free(mangledTypeName);
      return buffer;
    }
    case K_CONST: {
      char *mangledType = mangleType(type->data.modifier.type);
      char *buffer = format("C%s", mangledType);
      free(mangledType);
      return buffer;
    }
    case K_ARRAY: {
      char *mangledType = mangleType(type->data.array.type);
      char *buffer = format("A%zu%s", type->data.array.size, mangledType);
      free(mangledType);
      return buffer;
    }
    case K_PTR: {
      char *mangledType = mangleType(type->data.modifier.type);
      char *buffer = format("P%s", mangledType);
      free(mangledType);
      return buffer;
    }
    case K_FUNCTION_PTR: {
      char *mangledReturnType = mangleType(type->data.functionPtr.returnType);
      char *mangledArgTypes =
          mangleTypeString(type->data.functionPtr.argumentTypes);
      char *buffer = format("F%s%s", mangledReturnType, mangledArgTypes);
      free(mangledReturnType);
      free(mangledArgTypes);
      return buffer;
    }
    default: {
      error(__FILE__, __LINE__,
            "attempted to mangle an unexpressable type (aggregate init type?)");
    }
  }
}
static char *mangleTypeString(TypeVector const *args) {
  char *buffer = strdup("");
  for (size_t idx = 0; idx < args->size; idx++) {
    char *mangledType = mangleType(args->elements[idx]);
    char *newBuffer = format("%s%s", buffer, mangledType);
    free(buffer);
    buffer = newBuffer;
    free(mangledType);
  }
  return buffer;
}
static char *mangleVarName(char const *moduleName, char const *id) {
  char *mangledModuleName = mangleModuleName(moduleName);
  char *retVal = format("%s%zu%s", mangledModuleName, strlen(id), id);
  free(mangledModuleName);
  return retVal;
}
static char *mangleFunctionName(char const *moduleName, char const *id,
                                TypeVector *argumentTypes) {
  char *mangledModuleName = mangleModuleName(moduleName);
  char *mangledArgumentTypes = mangleTypeString(argumentTypes);
  char *retVal = format("%s%zu%s%s", mangledModuleName, strlen(id), id,
                        mangledArgumentTypes);
  free(mangledModuleName);
  free(mangledArgumentTypes);
  return retVal;
}

// typeof
static Type const *expressionTypeof(Node const *exp) {
  switch (exp->type) {
    case NT_SEQEXP: {
      return exp->data.seqExp.resultType;
    }
    case NT_BINOPEXP: {
      return exp->data.binOpExp.resultType;
    }
    case NT_UNOPEXP: {
      return exp->data.unOpExp.resultType;
    }
    case NT_COMPOPEXP: {
      return exp->data.compOpExp.resultType;
    }
    case NT_LANDASSIGNEXP: {
      return exp->data.landAssignExp.resultType;
    }
    case NT_LORASSIGNEXP: {
      return exp->data.lorAssignExp.resultType;
    }
    case NT_TERNARYEXP: {
      return exp->data.ternaryExp.resultType;
    }
    case NT_LANDEXP: {
      return exp->data.landExp.resultType;
    }
    case NT_LOREXP: {
      return exp->data.lorExp.resultType;
    }
    case NT_STRUCTACCESSEXP: {
      return exp->data.structAccessExp.resultType;
    }
    case NT_STRUCTPTRACCESSEXP: {
      return exp->data.structPtrAccessExp.resultType;
    }
    case NT_FNCALLEXP: {
      return exp->data.fnCallExp.resultType;
    }
    case NT_CONSTEXP: {
      return exp->data.constExp.resultType;
    }
    case NT_AGGREGATEINITEXP: {
      return exp->data.aggregateInitExp.resultType;
    }
    case NT_CASTEXP: {
      return exp->data.castExp.resultType;
    }
    case NT_SIZEOFTYPEEXP: {
      return exp->data.sizeofTypeExp.resultType;
    }
    case NT_SIZEOFEXPEXP: {
      return exp->data.sizeofExpExp.resultType;
    }
    case NT_ID: {
      return exp->data.id.resultType;
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered a non-expression in an expression position");
    }
  }
}

// constant stuff
static bool constantIsZero(Node *initializer) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          return initializer->data.constExp.value.ubyteVal == 0;
        }
        case CT_BYTE: {
          return initializer->data.constExp.value.byteVal == 0;
        }
        case CT_CHAR: {
          return initializer->data.constExp.value.charVal == 0;
        }
        case CT_USHORT: {
          return initializer->data.constExp.value.ushortVal == 0;
        }
        case CT_SHORT: {
          return initializer->data.constExp.value.shortVal == 0;
        }
        case CT_UINT: {
          return initializer->data.constExp.value.uintVal == 0;
        }
        case CT_INT: {
          return initializer->data.constExp.value.intVal == 0;
        }
        case CT_WCHAR: {
          return initializer->data.constExp.value.wcharVal == 0;
        }
        case CT_ULONG: {
          return initializer->data.constExp.value.ulongVal == 0;
        }
        case CT_LONG: {
          return initializer->data.constExp.value.longVal == 0;
        }
        case CT_FLOAT: {
          return initializer->data.constExp.value.floatBits == 0;
        }
        case CT_DOUBLE: {
          return initializer->data.constExp.value.doubleBits == 0;
        }
        case CT_BOOL: {
          return initializer->data.constExp.value.boolVal == false;
        }
        case CT_STRING:
        case CT_WSTRING: {
          return false;
        }
        case CT_NULL: {
          return true;
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid ConstType enum constant");
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      NodeList *elements = initializer->data.aggregateInitExp.elements;
      for (size_t idx = 0; idx < elements->size; idx++) {
        if (!constantIsZero(elements->elements[idx])) {
          return false;
        }
      }
      return true;
    }
    default: {
      error(__FILE__, __LINE__, "expected a constant, found something else");
    }
  }
}
static void constantToData(Node *initializer, IREntryVector *out,
                           FragmentVector *fragments,
                           LabelGenerator *labelGenerator) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          IR(out, CONST(BYTE_WIDTH,
                        UBYTE(initializer->data.constExp.value.ubyteVal)));
          return;
        }
        case CT_BYTE: {
          IR(out,
             CONST(BYTE_WIDTH, BYTE(initializer->data.constExp.value.byteVal)));
          return;
        }
        case CT_CHAR: {
          IR(out, CONST(CHAR_WIDTH,
                        UBYTE(initializer->data.constExp.value.charVal)));
          return;
        }
        case CT_USHORT: {
          IR(out, CONST(SHORT_WIDTH,
                        USHORT(initializer->data.constExp.value.ushortVal)));
          return;
        }
        case CT_SHORT: {
          IR(out, CONST(SHORT_WIDTH,
                        SHORT(initializer->data.constExp.value.shortVal)));
          return;
        }
        case CT_UINT: {
          IR(out,
             CONST(INT_WIDTH, UINT(initializer->data.constExp.value.uintVal)));
          return;
        }
        case CT_INT: {
          IR(out,
             CONST(INT_WIDTH, INT(initializer->data.constExp.value.intVal)));
          return;
        }
        case CT_WCHAR: {
          IR(out, CONST(WCHAR_WIDTH,
                        UINT(initializer->data.constExp.value.wcharVal)));
          return;
        }
        case CT_ULONG: {
          IR(out, CONST(LONG_WIDTH,
                        ULONG(initializer->data.constExp.value.ulongVal)));
          return;
        }
        case CT_LONG: {
          IR(out,
             CONST(LONG_WIDTH, LONG(initializer->data.constExp.value.longVal)));
          return;
        }
        case CT_FLOAT: {
          IR(out, CONST(FLOAT_WIDTH,
                        FLOAT(initializer->data.constExp.value.floatBits)));
          return;
        }
        case CT_DOUBLE: {
          IR(out, CONST(DOUBLE_WIDTH,
                        DOUBLE(initializer->data.constExp.value.doubleBits)));
          return;
        }
        case CT_BOOL: {
          IR(out,
             CONST(BYTE_WIDTH,
                   UBYTE(initializer->data.constExp.value.boolVal ? 1 : 0)));
          return;
        }
        case CT_STRING: {
          Fragment *f =
              rodataFragmentCreate(NEW_DATA_LABEL(labelGenerator), CHAR_WIDTH);
          IR(f->data.rodata.ir,
             CONST(0, STRING(tstrdup(
                          initializer->data.constExp.value.stringVal))));
          fragmentVectorInsert(fragments, f);
          IR(out, CONST(POINTER_WIDTH, NAME(strdup(f->label))));
          return;
        }
        case CT_WSTRING: {
          Fragment *f =
              rodataFragmentCreate(NEW_DATA_LABEL(labelGenerator), CHAR_WIDTH);
          IR(f->data.rodata.ir,
             CONST(0, WSTRING(twstrdup(
                          initializer->data.constExp.value.wstringVal))));
          fragmentVectorInsert(fragments, f);
          IR(out, CONST(POINTER_WIDTH, NAME(strdup(f->label))));
          return;
        }
        case CT_NULL: {
          IR(out, CONST(POINTER_WIDTH, ULONG(0)));
          return;
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid ConstType enum constant");
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      NodeList *elements = initializer->data.aggregateInitExp.elements;
      for (size_t idx = 0; idx < elements->size; idx++) {
        constantToData(elements->elements[idx], out, fragments, labelGenerator);
      }
      return;
    }
    default: {
      error(__FILE__, __LINE__, "expected a constant, found something else");
    }
  }
}

// global accesses
static void addGlobalAccesses(SymbolTable *stab, char const *moduleName,
                              GlobalAccessCtor globalAccessCtor,
                              FunctionAccessCtor functionAccessCtor) {
  for (size_t idx = 0; idx < stab->capacity; idx++) {
    if (stab->keys[idx] != NULL) {
      SymbolInfo *info = stab->values[idx];
      if (info->kind == SK_FUNCTION) {
        OverloadSet *set = &info->data.function.overloadSet;
        for (size_t overloadIdx = 0; overloadIdx < set->size; overloadIdx++) {
          OverloadSetElement *elm = set->elements[overloadIdx];
          elm->access = functionAccessCtor(mangleFunctionName(
              moduleName, stab->keys[idx], &elm->argumentTypes));
        }
      } else if (info->kind == SK_VAR) {
        info->data.var.access = globalAccessCtor(
            typeSizeof(info->data.var.type), typeAlignof(info->data.var.type),
            typeKindof(info->data.var.type),
            mangleVarName(moduleName, stab->keys[idx]));
      }
    }
  }
}

// default args
static IROperand *defaultArgToOperand(Node *initializer, Type const *argType,
                                      FragmentVector *fragments,
                                      LabelGenerator *labelGenerator) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          return NULL;
        }
        case CT_BYTE: {
          return NULL;
        }
        case CT_CHAR: {
          return NULL;
        }
        case CT_USHORT: {
          return NULL;
        }
        case CT_SHORT: {
          return NULL;
        }
        case CT_UINT: {
          return NULL;
        }
        case CT_INT: {
          return NULL;
        }
        case CT_WCHAR: {
          return NULL;
        }
        case CT_ULONG: {
          return NULL;
        }
        case CT_LONG: {
          return NULL;
        }
        case CT_FLOAT: {
          return NULL;
        }
        case CT_DOUBLE: {
          return NULL;
        }
        case CT_BOOL: {
          return NULL;
        }
        case CT_STRING: {
          return NULL;
        }
        case CT_WSTRING: {
          return NULL;
        }
        case CT_NULL: {
          return NULL;
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid ConstType enum constant");
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      return NULL;
    }
    default: {
      error(__FILE__, __LINE__, "expected a constant, found something else");
    }
  }
}
static void addDefaultArgs(Node *file, FragmentVector *fragments,
                           LabelGenerator *labelGenerator) {
  NodeList *bodies = file->data.file.bodies;
  for (size_t bodyIdx = 0; bodyIdx < bodies->size; bodyIdx++) {
    Node *body = bodies->elements[bodyIdx];
    if (body->type == NT_FUNCTION) {
      NodeTripleList *formals = body->data.function.formals;
      OverloadSetElement *elm = body->data.function.id->data.id.overload;
      IROperandVector *defaultArgs = &elm->defaultArgs;
      for (size_t idx = elm->argumentTypes.size - elm->numOptional;
           idx < elm->argumentTypes.size; idx++) {
        irOperandVectorInsert(
            defaultArgs, defaultArgToOperand(formals->thirdElements[idx],
                                             elm->argumentTypes.elements[idx],
                                             fragments, labelGenerator));
      }
    }
  }
}

// translation - expressions
struct Lvalue;
typedef struct {
  void (*dtor)(struct Lvalue *this);
  IROperand *(*load)(struct Lvalue *this, IREntryVector *out,
                     TempAllocator *tempAllocator);
  void (*store)(struct Lvalue *this, IREntryVector *out, IROperand *value,
                TempAllocator *tempAllocator);
  IROperand *(*addrof)(struct Lvalue *this, IREntryVector *out,
                       TempAllocator *tempAllocator);
} LvalueVTable;
typedef struct Lvalue {
  LvalueVTable *vtable;
} Lvalue;
static void lvalueDtor(Lvalue *this) { this->vtable->dtor(this); }
static IROperand *lvalueLoad(Lvalue *this, IREntryVector *out,
                             TempAllocator *tempAllocator) {
  return this->vtable->load(this, out, tempAllocator);
}
static void lvalueStore(Lvalue *this, IREntryVector *out, IROperand *value,
                        TempAllocator *tempAllocator) {
  this->vtable->store(this, out, value, tempAllocator);
}
static IROperand *lvalueAddrof(Lvalue *this, IREntryVector *out,
                               TempAllocator *tempAllocator) {
  return this->vtable->addrof(this, out, tempAllocator);
}

static IROperand *translateCast(IROperand *from, Type const *fromType,
                                Type const *toType, IREntryVector *out,
                                TempAllocator *tempAllocator) {
  error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
}
static Lvalue *translateLvalue(Node *, IREntryVector *, FragmentVector *,
                               Frame *, LabelGenerator *, TempAllocator *);
static IROperand *translateRvalue(Node *, IREntryVector *, FragmentVector *,
                                  Frame *, LabelGenerator *, TempAllocator *);
static void translateJumpIfNot(Node *, IREntryVector *, FragmentVector *,
                               Frame *, LabelGenerator *, TempAllocator *,
                               char const *);

static void translateJumpIf(Node *, IREntryVector *, FragmentVector *, Frame *,
                            LabelGenerator *, TempAllocator *, char const *);
static void translateVoiddedValue(Node *exp, IREntryVector *out,
                                  FragmentVector *fragments, Frame *frame,
                                  LabelGenerator *labelGenerator,
                                  TempAllocator *tempAllocator) {
  switch (exp->type) {
    case NT_SEQEXP: {
      translateVoiddedValue(exp->data.seqExp.prefix, out, fragments, frame,
                            labelGenerator, tempAllocator);
      translateVoiddedValue(exp->data.seqExp.last, out, fragments, frame,
                            labelGenerator, tempAllocator);
      break;
    }
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_ASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          lvalueStore(
              lhs, out,
              translateCast(
                  translateRvalue(exp->data.binOpExp.rhs, out, fragments, frame,
                                  labelGenerator, tempAllocator),
                  expressionTypeof(exp->data.binOpExp.rhs),
                  exp->data.binOpExp.resultType, out, tempAllocator),
              tempAllocator);

          lvalueDtor(lhs);
          break;
        }
        case BO_MULASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *resultType = exp->data.binOpExp.resultType;
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(resultType);
          AllocHint kind = typeKindof(resultType);

          IROperator op;
          if (typeIsFloat(resultType)) {
            op = IO_FP_MUL;
          } else if (typeIsSignedIntegral(resultType)) {
            op = IO_SMUL;
          } else {
            op = IO_UMUL;
          }
          IR(out,
             BINOP(size, op, TEMP(temp, size, size, kind),
                   translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                 resultType, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), resultType,
                       out, tempAllocator)));
          lvalueStore(lhs, out,
                      translateCast(TEMP(temp, size, size, kind),
                                    exp->data.binOpExp.resultType, lhsType, out,
                                    tempAllocator),
                      tempAllocator);

          lvalueDtor(lhs);
          break;
        }
        case BO_DIVASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *resultType = exp->data.binOpExp.resultType;
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(resultType);
          AllocHint kind = typeKindof(resultType);

          IROperator op;
          if (typeIsFloat(resultType)) {
            op = IO_FP_DIV;
          } else if (typeIsSignedIntegral(resultType)) {
            op = IO_SDIV;
          } else {
            op = IO_UDIV;
          }
          IR(out,
             BINOP(size, op, TEMP(temp, size, size, kind),
                   translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                 resultType, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), resultType,
                       out, tempAllocator)));
          lvalueStore(lhs, out,
                      translateCast(TEMP(temp, size, size, kind),
                                    exp->data.binOpExp.resultType, lhsType, out,
                                    tempAllocator),
                      tempAllocator);

          lvalueDtor(lhs);
          break;
        }
        case BO_MODASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *resultType = exp->data.binOpExp.resultType;
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(resultType);
          // kind isn't needed - this is integral only

          IROperator op;
          if (typeIsSignedIntegral(resultType)) {
            op = IO_SMOD;
          } else {
            op = IO_UMOD;
          }
          IR(out,
             BINOP(size, op, TEMP(temp, size, size, AH_GP),
                   translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                 resultType, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), resultType,
                       out, tempAllocator)));
          lvalueStore(lhs, out,
                      translateCast(TEMP(temp, size, size, AH_GP),
                                    exp->data.binOpExp.resultType, lhsType, out,
                                    tempAllocator),
                      tempAllocator);

          lvalueDtor(lhs);
          break;
        }
        case BO_ADDASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);

          if (typeIsPointer(lhsType)) {
            Type *dereferenced = typeGetDereferenced(lhsType);
            size_t temp = NEW(tempAllocator);
            size_t rhsValue = NEW(tempAllocator);

            // cast to uint64_t safe only if on <= 64 bit platform - caught by
            // static asserts
            IR(out,
               BINOP(POINTER_WIDTH, IO_SMUL,
                     TEMP(rhsValue, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                     translateCast(
                         translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                         frame, labelGenerator, tempAllocator),
                         expressionTypeof(exp->data.binOpExp.rhs), lhsType, out,
                         tempAllocator),
                     ULONG((uint64_t)typeSizeof(dereferenced))));
            IR(out, BINOP(POINTER_WIDTH, IO_ADD,
                          TEMP(temp, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                          lvalueLoad(lhs, out, tempAllocator),
                          TEMP(rhsValue, POINTER_WIDTH, POINTER_WIDTH, AH_GP)));

            typeDestroy(dereferenced);
          } else {
            Type const *resultType = exp->data.binOpExp.resultType;
            size_t temp = NEW(tempAllocator);
            size_t size = typeSizeof(resultType);
            AllocHint kind = typeKindof(resultType);
            IROperator op;

            if (typeIsFloat(resultType)) {
              op = IO_FP_ADD;
            } else {
              op = IO_ADD;
            }
            IR(out,
               BINOP(size, op, TEMP(temp, size, size, kind),
                     translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                   resultType, out, tempAllocator),
                     translateCast(
                         translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                         frame, labelGenerator, tempAllocator),
                         expressionTypeof(exp->data.binOpExp.rhs), resultType,
                         out, tempAllocator)));
            lvalueStore(lhs, out,
                        translateCast(TEMP(temp, size, size, kind),
                                      exp->data.binOpExp.resultType, lhsType,
                                      out, tempAllocator),
                        tempAllocator);
          }

          lvalueDtor(lhs);
          break;
        }
        case BO_SUBASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);

          if (typeIsPointer(lhsType)) {
            Type *dereferenced = typeGetDereferenced(lhsType);
            size_t temp = NEW(tempAllocator);
            size_t rhsValue = NEW(tempAllocator);

            // cast to uint64_t safe only if on <= 64 bit platform - caught by
            // static asserts
            IR(out,
               BINOP(POINTER_WIDTH, IO_SMUL,
                     TEMP(rhsValue, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                     translateCast(
                         translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                         frame, labelGenerator, tempAllocator),
                         expressionTypeof(exp->data.binOpExp.rhs), lhsType, out,
                         tempAllocator),
                     ULONG((uint64_t)typeSizeof(dereferenced))));
            IR(out, BINOP(POINTER_WIDTH, IO_SUB,
                          TEMP(temp, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                          lvalueLoad(lhs, out, tempAllocator),
                          TEMP(rhsValue, POINTER_WIDTH, POINTER_WIDTH, AH_GP)));

            typeDestroy(dereferenced);
          } else {
            Type const *resultType = exp->data.binOpExp.resultType;
            size_t temp = NEW(tempAllocator);
            size_t size = typeSizeof(resultType);
            AllocHint kind = typeKindof(resultType);
            IROperator op;

            if (typeIsFloat(resultType)) {
              op = IO_FP_SUB;
            } else {
              op = IO_SUB;
            }
            IR(out,
               BINOP(size, op, TEMP(temp, size, size, kind),
                     translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                   resultType, out, tempAllocator),
                     translateCast(
                         translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                         frame, labelGenerator, tempAllocator),
                         expressionTypeof(exp->data.binOpExp.rhs), resultType,
                         out, tempAllocator)));
            lvalueStore(lhs, out,
                        translateCast(TEMP(temp, size, size, kind),
                                      exp->data.binOpExp.resultType, lhsType,
                                      out, tempAllocator),
                        tempAllocator);
          }

          lvalueDtor(lhs);
          break;
        }
        case BO_LSHIFTASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type *byteType = keywordTypeCreate(K_UBYTE);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(expressionTypeof(exp->data.binOpExp.lhs));

          IR(out,
             BINOP(size, IO_SLL, TEMP(temp, size, size, AH_GP),
                   lvalueLoad(lhs, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), byteType, out,
                       tempAllocator)));
          lvalueStore(lhs, out, TEMP(temp, size, size, AH_GP), tempAllocator);

          typeDestroy(byteType);
          lvalueDtor(lhs);
          break;
        }
        case BO_LRSHIFTASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type *byteType = keywordTypeCreate(K_UBYTE);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(expressionTypeof(exp->data.binOpExp.lhs));

          IR(out,
             BINOP(size, IO_SLR, TEMP(temp, size, size, AH_GP),
                   lvalueLoad(lhs, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), byteType, out,
                       tempAllocator)));
          lvalueStore(lhs, out, TEMP(temp, size, size, AH_GP), tempAllocator);

          typeDestroy(byteType);
          lvalueDtor(lhs);
          break;
        }
        case BO_ARSHIFTASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type *byteType = keywordTypeCreate(K_UBYTE);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(expressionTypeof(exp->data.binOpExp.lhs));

          IR(out,
             BINOP(size, IO_SAR, TEMP(temp, size, size, AH_GP),
                   lvalueLoad(lhs, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), byteType, out,
                       tempAllocator)));
          lvalueStore(lhs, out, TEMP(temp, size, size, AH_GP), tempAllocator);

          typeDestroy(byteType);
          lvalueDtor(lhs);
          break;
        }
        case BO_BITANDASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *resultType = exp->data.binOpExp.resultType;
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(resultType);
          AllocHint kind = typeKindof(resultType);

          IR(out,
             BINOP(size, IO_AND, TEMP(temp, size, size, kind),
                   translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                 resultType, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), resultType,
                       out, tempAllocator)));
          lvalueStore(lhs, out,
                      translateCast(TEMP(temp, size, size, kind),
                                    exp->data.binOpExp.resultType, lhsType, out,
                                    tempAllocator),
                      tempAllocator);

          lvalueDtor(lhs);
          break;
        }
        case BO_BITXORASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *resultType = exp->data.binOpExp.resultType;
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(resultType);
          AllocHint kind = typeKindof(resultType);

          IR(out,
             BINOP(size, IO_XOR, TEMP(temp, size, size, kind),
                   translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                 resultType, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), resultType,
                       out, tempAllocator)));
          lvalueStore(lhs, out,
                      translateCast(TEMP(temp, size, size, kind),
                                    exp->data.binOpExp.resultType, lhsType, out,
                                    tempAllocator),
                      tempAllocator);

          lvalueDtor(lhs);
          break;
        }
        case BO_BITORASSIGN: {
          Lvalue *lhs = translateLvalue(exp->data.binOpExp.lhs, out, fragments,
                                        frame, labelGenerator, tempAllocator);
          Type const *resultType = exp->data.binOpExp.resultType;
          Type const *lhsType = expressionTypeof(exp->data.binOpExp.lhs);
          size_t temp = NEW(tempAllocator);
          size_t size = typeSizeof(resultType);
          AllocHint kind = typeKindof(resultType);

          IR(out,
             BINOP(size, IO_OR, TEMP(temp, size, size, kind),
                   translateCast(lvalueLoad(lhs, out, tempAllocator), lhsType,
                                 resultType, out, tempAllocator),
                   translateCast(
                       translateRvalue(exp->data.binOpExp.rhs, out, fragments,
                                       frame, labelGenerator, tempAllocator),
                       expressionTypeof(exp->data.binOpExp.rhs), resultType,
                       out, tempAllocator)));
          lvalueStore(lhs, out,
                      translateCast(TEMP(temp, size, size, kind),
                                    exp->data.binOpExp.resultType, lhsType, out,
                                    tempAllocator),
                      tempAllocator);

          lvalueDtor(lhs);
          break;
        }
        case BO_BITAND:
        case BO_BITOR:
        case BO_BITXOR:
        case BO_SPACESHIP:
        case BO_LSHIFT:
        case BO_LRSHIFT:
        case BO_ARSHIFT:
        case BO_ADD:
        case BO_SUB:
        case BO_MUL:
        case BO_DIV:
        case BO_MOD:
        case BO_ARRAYACCESS: {
          // these operations are side effect free
          translateVoiddedValue(exp->data.binOpExp.lhs, out, fragments, frame,
                                labelGenerator, tempAllocator);
          translateVoiddedValue(exp->data.binOpExp.rhs, out, fragments, frame,
                                labelGenerator, tempAllocator);
          break;
        }
        default: { error(__FILE__, __LINE__, "invalid BinOpType enum"); }
      }
      break;
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF:
        case UO_ADDROF: {
          // these operations are side effect free
          translateVoiddedValue(exp->data.unOpExp.target, out, fragments, frame,
                                labelGenerator, tempAllocator);
          break;
        }
        case UO_PREINC:
        case UO_POSTINC: {  // no value produced, so it's just an increment
          Lvalue *value =
              translateLvalue(exp->data.unOpExp.target, out, fragments, frame,
                              labelGenerator, tempAllocator);
          if (typeIsPointer(exp->data.unOpExp.resultType)) {
            // is pointer
            Type *dereferenced =
                typeGetDereferenced(exp->data.unOpExp.resultType);
            size_t temp = NEW(tempAllocator);
            // note - size_t to 64 bit conversion only safe on <= 64 bit
            // platforms - static assert catches that
            IR(out, BINOP(POINTER_WIDTH, IO_ADD,
                          TEMP(temp, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                          lvalueLoad(value, out, tempAllocator),
                          ULONG((size_t)typeSizeof(dereferenced))));
            lvalueStore(value, out,
                        TEMP(temp, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                        tempAllocator);
          } else if (typeIsIntegral(exp->data.unOpExp.resultType)) {
            // is integral
            size_t temp = NEW(tempAllocator);
            size_t size = typeSizeof(exp->data.unOpExp.resultType);
            IROperand *one = malloc(sizeof(IROperand));
            one->kind = OK_CONSTANT;
            one->data.constant.bits =
                0x1;  // constant one, unsized, sign-agnostic
            IR(out, BINOP(size, IO_ADD, TEMP(temp, size, size, AH_GP),
                          lvalueLoad(value, out, tempAllocator), one));
            lvalueStore(value, out, TEMP(temp, size, size, AH_GP),
                        tempAllocator);
          } else {
            // is float/double
            size_t temp = NEW(tempAllocator);
            size_t size = typeSizeof(exp->data.unOpExp.resultType);
            IROperand *one = size == FLOAT_WIDTH ? UINT(FLOAT_BITS_ONE)
                                                 : ULONG(DOUBLE_BITS_ONE);
            IR(out, BINOP(size, IO_FP_ADD, TEMP(temp, size, size, AH_SSE),
                          lvalueLoad(value, out, tempAllocator), one));
            lvalueStore(value, out, TEMP(temp, size, size, AH_SSE),
                        tempAllocator);
          }

          lvalueDtor(value);
          break;
        }
        case UO_PREDEC:
        case UO_POSTDEC: {  // no value produced, so it's just a decrement
          Lvalue *value =
              translateLvalue(exp->data.unOpExp.target, out, fragments, frame,
                              labelGenerator, tempAllocator);
          if (typeIsPointer(exp->data.unOpExp.resultType)) {
            // is pointer
            Type *dereferenced =
                typeGetDereferenced(exp->data.unOpExp.resultType);
            size_t temp = NEW(tempAllocator);
            // note - size_t to 64 bit conversion only safe on <= 64 bit
            // platforms - static assert catches that
            IR(out, BINOP(POINTER_WIDTH, IO_SUB,
                          TEMP(temp, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                          lvalueLoad(value, out, tempAllocator),
                          ULONG((size_t)typeSizeof(dereferenced))));
            lvalueStore(value, out,
                        TEMP(temp, POINTER_WIDTH, POINTER_WIDTH, AH_GP),
                        tempAllocator);
          } else if (typeIsIntegral(exp->data.unOpExp.resultType)) {
            // is integral
            size_t temp = NEW(tempAllocator);
            size_t size = typeSizeof(exp->data.unOpExp.resultType);
            IROperand *one = malloc(sizeof(IROperand));
            one->kind = OK_CONSTANT;
            one->data.constant.bits =
                0x1;  // constant one, unsized, sign-agnostic
            IR(out, BINOP(size, IO_SUB, TEMP(temp, size, size, AH_GP),
                          lvalueLoad(value, out, tempAllocator), one));
            lvalueStore(value, out, TEMP(temp, size, size, AH_GP),
                        tempAllocator);
          } else {
            // is float/double
            size_t temp = NEW(tempAllocator);
            size_t size = typeSizeof(exp->data.unOpExp.resultType);
            IROperand *one = size == FLOAT_WIDTH ? UINT(FLOAT_BITS_ONE)
                                                 : ULONG(DOUBLE_BITS_ONE);
            IR(out, BINOP(size, IO_FP_SUB, TEMP(temp, size, size, AH_SSE),
                          lvalueLoad(value, out, tempAllocator), one));
            lvalueStore(value, out, TEMP(temp, size, size, AH_SSE),
                        tempAllocator);
          }

          lvalueDtor(value);
          break;
        }
        case UO_NEG:
        case UO_LNOT:
        case UO_BITNOT: {
          // these operations are side effect free
          translateVoiddedValue(exp->data.unOpExp.target, out, fragments, frame,
                                labelGenerator, tempAllocator);
          break;
        }
        default: { error(__FILE__, __LINE__, "invalid UnOpType enum"); }
      }
      break;
    }
    case NT_COMPOPEXP: {
      // comparisons are side effect free
      translateVoiddedValue(exp->data.compOpExp.lhs, out, fragments, frame,
                            labelGenerator, tempAllocator);
      translateVoiddedValue(exp->data.compOpExp.rhs, out, fragments, frame,
                            labelGenerator, tempAllocator);
      break;
    }
    case NT_LANDASSIGNEXP: {
      // load lhs
      // if !lhs goto end
      // store rhs
      // end:
      Lvalue *lhs = translateLvalue(exp->data.landAssignExp.lhs, out, fragments,
                                    frame, labelGenerator, tempAllocator);
      char *end = NEW_LABEL(labelGenerator);
      IR(out, CJUMP(BYTE_WIDTH, IO_JE, strdup(end),
                    lvalueLoad(lhs, out, tempAllocator), UBYTE(0)));
      lvalueStore(lhs, out,
                  translateRvalue(exp->data.landAssignExp.rhs, out, fragments,
                                  frame, labelGenerator, tempAllocator),
                  tempAllocator);
      IR(out, LABEL(end));

      lvalueDtor(lhs);
      break;
    }
    case NT_LORASSIGNEXP: {
      // load lhs
      // if lhs goto end
      // store rhs
      // end:

      Lvalue *lhs = translateLvalue(exp->data.landAssignExp.lhs, out, fragments,
                                    frame, labelGenerator, tempAllocator);
      char *end = NEW_LABEL(labelGenerator);
      IR(out, CJUMP(BYTE_WIDTH, IO_JNE, strdup(end),
                    lvalueLoad(lhs, out, tempAllocator), UBYTE(0)));
      lvalueStore(lhs, out,
                  translateRvalue(exp->data.landAssignExp.rhs, out, fragments,
                                  frame, labelGenerator, tempAllocator),
                  tempAllocator);
      IR(out, LABEL(end));

      lvalueDtor(lhs);
      break;
    }
    case NT_TERNARYEXP: {
      // jump if not (condition) to elseCase
      // trueCase
      // jump to end
      // elseCase:
      // falseCase
      // end:

      char *elseCase = NEW_LABEL(labelGenerator);
      char *end = NEW_LABEL(labelGenerator);

      translateJumpIfNot(exp->data.ternaryExp.condition, out, fragments, frame,
                         labelGenerator, tempAllocator, elseCase);
      translateVoiddedValue(exp->data.ternaryExp.thenExp, out, fragments, frame,
                            labelGenerator, tempAllocator);
      IR(out, JUMP(strdup(end)));
      IR(out, LABEL(elseCase));
      translateVoiddedValue(exp->data.ternaryExp.elseExp, out, fragments, frame,
                            labelGenerator, tempAllocator);
      IR(out, LABEL(end));
      break;
    }
    case NT_LANDEXP: {
      // if lhs
      // rhs

      char *end = NEW_LABEL(labelGenerator);
      translateJumpIfNot(exp->data.landExp.lhs, out, fragments, frame,
                         labelGenerator, tempAllocator, end);
      translateVoiddedValue(exp->data.landExp.rhs, out, fragments, frame,
                            labelGenerator, tempAllocator);
      IR(out, LABEL(end));
      break;
    }
    case NT_LOREXP: {
      // if !lhs
      // rhs

      char *end = NEW_LABEL(labelGenerator);
      translateJumpIf(exp->data.lorExp.lhs, out, fragments, frame,
                      labelGenerator, tempAllocator, end);
      translateVoiddedValue(exp->data.lorExp.rhs, out, fragments, frame,
                            labelGenerator, tempAllocator);
      IR(out, LABEL(end));
      break;
    }
    case NT_STRUCTACCESSEXP: {
      translateVoiddedValue(exp->data.structAccessExp.base, out, fragments,
                            frame, labelGenerator, tempAllocator);
      break;
    }
    case NT_STRUCTPTRACCESSEXP: {
      translateVoiddedValue(exp->data.structPtrAccessExp.base, out, fragments,
                            frame, labelGenerator, tempAllocator);
      break;
    }
    case NT_FNCALLEXP: {
      // if who is a function id, then do a direct call.
      // otherwise, do an indirect call.
      IROperand *result;

      Node *who = exp->data.fnCallExp.who;
      if (who->type == NT_ID && who->data.id.symbol->kind == SK_FUNCTION) {
        // direct call - is call <name>, with default args
        OverloadSetElement *elm = who->data.id.overload;
        SymbolInfo *info = who->data.id.symbol;
        IROperandVector *actualArgs = irOperandVectorCreate();
        // get args and default args
        NodeList *args = exp->data.fnCallExp.args;
        size_t idx = 0;
        for (; idx < args->size; idx++) {
          // args
          irOperandVectorInsert(
              actualArgs,
              translateCast(
                  translateRvalue(args->elements[idx], out, fragments, frame,
                                  labelGenerator, tempAllocator),
                  expressionTypeof(args->elements[idx]),
                  elm->argumentTypes.elements[idx], out, tempAllocator));
        }
        size_t numRequired = elm->argumentTypes.size - elm->numOptional;
        for (; idx < elm->argumentTypes.size; idx++) {
          // default args
          irOperandVectorInsert(
              actualArgs,
              irOperandCopy(elm->defaultArgs.elements[idx - numRequired]));
        }
        result =
            frameDirectCall(frame,
                            mangleFunctionName(info->module, who->data.id.id,
                                               &elm->argumentTypes),
                            args, elm, out, tempAllocator);
      } else {
        // indirect call - is call *<temp>, with no default args
        Type const *functionType = expressionTypeof(who);
        IROperand *function = translateRvalue(who, out, fragments, frame,
                                              labelGenerator, tempAllocator);
        IROperandVector *actualArgs = irOperandVectorCreate();
        // get args
        NodeList *args = exp->data.fnCallExp.args;
        for (size_t idx = 0; idx < args->size; idx++) {
          irOperandVectorInsert(
              actualArgs,
              translateCast(
                  translateRvalue(args->elements[idx], out, fragments, frame,
                                  labelGenerator, tempAllocator),
                  expressionTypeof(args->elements[idx]),
                  functionType->data.functionPtr.argumentTypes->elements[idx],
                  out, tempAllocator));
        }
        result = frameIndirectCall(frame, function, actualArgs, functionType,
                                   out, tempAllocator);
      }

      if (result != NULL) {
        irOperandDestroy(result);
      }

      break;
    }
    case NT_CONSTEXP:
    case NT_AGGREGATEINITEXP:
    case NT_SIZEOFTYPEEXP: {
      break;  // constants are side effect free
              // note - constants, aggregate initializers, and sizeof(type) are
              // all considered constants
    }
    case NT_CASTEXP: {
      // casts are side effect free
      translateVoiddedValue(exp->data.castExp.target, out, fragments, frame,
                            labelGenerator, tempAllocator);
      break;
    }
    case NT_SIZEOFEXPEXP: {
      translateVoiddedValue(exp->data.sizeofExpExp.target, out, fragments,
                            frame, labelGenerator, tempAllocator);
      break;
    }
    case NT_ID: {
      // value accesses are side effect free
      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered a non-expression in an expression position");
    }
  }
}
static Lvalue *translateLvalue(Node *exp, IREntryVector *out,
                               FragmentVector *fragments, Frame *frame,
                               LabelGenerator *labelGenerator,
                               TempAllocator *tempAllocator) {
  switch (exp->type) {
    case NT_SEQEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_ASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MULASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_DIVASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MODASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ADDASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_SUBASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LSHIFTASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LRSHIFTASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ARSHIFTASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITANDASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITXORASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITORASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITAND: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITOR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITXOR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_SPACESHIP: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LSHIFT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LRSHIFT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ARSHIFT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ADD: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_SUB: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MUL: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_DIV: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MOD: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ARRAYACCESS: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid BinOpType enum"); }
      }
      break;
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_ADDROF: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_PREINC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_PREDEC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_NEG: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_LNOT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_BITNOT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_POSTINC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_POSTDEC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid UnOpType enum"); }
      }
    }
    case NT_COMPOPEXP: {
      switch (exp->data.compOpExp.op) {
        case CO_EQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_NEQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_LT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_GT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_LTEQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_GTEQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid CompOpType enum"); }
      }
    }
    case NT_LANDASSIGNEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_LORASSIGNEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_TERNARYEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_LANDEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_LOREXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_STRUCTACCESSEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_STRUCTPTRACCESSEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_FNCALLEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_CONSTEXP: {
      switch (exp->data.constExp.type) {
        case CT_UBYTE: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_BYTE: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_CHAR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_USHORT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_SHORT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_UINT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_INT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_WCHAR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_ULONG: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_LONG: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_FLOAT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_DOUBLE: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_BOOL: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_STRING: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_WSTRING: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_NULL: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid ConstType enum constant");
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_CASTEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_SIZEOFTYPEEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_SIZEOFEXPEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_ID: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered a non-expression in an expression position");
    }
  }
}
static IROperand *translateRvalue(Node *exp, IREntryVector *out,
                                  FragmentVector *fragments, Frame *frame,
                                  LabelGenerator *labelGenerator,
                                  TempAllocator *tempAllocator) {
  switch (exp->type) {
    case NT_SEQEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_ASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MULASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_DIVASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MODASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ADDASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_SUBASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LSHIFTASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LRSHIFTASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ARSHIFTASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITANDASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITXORASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITORASSIGN: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITAND: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITOR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_BITXOR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_SPACESHIP: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LSHIFT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_LRSHIFT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ARSHIFT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ADD: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_SUB: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MUL: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_DIV: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_MOD: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case BO_ARRAYACCESS: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid BinOpType enum"); }
      }
      break;
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_ADDROF: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_PREINC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_PREDEC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_NEG: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_LNOT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_BITNOT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_POSTINC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case UO_POSTDEC: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid UnOpType enum"); }
      }
    }
    case NT_COMPOPEXP: {
      switch (exp->data.compOpExp.op) {
        case CO_EQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_NEQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_LT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_GT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_LTEQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CO_GTEQ: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid CompOpType enum"); }
      }
    }
    case NT_LANDASSIGNEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_LORASSIGNEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_TERNARYEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_LANDEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_LOREXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_STRUCTACCESSEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_STRUCTPTRACCESSEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_FNCALLEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_CONSTEXP: {
      switch (exp->data.constExp.type) {
        case CT_UBYTE: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_BYTE: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_CHAR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_USHORT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_SHORT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_UINT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_INT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_WCHAR: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_ULONG: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_LONG: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_FLOAT: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_DOUBLE: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_BOOL: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_STRING: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_WSTRING: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        case CT_NULL: {
          error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
        }
        default: {
          error(__FILE__, __LINE__,
                "encountered an invalid ConstType enum constant");
        }
      }
    }
    case NT_AGGREGATEINITEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_CASTEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_SIZEOFTYPEEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_SIZEOFEXPEXP: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    case NT_ID: {
      error(__FILE__, __LINE__, "Not yet implemented");  // TODO: write this
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered a non-expression in an expression position");
    }
  }
}

// translation - branching
static void translateJumpIfNot(Node *condition, IREntryVector *out,
                               FragmentVector *fragments, Frame *frame,
                               LabelGenerator *labelGenerator,
                               TempAllocator *tempAllocator,
                               char const *target) {
  error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
}
static void translateJumpIf(Node *condition, IREntryVector *out,
                            FragmentVector *fragments, Frame *frame,
                            LabelGenerator *labelGenerator,
                            TempAllocator *tempAllocator, char const *target) {
  error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
}

// translation - statements
static IREntryVector *translateStmt(
    Node *stmt, IREntryVector *out, FragmentVector *fragments, Frame *frame,
    Access *outArg, char const *breakLabel, char const *continueLabel,
    char const *exitLabel, LabelGenerator *labelGenerator,
    TempAllocator *tempAllocator, Type const *returnType) {
  if (stmt == NULL) {
    return out;
  }

  switch (stmt->type) {
    case NT_COMPOUNDSTMT: {
      frameScopeStart(frame);

      IREntryVector *body = irEntryVectorCreate();
      NodeList *stmts = stmt->data.compoundStmt.statements;
      for (size_t idx = 0; idx < stmts->size; idx++) {
        body = translateStmt(stmts->elements[idx], body, fragments, frame,
                             outArg, breakLabel, continueLabel, exitLabel,
                             labelGenerator, tempAllocator, returnType);
      }

      return irEntryVectorMerge(out, frameScopeEnd(frame, body, tempAllocator));
    }
    case NT_IFSTMT: {
      if (stmt->data.ifStmt.elseStmt == NULL) {
        // jump if not (condition) to end
        // thenBody
        // end:
        char *skip = NEW_LABEL(labelGenerator);

        translateJumpIfNot(stmt->data.ifStmt.condition, out, fragments, frame,
                           labelGenerator, tempAllocator, skip);
        translateStmt(stmt->data.ifStmt.thenStmt, out, fragments, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempAllocator, returnType);
        IR(out, LABEL(skip));
      } else {
        // jump if not (condition) to elseCase
        // thenBody
        // jump to end
        // elseCase:
        // elseBody
        // end:
        char *elseCase = NEW_LABEL(labelGenerator);
        char *end = NEW_LABEL(labelGenerator);

        translateJumpIfNot(stmt->data.ifStmt.condition, out, fragments, frame,
                           labelGenerator, tempAllocator, elseCase);
        translateStmt(stmt->data.ifStmt.thenStmt, out, fragments, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempAllocator, returnType);
        IR(out, JUMP(strdup(end)));
        IR(out, LABEL(elseCase));
        translateStmt(stmt->data.ifStmt.elseStmt, out, fragments, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempAllocator, returnType);
        IR(out, LABEL(end));
      }
      return out;
    }
    case NT_WHILESTMT: {
      // start:
      // jump if not (condition) to end
      // body
      // jump to start
      // end:
      char *start = NEW_LABEL(labelGenerator);
      char *end = NEW_LABEL(labelGenerator);

      IR(out, LABEL(strdup(start)));
      translateJumpIfNot(stmt->data.whileStmt.condition, out, fragments, frame,
                         labelGenerator, tempAllocator, end);
      translateStmt(stmt->data.whileStmt.body, out, fragments, frame, outArg,
                    end, start, exitLabel, labelGenerator, tempAllocator,
                    returnType);
      IR(out, JUMP(start));
      IR(out, LABEL(end));
      return out;
    }
    case NT_DOWHILESTMT: {
      // start:
      // body
      // continue:
      // jump if (condition) to start
      // end:
      char *start = NEW_LABEL(labelGenerator);
      char *loopContinue = NEW_LABEL(labelGenerator);
      char *end = NEW_LABEL(labelGenerator);

      IR(out, LABEL(start));
      translateStmt(stmt->data.doWhileStmt.body, out, fragments, frame, outArg,
                    end, loopContinue, exitLabel, labelGenerator, tempAllocator,
                    returnType);
      IR(out, LABEL(loopContinue));
      translateJumpIf(stmt->data.doWhileStmt.condition, out, fragments, frame,
                      labelGenerator, tempAllocator, start);
      IR(out, LABEL(end));
      return out;
    }
    case NT_FORSTMT: {
      // {
      //  initialize
      //  start:
      //  jump if not (condition) to end
      //  body
      //  update
      //  jump to start
      //  end:
      // }
      IREntryVector *body = irEntryVectorCreate();
      frameScopeStart(frame);

      char *start = NEW_LABEL(labelGenerator);
      char *end = NEW_LABEL(labelGenerator);

      Node *initialize = stmt->data.forStmt.initialize;
      if (initialize != NULL) {
        if (initialize->type == NT_VARDECL) {
          translateStmt(stmt->data.forStmt.initialize, body, fragments, frame,
                        outArg, breakLabel, continueLabel, exitLabel,
                        labelGenerator, tempAllocator, returnType);
        } else {
          translateVoiddedValue(initialize, body, fragments, frame,
                                labelGenerator, tempAllocator);
        }
      }

      IR(body, LABEL(strdup(start)));
      translateJumpIfNot(stmt->data.forStmt.condition, body, fragments, frame,
                         labelGenerator, tempAllocator, end);
      translateStmt(stmt->data.forStmt.body, body, fragments, frame, outArg,
                    end, start, exitLabel, labelGenerator, tempAllocator,
                    returnType);
      translateVoiddedValue(stmt->data.forStmt.update, body, fragments, frame,
                            labelGenerator, tempAllocator);
      IR(body, JUMP(start));
      IR(body, LABEL(end));

      return irEntryVectorMerge(out, frameScopeEnd(frame, body, tempAllocator));
    }
    case NT_SWITCHSTMT: {
      error(__FILE__, __LINE__, "not yet implemented!");  // TODO: write this
      return out;
    }
    case NT_BREAKSTMT: {
      IR(out, JUMP(strdup(breakLabel)));
      return out;
    }
    case NT_CONTINUESTMT: {
      IR(out, JUMP(strdup(continueLabel)));
      return out;
    }
    case NT_RETURNSTMT: {
      if (stmt->data.returnStmt.value != NULL) {
        accessStore(
            outArg, out,
            translateCast(
                translateRvalue(stmt->data.returnStmt.value, out, fragments,
                                frame, labelGenerator, tempAllocator),
                expressionTypeof(stmt->data.returnStmt.value), returnType, out,
                tempAllocator),
            tempAllocator);
      }
      IR(out, JUMP(strdup(exitLabel)));
      return out;
    }
    case NT_ASMSTMT: {
      IR(out, ASM(strdup(stmt->data.asmStmt.assembly)));
      return out;
    }
    case NT_EXPRESSIONSTMT: {
      translateVoiddedValue(stmt->data.expressionStmt.expression, out,
                            fragments, frame, labelGenerator, tempAllocator);
      return out;
    }
    case NT_NULLSTMT:
    case NT_STRUCTDECL:
    case NT_STRUCTFORWARDDECL:
    case NT_UNIONDECL:
    case NT_UNIONFORWARDDECL:
    case NT_ENUMDECL:
    case NT_ENUMFORWARDDECL:
    case NT_TYPEDEFDECL: {
      // semantics only - no generated code
      return out;
    }
    case NT_VARDECL: {
      NodePairList *idValuePairs = stmt->data.varDecl.idValuePairs;
      for (size_t idx = 0; idx < idValuePairs->size; idx++) {
        Node *id = idValuePairs->firstElements[idx];
        Node *initializer = idValuePairs->secondElements[idx];

        SymbolInfo *info = id->data.id.symbol;
        Access *access = info->data.var.access = frameAllocLocal(
            frame, info->data.var.type, info->data.var.escapes, tempAllocator);

        if (initializer != NULL) {
          accessStore(
              access, out,
              translateCast(translateRvalue(initializer, out, fragments, frame,
                                            labelGenerator, tempAllocator),
                            expressionTypeof(initializer), info->data.var.type,
                            out, tempAllocator),
              tempAllocator);
        }
      }
      return out;
    }
    default: {
      error(__FILE__, __LINE__,
            "bad syntax past parse phase - encountered non-statement in "
            "statement position");
    }
  }
}

// translation - top level
static void translateGlobalVar(Node *varDecl, FragmentVector *fragments,
                               char const *moduleName,
                               LabelGenerator *labelGenerator) {
  NodePairList *idValuePairs = varDecl->data.varDecl.idValuePairs;
  for (size_t idx = 0; idx < idValuePairs->size; idx++) {
    Node *id = idValuePairs->firstElements[idx];
    Node *initializer = idValuePairs->secondElements[idx];
    char *mangledName = accessGetLabel(id->data.id.symbol->data.var.access);
    Fragment *f;
    if (initializer == NULL || constantIsZero(initializer)) {
      Type const *type = id->data.id.symbol->data.var.type;
      f = bssFragmentCreate(mangledName, typeSizeof(type), typeAlignof(type));
    } else if (id->data.id.symbol->data.var.type->kind == K_CONST) {
      f = rodataFragmentCreate(mangledName,
                               typeAlignof(id->data.id.symbol->data.var.type));
      constantToData(initializer, f->data.rodata.ir, fragments, labelGenerator);
    } else {
      f = dataFragmentCreate(mangledName,
                             typeAlignof(id->data.id.symbol->data.var.type));
      constantToData(initializer, f->data.data.ir, fragments, labelGenerator);
    }

    fragmentVectorInsert(fragments, f);
  }

  return;
}
static void translateFunction(Node *function, FragmentVector *fragments,
                              char const *moduleName, FrameCtor frameCtor,
                              LabelGenerator *labelGenerator) {
  // get function information
  Access *functionAccess = function->data.function.id->data.id.overload->access;
  char *mangledName = accessGetLabel(functionAccess);
  Type const *returnType =
      function->data.function.id->data.id.overload->returnType;

  Frame *frame = frameCtor(strdup(mangledName));
  TempAllocator *allocator = tempAllocatorCreate();
  Fragment *f = textFragmentCreate(mangledName, frame, allocator);

  // allocate function accesses
  for (size_t idx = 0; idx < function->data.function.formals->size; idx++) {
    Node *id = function->data.function.formals->secondElements[idx];
    SymbolInfo *info = id->data.id.symbol;
    info->data.var.access = frameAllocArg(frame, info->data.var.type,
                                          info->data.var.escapes, allocator);
  }

  Access *outArg = returnType->kind == K_VOID
                       ? NULL
                       : frameAllocRetVal(frame, returnType, allocator);

  char *exitLabel = NEW_LABEL(labelGenerator);
  NodeList *statements =
      function->data.function.body->data.compoundStmt.statements;
  for (size_t idx = 0; idx < statements->size; idx++) {
    f->data.text.ir = translateStmt(
        statements->elements[idx], f->data.text.ir, fragments, frame, outArg,
        NULL, NULL, exitLabel, labelGenerator, allocator, returnType);
  }
  IR(f->data.text.ir, LABEL(exitLabel));

  if (outArg != NULL) {
    accessDtor(outArg);
  }

  fragmentVectorInsert(fragments, f);
  return;
}
static void translateFile(Node *file, FragmentVector *fragments,
                          FrameCtor frameCtor, LabelGenerator *labelGenerator) {
  NodeList *bodies = file->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; idx++) {
    Node *body = bodies->elements[idx];
    char const *moduleName = file->data.module.id->data.id.id;
    switch (body->type) {
      case NT_VARDECL: {
        translateGlobalVar(body, fragments, moduleName, labelGenerator);
        return;
      }
      case NT_FUNCTION: {
        translateFunction(body, fragments, moduleName, frameCtor,
                          labelGenerator);
        return;
      }
      default: { return; }
    }
  }
}

void translate(FileIRFileMap *fileMap, ModuleAstMapPair *asts,
               LabelGeneratorCtor labelGeneratorCtor, FrameCtor frameCtor,
               GlobalAccessCtor globalAccessCtor,
               FunctionAccessCtor functionAccessCtor) {
  fileIRFileMapInit(fileMap);

  for (size_t idx = 0; idx < asts->decls.capacity; idx++) {
    if (asts->decls.keys[idx] != NULL) {
      Node *file = asts->decls.values[idx];
      addGlobalAccesses(file->data.file.symbols,
                        file->data.file.module->data.module.id->data.id.id,
                        globalAccessCtor, functionAccessCtor);
    }
  }
  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      addGlobalAccesses(file->data.file.symbols,
                        file->data.file.module->data.module.id->data.id.id,
                        globalAccessCtor, functionAccessCtor);
      char *filename = codeFilenameToAssembyFilename(file->data.file.filename);
      IRFile *irFile = irFileCreate(filename, labelGeneratorCtor());
      addDefaultArgs(file, &irFile->fragments, irFile->labelGenerator);
      fileIRFileMapPut(fileMap, filename, irFile);
    }
  }

  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      char *filename = codeFilenameToAssembyFilename(file->data.file.filename);
      IRFile *irFile = fileIRFileMapGet(fileMap, filename);
      translateFile(file, &irFile->fragments, frameCtor,
                    irFile->labelGenerator);
      free(filename);
    }
  }
}