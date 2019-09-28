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
Fragment *bssFragmentCreate(char *label, size_t size) {
  Fragment *f = fragmentCreate(FK_BSS, label);
  f->data.bss.size = size;
  return f;
}
Fragment *rodataFragmentCreate(char *label) {
  Fragment *f = fragmentCreate(FK_RODATA, label);
  f->data.rodata.ir = irVectorCreate();
  return f;
}
Fragment *dataFragmentCreate(char *label) {
  Fragment *f = fragmentCreate(FK_DATA, label);
  f->data.data.ir = irVectorCreate();
  return f;
}
Fragment *textFragmentCreate(char *label, Frame *frame) {
  Fragment *f = fragmentCreate(FK_TEXT, label);
  f->data.text.frame = frame;
  f->data.text.ir = irVectorCreate();
  return f;
}
void fragmentDestroy(Fragment *f) {
  switch (f->kind) {
    case FK_BSS: {
      break;
    }
    case FK_RODATA: {
      fragmentVectorDestroy(f->data.rodata.ir);
      break;
    }
    case FK_DATA: {
      fragmentVectorDestroy(f->data.data.ir);
      break;
    }
    case FK_TEXT: {
      fragmentVectorDestroy(f->data.text.ir);
      break;
    }
  }
  free(f->label);
  free(f);
}

// fragment vector
FragmentVector *fragmentVectorCreate(void) { return vectorCreate(); }
void fragmentVectorInsert(FragmentVector *v, Fragment *f) {
  vectorInsert(v, f);
}
void fragmentVectorDestroy(FragmentVector *v) {
  vectorDestroy(v, (void (*)(void *))fragmentDestroy);
}

void fileFragmentVectorMapInit(FileFragmentVectorMap *map) { hashMapInit(map); }
FragmentVector *fileFragmentVectorMapGet(FileFragmentVectorMap *map,
                                         char const *key) {
  return hashMapGet(map, key);
}
int fileFragmentVectorMapPut(FileFragmentVectorMap *map, char const *key,
                             FragmentVector *vector) {
  return hashMapPut(map, key, vector, (void (*)(void *))fragmentVectorDestroy);
}
void fileFragmentVectorMapUninit(FileFragmentVectorMap *map) {
  hashMapUninit(map, (void (*)(void *))fragmentVectorDestroy);
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
    case K_ARRAY: {
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
static Type const *typeofExpression(Node const *exp) {
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
static void constantToData(Node *initializer, IRVector *out,
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
          Fragment *f = rodataFragmentCreate(NEW_DATA_LABEL(labelGenerator));
          IR(f->data.rodata.ir,
             CONST(0, STRING(tstrdup(
                          initializer->data.constExp.value.stringVal))));
          fragmentVectorInsert(fragments, f);
          IR(out, CONST(POINTER_WIDTH, NAME(strdup(f->label))));
          return;
        }
        case CT_WSTRING: {
          Fragment *f = rodataFragmentCreate(NEW_DATA_LABEL(labelGenerator));
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

// translation - branching
static void translateJumpIfNot(Node *condition, IRVector *out,
                               LabelGenerator *labelGenerator,
                               TempAllocator *tempAllocator,
                               char const *target) {
  // TODO: write this
}
static void translateJumpIf(Node *condition, IRVector *out,
                            LabelGenerator *labelGenerator,
                            TempAllocator *tempAllocator, char const *target) {
  // TODO: write this
}

// translation - expressions
static IROperand *translateCast(IROperand *from, Type const *fromType,
                                Type const *toType, IRVector *out,
                                TempAllocator *tempAllocator) {
  return from;  // TODO: write this
}
static IROperand *translateExpression(Node *exp, IRVector *out,
                                      LabelGenerator *labelGenerator,
                                      TempAllocator *tempAllocator) {
  switch (exp->type) {
    case NT_SEQEXP: {
      irOperandDestroy(translateExpression(exp->data.seqExp.prefix, out,
                                           labelGenerator, tempAllocator));
      return translateExpression(exp->data.seqExp.last, out, labelGenerator,
                                 tempAllocator);
    }
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_ASSIGN: {
          // TODO: write this
        }
        case BO_MULASSIGN: {
          // TODO: write this
        }
        case BO_DIVASSIGN: {
          // TODO: write this
        }
        case BO_MODASSIGN: {
          // TODO: write this
        }
        case BO_ADDASSIGN: {
          // TODO: write this
        }
        case BO_SUBASSIGN: {
          // TODO: write this
        }
        case BO_LSHIFTASSIGN: {
          // TODO: write this
        }
        case BO_LRSHIFTASSIGN: {
          // TODO: write this
        }
        case BO_ARSHIFTASSIGN: {
          // TODO: write this
        }
        case BO_BITANDASSIGN: {
          // TODO: write this
        }
        case BO_BITXORASSIGN: {
          // TODO: write this
        }
        case BO_BITORASSIGN: {
          // TODO: write this
        }
        case BO_BITAND: {
          // TODO: write this
        }
        case BO_BITOR: {
          // TODO: write this
        }
        case BO_BITXOR: {
          // TODO: write this
        }
        case BO_SPACESHIP: {
          // TODO: write this
        }
        case BO_LSHIFT: {
          // TODO: write this
        }
        case BO_LRSHIFT: {
          // TODO: write this
        }
        case BO_ARSHIFT: {
          // TODO: write this
        }
        case BO_ADD: {
          // TODO: write this
        }
        case BO_SUB: {
          // TODO: write this
        }
        case BO_MUL: {
          // TODO: write this
        }
        case BO_DIV: {
          // TODO: write this
        }
        case BO_MOD: {
          // TODO: write this
        }
        case BO_ARRAYACCESS: {
          // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid BinOpType enum"); }
      }
    }
    case NT_UNOPEXP: {
      switch (exp->data.unOpExp.op) {
        case UO_DEREF: {
          Type const *resultType = exp->data.unOpExp.resultType;
          size_t resultSize = typeSizeof(resultType);
          size_t resultAlignment = typeAlignof(resultType);
          AllocHint kind = typeKindof(resultType);
          size_t temp = NEW(tempAllocator);
          IR(out,
             MEM_LOAD(resultSize, TEMP(temp, resultSize, resultAlignment, kind),
                      translateExpression(exp->data.unOpExp.target, out,
                                          labelGenerator, tempAllocator)));
          return TEMP(temp, resultSize, resultAlignment, kind);
        }
        case UO_ADDROF: {
          // TODO: write this
        }
        case UO_PREINC: {
          // TODO: write this
        }
        case UO_PREDEC: {
          // TODO: write this
        }
        case UO_NEG: {
          // TODO: write this
        }
        case UO_LNOT: {
          // TODO: write this
        }
        case UO_BITNOT: {
          // TODO: write this
        }
        case UO_POSTINC: {
          // TODO: write this
        }
        case UO_POSTDEC: {
          // TODO: write this
        }
        default: { error(__FILE__, __LINE__, "invalid UnOpType enum"); }
      }
    }
    case NT_COMPOPEXP: {
      // TODO: write this
    }
    case NT_LANDASSIGNEXP: {
      // TODO: write this
    }
    case NT_LORASSIGNEXP: {
      // TODO: write this
    }
    case NT_TERNARYEXP: {
      // var x
      // jump if not (condition) to elseCase
      // x = trueCase
      // jump to end
      // elseCase:
      // x = falseCase
      // end:

      size_t resultTemp = NEW(tempAllocator);
      Type const *resultType = exp->data.ternaryExp.resultType;
      size_t resultSize = typeSizeof(resultType);
      size_t resultAlignment = typeAlignof(resultType);
      AllocHint kind = typeKindof(resultType);

      char *elseCase = NEW_LABEL(labelGenerator);
      char *end = NEW_LABEL(labelGenerator);

      translateJumpIfNot(exp->data.ternaryExp.condition, out, labelGenerator,
                         tempAllocator, elseCase);
      IR(out,
         MOVE(resultSize, TEMP(resultTemp, resultSize, resultAlignment, kind),
              translateCast(
                  translateExpression(exp->data.ternaryExp.thenExp, out,
                                      labelGenerator, tempAllocator),
                  typeofExpression(exp->data.ternaryExp.thenExp), resultType,
                  out, tempAllocator)));
      IR(out, JUMP(strdup(end)));
      IR(out, LABEL(elseCase));
      IR(out,
         MOVE(resultSize, TEMP(resultTemp, resultSize, resultAlignment, kind),
              translateCast(
                  translateExpression(exp->data.ternaryExp.elseExp, out,
                                      labelGenerator, tempAllocator),
                  typeofExpression(exp->data.ternaryExp.elseExp), resultType,
                  out, tempAllocator)));
      IR(out, LABEL(end));
      return TEMP(resultTemp, resultSize, resultAlignment, kind);
    }
    case NT_LANDEXP: {
      // TODO: write this
    }
    case NT_LOREXP: {
      // TODO: write this
    }
    case NT_STRUCTACCESSEXP: {
      // TODO: write this
    }
    case NT_STRUCTPTRACCESSEXP: {
      // TODO: write this
    }
    case NT_FNCALLEXP: {
      // TODO: write this
    }
    case NT_CONSTEXP: {
      // TODO: write this
    }
    case NT_AGGREGATEINITEXP: {
      // TODO: write this
    }
    case NT_CASTEXP: {
      // TODO: write this
    }
    case NT_SIZEOFTYPEEXP: {
      // TODO: write this
    }
    case NT_SIZEOFEXPEXP: {
      // TODO: write this
    }
    case NT_ID: {
      // TODO: write this
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered a non-expression in an expression position");
    }
  }
}

// translation - statements
static void translateStmt(Node *stmt, IRVector *out, Frame *frame,
                          Access *outArg, char const *breakLabel,
                          char const *continueLabel, char const *exitLabel,
                          LabelGenerator *labelGenerator,
                          TempAllocator *tempAllocator,
                          Type const *returnType) {
  if (stmt == NULL) {
    return;
  }

  switch (stmt->type) {
    case NT_COMPOUNDSTMT: {
      NodeList *stmts = stmt->data.compoundStmt.statements;
      for (size_t idx = 0; idx < stmts->size; idx++) {
        translateStmt(stmts->elements[idx], out, frame, outArg, breakLabel,
                      continueLabel, exitLabel, labelGenerator, tempAllocator,
                      returnType);
      }
      break;
    }
    case NT_IFSTMT: {
      if (stmt->data.ifStmt.elseStmt == NULL) {
        // jump if not (condition) to end
        // thenBody
        // end:
        char *skip = labelGenerator->vtable->generateCodeLabel(labelGenerator);

        translateJumpIfNot(stmt->data.ifStmt.condition, out, labelGenerator,
                           tempAllocator, skip);
        translateStmt(stmt->data.ifStmt.thenStmt, out, frame, outArg,
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

        translateJumpIfNot(stmt->data.ifStmt.condition, out, labelGenerator,
                           tempAllocator, elseCase);
        translateStmt(stmt->data.ifStmt.thenStmt, out, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempAllocator, returnType);
        IR(out, JUMP(strdup(end)));
        IR(out, LABEL(elseCase));
        translateStmt(stmt->data.ifStmt.elseStmt, out, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempAllocator, returnType);
        IR(out, LABEL(end));
      }
      break;
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
      translateJumpIfNot(stmt->data.whileStmt.condition, out, labelGenerator,
                         tempAllocator, end);
      translateStmt(stmt->data.whileStmt.body, out, frame, outArg, end, start,
                    exitLabel, labelGenerator, tempAllocator, returnType);
      IR(out, JUMP(start));
      IR(out, LABEL(end));
      break;
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
      translateStmt(stmt->data.doWhileStmt.body, out, frame, outArg, end,
                    loopContinue, exitLabel, labelGenerator, tempAllocator,
                    returnType);
      IR(out, LABEL(loopContinue));
      translateJumpIf(stmt->data.doWhileStmt.condition, out, labelGenerator,
                      tempAllocator, start);
      IR(out, LABEL(end));
      break;
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
      IRVector *body = irVectorCreate();
      frame->vtable->scopeStart(frame);

      char *start = NEW_LABEL(labelGenerator);
      char *end = NEW_LABEL(labelGenerator);

      Node *initialize = stmt->data.forStmt.initialize;
      if (initialize != NULL) {
        if (initialize->type == NT_VARDECL) {
          translateStmt(stmt->data.forStmt.initialize, body, frame, outArg,
                        breakLabel, continueLabel, exitLabel, labelGenerator,
                        tempAllocator, returnType);
        } else {
          irOperandDestroy(translateExpression(initialize, body, labelGenerator,
                                               tempAllocator));
        }
      }

      IR(body, LABEL(strdup(start)));
      translateJumpIfNot(stmt->data.forStmt.condition, body, labelGenerator,
                         tempAllocator, end);
      translateStmt(stmt->data.forStmt.body, body, frame, outArg, end, start,
                    exitLabel, labelGenerator, tempAllocator, returnType);
      irOperandDestroy(translateExpression(stmt->data.forStmt.update, body,
                                           labelGenerator, tempAllocator));
      IR(body, JUMP(start));
      IR(body, LABEL(end));

      out = irVectorMerge(out,
                          frame->vtable->scopeEnd(frame, body, tempAllocator));
      break;
    }
    case NT_SWITCHSTMT: {
      // TODO: write this
      break;
    }
    case NT_BREAKSTMT: {
      IR(out, JUMP(strdup(breakLabel)));
      break;
    }
    case NT_CONTINUESTMT: {
      IR(out, JUMP(strdup(continueLabel)));
      break;
    }
    case NT_RETURNSTMT: {
      if (stmt->data.returnStmt.value != NULL) {
        outArg->vtable->store(
            outArg, out,
            translateCast(translateExpression(stmt->data.returnStmt.value, out,
                                              labelGenerator, tempAllocator),
                          typeofExpression(stmt->data.returnStmt.value),
                          returnType, out, tempAllocator),
            tempAllocator);
      }
      IR(out, JUMP(strdup(exitLabel)));
      break;
    }
    case NT_ASMSTMT: {
      IR(out, ASM(strdup(stmt->data.asmStmt.assembly)));
      break;
    }
    case NT_EXPRESSIONSTMT: {
      irOperandDestroy(translateExpression(stmt->data.expressionStmt.expression,
                                           out, labelGenerator, tempAllocator));
      break;
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
      break;
    }
    case NT_VARDECL: {
      NodePairList *idValuePairs = stmt->data.varDecl.idValuePairs;
      for (size_t idx = 0; idx < idValuePairs->size; idx++) {
        Node *id = idValuePairs->firstElements[idx];
        Node *initializer = idValuePairs->secondElements[idx];

        SymbolInfo *info = id->data.id.symbol;
        Access *access = info->data.var.access = frame->vtable->allocLocal(
            frame, info->data.var.type, info->data.var.escapes, tempAllocator);

        if (initializer != NULL) {
          access->vtable->store(
              access, out,
              translateCast(translateExpression(initializer, out,
                                                labelGenerator, tempAllocator),
                            typeofExpression(initializer), info->data.var.type,
                            out, tempAllocator),
              tempAllocator);
        }
      }
      break;
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
    Access *access = id->data.id.symbol->data.var.access;
    char *mangledName =
        access->vtable->getLabel(id->data.id.symbol->data.var.access);
    Fragment *f;
    if (initializer == NULL || constantIsZero(initializer)) {
      f = bssFragmentCreate(mangledName,
                            typeSizeof(id->data.id.symbol->data.var.type));
    } else if (id->data.id.symbol->data.var.type->kind == K_CONST) {
      f = rodataFragmentCreate(mangledName);
      constantToData(initializer, f->data.rodata.ir, fragments, labelGenerator);
    } else {
      f = dataFragmentCreate(mangledName);
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
  char *mangledName = functionAccess->vtable->getLabel(functionAccess);
  Type const *returnType =
      function->data.function.id->data.id.overload->returnType;

  Frame *frame = frameCtor(strdup(mangledName));

  Fragment *f = textFragmentCreate(mangledName, frame);

  TempAllocator tempAllocator;
  tempAllocatorInit(&tempAllocator);

  // allocate function accesses
  for (size_t idx = 0; idx < function->data.function.formals->size; idx++) {
    Node *id = function->data.function.formals->secondElements[idx];
    SymbolInfo *info = id->data.id.symbol;
    info->data.var.access = frame->vtable->allocArg(
        frame, info->data.var.type, info->data.var.escapes, &tempAllocator);
  }

  Access *outArg =
      returnType->kind == K_VOID
          ? NULL
          : frame->vtable->allocRetVal(frame, returnType, &tempAllocator);

  char *exitLabel = labelGenerator->vtable->generateCodeLabel(labelGenerator);
  translateStmt(function->data.function.body, f->data.text.ir, frame, outArg,
                NULL, NULL, exitLabel, labelGenerator, &tempAllocator,
                returnType);
  IR(f->data.text.ir, LABEL(exitLabel));

  if (outArg != NULL) {
    outArg->vtable->dtor(outArg);
  }

  fragmentVectorInsert(fragments, f);
  return;
}
static void translateBody(Node *body, FragmentVector *fragments,
                          char const *moduleName, FrameCtor frameCtor,
                          LabelGenerator *labelGenerator) {
  switch (body->type) {
    case NT_VARDECL: {
      translateGlobalVar(body, fragments, moduleName, labelGenerator);
      return;
    }
    case NT_FUNCTION: {
      translateFunction(body, fragments, moduleName, frameCtor, labelGenerator);
      return;
    }
    default: { return; }
  }
}
static FragmentVector *translateFile(Node *file, FrameCtor frameCtor,
                                     LabelGenerator *labelGenerator) {
  FragmentVector *fragments = fragmentVectorCreate();

  NodeList *bodies = file->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; idx++) {
    translateBody(bodies->elements[idx], fragments,
                  file->data.module.id->data.id.id, frameCtor, labelGenerator);
  }

  return fragments;
}

void translate(FileFragmentVectorMap *fragmentMap, ModuleAstMapPair *asts,
               LabelGeneratorCtor labelGeneratorCtor, FrameCtor frameCtor,
               GlobalAccessCtor globalAccessCtor,
               FunctionAccessCtor functionAccessCtor) {
  fileFragmentVectorMapInit(fragmentMap);

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
    }
  }

  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      fileFragmentVectorMapPut(
          fragmentMap, codeFilenameToAssembyFilename(file->data.file.filename),
          translateFile(file, frameCtor, labelGeneratorCtor()));
    }
  }
}