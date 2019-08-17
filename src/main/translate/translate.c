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

// implemetation of the translation phase

#include "translate/translate.h"

#include "constants.h"
#include "util/container/stringBuilder.h"
#include "util/format.h"
#include "util/internalError.h"
#include "util/nameUtils.h"
#include "util/tstring.h"

#include <stdlib.h>
#include <string.h>

// data structures
void fileFragmentVectorMapInit(FileFragmentVectorMap *map) { hashMapInit(map); }
FragmentVector *fileFragmentVectorMapGet(FileFragmentVectorMap *map,
                                         char const *file) {
  return hashMapGet(map, file);
}
int fileFragmentVectorMapPut(FileFragmentVectorMap *map, char *file,
                             FragmentVector *vector) {
  return hashMapPut(map, file, vector, (void (*)(void *))fragmentVectorDestroy);
}
void fileFragmentVectorMapUninit(FileFragmentVectorMap *map) {
  for (size_t idx = 0; idx < map->capacity; idx++) {
    if (map->keys[idx] != NULL) {
      fragmentVectorDestroy(map->values[idx]);
    }
  }
  free(map->values);
  for (size_t idx = 0; idx < map->capacity; idx++) {
    if (map->keys[idx] != NULL) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
      free((char *)(map->keys[idx]));
#pragma GCC diagnostic pop
    }
  }
  free(map->keys);
}

// accesses
static char *mangleModuleName(char const *moduleName) {
  char *buffer = strdup("__Z");
  StringVector *exploded = explodeName(moduleName);
  for (size_t idx = 0; idx < exploded->size; idx++) {
    buffer = format("%s%zu%s", buffer, strlen(exploded->elements[idx]),
                    (char const *)exploded->elements[idx]);
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
    buffer = format("%s%s", buffer, mangledType);
    free(mangledType);
  }
  return buffer;
}
static char *mangleVarName(char const *moduleName, char const *id) {
  return format("%s%zu%s", mangleModuleName(moduleName), strlen(id), id);
}
static char *mangleFunctionName(char const *moduleName, char const *id,
                                TypeVector *argumentTypes) {
  return format("%s%zu%s%s", mangleModuleName(moduleName), strlen(id), id,
                mangleTypeString(argumentTypes));
}
static void addGlobalAccesses(SymbolTable *stab, char const *moduleName,
                              GlobalAccessCtor globalAccessCtor) {
  for (size_t idx = 0; idx < stab->capacity; idx++) {
    if (stab->keys[idx] != NULL) {
      SymbolInfo *info = stab->values[idx];
      if (info->kind == SK_FUNCTION) {
        OverloadSet *set = &info->data.function.overloadSet;
        for (size_t overloadIdx = 0; overloadIdx < set->size; overloadIdx++) {
          OverloadSetElement *elm = set->elements[overloadIdx];
          elm->access = globalAccessCtor(mangleFunctionName(
              moduleName, stab->keys[idx], &elm->argumentTypes));
        }
      } else if (info->kind == SK_VAR) {
        info->data.var.access =
            globalAccessCtor(mangleVarName(moduleName, stab->keys[idx]));
      }
    }
  }
}

// helpers
static bool constantNotZero(Node *initializer) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          return initializer->data.constExp.value.ubyteVal != 0;
        }
        case CT_BYTE: {
          return initializer->data.constExp.value.byteVal != 0;
        }
        case CT_CHAR: {
          return initializer->data.constExp.value.charVal != 0;
        }
        case CT_USHORT: {
          return initializer->data.constExp.value.ushortVal != 0;
        }
        case CT_SHORT: {
          return initializer->data.constExp.value.shortVal != 0;
        }
        case CT_UINT: {
          return initializer->data.constExp.value.uintVal != 0;
        }
        case CT_INT: {
          return initializer->data.constExp.value.intVal != 0;
        }
        case CT_WCHAR: {
          return initializer->data.constExp.value.wcharVal != 0;
        }
        case CT_ULONG: {
          return initializer->data.constExp.value.ulongVal != 0;
        }
        case CT_LONG: {
          return initializer->data.constExp.value.longVal != 0;
        }
        case CT_FLOAT: {
          return initializer->data.constExp.value.floatBits != 0;
        }
        case CT_DOUBLE: {
          return initializer->data.constExp.value.doubleBits != 0;
        }
        case CT_BOOL: {
          return initializer->data.constExp.value.boolVal != false;
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
        if (constantNotZero(elements->elements[idx])) {
          return true;
        }
      }
      return false;
    }
    default: {
      error(__FILE__, __LINE__, "expected a constant, found something else");
    }
  }
}
static void constantToData(Node *initializer, IRExpVector *out,
                           FragmentVector *fragments,
                           LabelGenerator *labelGenerator) {
  switch (initializer->type) {
    case NT_CONSTEXP: {
      switch (initializer->data.constExp.type) {
        case CT_UBYTE: {
          irExpVectorInsert(
              out,
              ubyteConstIRExpCreate(initializer->data.constExp.value.ubyteVal));
          return;
        }
        case CT_BYTE: {
          irExpVectorInsert(out, byteConstIRExpCreate(
                                     initializer->data.constExp.value.byteVal));
          return;
        }
        case CT_CHAR: {
          irExpVectorInsert(out, ubyteConstIRExpCreate(
                                     initializer->data.constExp.value.charVal));
          return;
        }
        case CT_USHORT: {
          irExpVectorInsert(out,
                            ushortConstIRExpCreate(
                                initializer->data.constExp.value.ushortVal));
          return;
        }
        case CT_SHORT: {
          irExpVectorInsert(
              out,
              shortConstIRExpCreate(initializer->data.constExp.value.shortVal));
          return;
        }
        case CT_UINT: {
          irExpVectorInsert(out, uintConstIRExpCreate(
                                     initializer->data.constExp.value.uintVal));
          return;
        }
        case CT_INT: {
          irExpVectorInsert(out, intConstIRExpCreate(
                                     initializer->data.constExp.value.intVal));
          return;
        }
        case CT_WCHAR: {
          irExpVectorInsert(
              out,
              uintConstIRExpCreate(initializer->data.constExp.value.wcharVal));
          return;
        }
        case CT_ULONG: {
          irExpVectorInsert(
              out,
              ulongConstIRExpCreate(initializer->data.constExp.value.ulongVal));
          return;
        }
        case CT_LONG: {
          irExpVectorInsert(out, longConstIRExpCreate(
                                     initializer->data.constExp.value.longVal));
          return;
        }
        case CT_FLOAT: {
          irExpVectorInsert(
              out,
              uintConstIRExpCreate(initializer->data.constExp.value.floatBits));
          return;
        }
        case CT_DOUBLE: {
          irExpVectorInsert(out,
                            doubleConstIRExpCreate(
                                initializer->data.constExp.value.doubleBits));
          return;
        }
        case CT_BOOL: {
          irExpVectorInsert(
              out, ubyteConstIRExpCreate(
                       initializer->data.constExp.value.boolVal ? 1 : 0));
          return;
        }
        case CT_STRING:
        case CT_WSTRING: {
          Fragment *f = roDataFragmentCreate(
              labelGenerator->vtable->generateDataLabel(labelGenerator));
          irExpVectorInsert(
              &f->data.roData.data,
              (initializer->data.constExp.type == CT_STRING
                   ? stringConstIRExpCreate(
                         tstrdup(initializer->data.constExp.value.stringVal))
                   : wstringConstIRExpCreate(twstrdup(
                         initializer->data.constExp.value.wstringVal))));
          irExpVectorInsert(out, nameIRExpCreate(f->data.roData.label));
          return;
        }
        case CT_NULL: {
          irExpVectorInsert(out, ulongConstIRExpCreate(0));
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

// branches
static void translateBranch(Node *branchedOn, char const *trueCase,
                            char const *falseCase, IRStmVector *out) {
  // TODO: write this
}

// expressions
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
static void translateAssignment(IRExp *from, IRExp *to, Type const *fromType,
                                Type const *toType, IRStmVector *out) {
  // TODO: write this
}
static IRExp *translateExpression(Node *, TempGenerator *, LabelGenerator *);
static IRExp *translatePlainBinop(Node *exp, TempGenerator *tempGenerator,
                                  LabelGenerator *labelGenerator,
                                  IRBinOpType byteOp, IRBinOpType shortOp,
                                  IRBinOpType intOp, IRBinOpType longOp) {
  size_t lhsTemp = tempGeneratorGenerate(tempGenerator);
  size_t rhsTemp = tempGeneratorGenerate(tempGenerator);
  Type const *resultType = exp->data.binOpExp.resultType;
  size_t resultSize = typeSizeof(resultType);
  IRBinOpType op;
  switch (resultSize) {
    case 1: {  // BYTE_WIDTH
      op = byteOp;
      break;
    }
    case 2: {  // SHORT_WIDTH
      op = shortOp;
      break;
    }
    case 4: {  // INT_WIDTH
      op = intOp;
      break;
    }
    case 8: {  // LONG_WIDTH
      op = longOp;
      break;
    }
    default: {
      error(__FILE__, __LINE__,
            "bitand operator applied to non-integral sized values");
    }
  }
  IRExp *e =
      eseqIRExpCreate(binopIRExpCreate(op, tempIRExpCreate(lhsTemp, resultSize),
                                       tempIRExpCreate(rhsTemp, resultSize)));
  translateAssignment(translateExpression(exp->data.binOpExp.lhs, tempGenerator,
                                          labelGenerator),
                      tempIRExpCreate(lhsTemp, resultSize),
                      typeofExpression(exp->data.binOpExp.lhs), resultType,
                      &e->data.eseq.stms);
  translateAssignment(translateExpression(exp->data.binOpExp.rhs, tempGenerator,
                                          labelGenerator),
                      tempIRExpCreate(rhsTemp, resultSize),
                      typeofExpression(exp->data.binOpExp.rhs), resultType,
                      &e->data.eseq.stms);
  return e;
}
static IRExp *translateExpression(Node *exp, TempGenerator *tempGenerator,
                                  LabelGenerator *labelGenerator) {
  switch (exp->type) {
    case NT_SEQEXP: {
      IRExp *e = eseqIRExpCreate(translateExpression(
          exp->data.seqExp.last, tempGenerator, labelGenerator));
      irStmVectorInsert(
          &e->data.eseq.stms,
          expIRStmCreate(translateExpression(exp->data.seqExp.prefix,
                                             tempGenerator, labelGenerator)));
      return e;
    }
    case NT_BINOPEXP: {
      switch (exp->data.binOpExp.op) {
        case BO_ASSIGN: {
          Type const *targetType = exp->data.binOpExp.resultType;
          bool isComposite = typeIsComposite(targetType);
          size_t targetSize = typeSizeof(targetType);
          size_t resultTemp = tempGeneratorGenerate(tempGenerator);

          IRExp *e = eseqIRExpCreate(
              (isComposite ? memTempIRExpCreate : tempIRExpCreate)(resultTemp,
                                                                   targetSize));
          translateAssignment(
              translateExpression(exp->data.binOpExp.rhs, tempGenerator,
                                  labelGenerator),
              (isComposite ? memTempIRExpCreate : tempIRExpCreate)(resultTemp,
                                                                   targetSize),
              typeofExpression(exp->data.binOpExp.rhs), targetType,
              &e->data.eseq.stms);
          irStmVectorInsert(
              &e->data.eseq.stms,
              moveIRStmCreate(
                  translateExpression(exp->data.binOpExp.lhs, tempGenerator,
                                      labelGenerator),
                  (isComposite ? memTempIRExpCreate : tempIRExpCreate)(
                      resultTemp, targetSize),
                  targetSize));
          return e;
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
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTEBITAND, IB_SHORTBITAND,
                                     IB_INTBITAND, IB_LONGBITAND);
        }
        case BO_BITOR: {
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTEBITOR, IB_SHORTBITOR, IB_INTBITOR,
                                     IB_LONGBITOR);
        }
        case BO_BITXOR: {
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTEBITXOR, IB_SHORTBITXOR,
                                     IB_INTBITXOR, IB_LONGBITXOR);
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
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTEADD, IB_SHORTADD, IB_INTADD,
                                     IB_LONGADD);
        }
        case BO_SUB: {
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTESUB, IB_SHORTSUB, IB_INTSUB,
                                     IB_LONGSUB);
        }
        case BO_MUL: {
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTEMUL, IB_SHORTMUL, IB_INTMUL,
                                     IB_LONGMUL);
        }
        case BO_DIV: {
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTEDIV, IB_SHORTDIV, IB_INTDIV,
                                     IB_LONGDIV);
        }
        case BO_MOD: {
          return translatePlainBinop(exp, tempGenerator, labelGenerator,
                                     IB_BYTEMOD, IB_SHORTMOD, IB_INTMOD,
                                     IB_LONGMOD);
        }
        case BO_ARRAYACCESS: {
          IRExp *e = translateExpression(exp->data.binOpExp.lhs, tempGenerator,
                                         labelGenerator);
          size_t offsetTemp = tempGeneratorGenerate(tempGenerator);
          Type *ulong = keywordTypeCreate(TK_ULONG);
          switch (e->kind) {
            case IE_MEM: {
              // MEM(x) becomes MEM(BINOP(+ x BINOP(* index sizeof)))
              e = eseqIRExpCreate(memIRExpCreate(binopIRExpCreate(
                  IB_LONGADD, e,
                  binopIRExpCreate(
                      IB_LONGMUL, tempIRExpCreate(offsetTemp, LONG_WIDTH),
                      ulongConstIRExpCreate(
                          typeSizeof(exp->data.binOpExp.resultType))))));
            }
            case IE_MEM_TEMP: {
              // MEM_TEMP(n) becomes MEM_TEMP_OFFSET(MEM_TEMP(n) BINOP(* index
              // sizeof))
              e = eseqIRExpCreate(memTempOffsetIRExpCreate(
                  e, binopIRExpCreate(
                         IB_LONGMUL, tempIRExpCreate(offsetTemp, LONG_WIDTH),
                         ulongConstIRExpCreate(
                             typeSizeof(exp->data.binOpExp.resultType)))));
            }
            default: {
              error(__FILE__, __LINE__,
                    "attempted to access part of a non-composite value");
            }
          }
          translateAssignment(
              translateExpression(exp->data.binOpExp.rhs, tempGenerator,
                                  labelGenerator),
              tempIRExpCreate(offsetTemp, LONG_WIDTH),
              typeofExpression(exp->data.binOpExp.rhs), ulong,
              &e->data.eseq.stms);
          typeDestroy(ulong);
          return e;
        }
      }
    }
    case NT_UNOPEXP: {
      // TODO: write this
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
      size_t resultTemp = tempGeneratorGenerate(tempGenerator);
      Type *resultType = exp->data.ternaryExp.resultType;
      size_t typeSize = typeSizeof(resultType);
      bool isComposite = typeIsComposite(resultType);
      IRExp *e =
          eseqIRExpCreate((isComposite ? memTempIRExpCreate : tempIRExpCreate)(
              resultTemp, typeSize));

      char *trueCaseLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *falseCaseLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *endLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);

      translateBranch(exp->data.ternaryExp.condition, trueCaseLabel,
                      falseCaseLabel, &e->data.eseq.stms);
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(trueCaseLabel));
      translateAssignment(translateExpression(exp->data.ternaryExp.thenExp,
                                              tempGenerator, labelGenerator),
                          (isComposite ? memTempIRExpCreate : tempIRExpCreate)(
                              resultTemp, typeSize),
                          typeofExpression(exp->data.ternaryExp.thenExp),
                          resultType, &e->data.eseq.stms);
      irStmVectorInsert(&e->data.eseq.stms, jumpIRStmCreate(endLabel));
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(falseCaseLabel));
      translateAssignment(translateExpression(exp->data.ternaryExp.elseExp,
                                              tempGenerator, labelGenerator),
                          (isComposite ? memTempIRExpCreate : tempIRExpCreate)(
                              resultTemp, typeSize),
                          typeofExpression(exp->data.ternaryExp.elseExp),
                          resultType, &e->data.eseq.stms);
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(endLabel));
      return e;
    }
    case NT_LANDEXP: {
      size_t resultTemp = tempGeneratorGenerate(tempGenerator);
      IRExp *e = eseqIRExpCreate(tempIRExpCreate(resultTemp, BYTE_WIDTH));

      char *trueCaseLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *falseCaseLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *endLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);

      translateBranch(exp->data.landExp.lhs, trueCaseLabel, falseCaseLabel,
                      &e->data.eseq.stms);
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(trueCaseLabel));
      translateAssignment(translateExpression(exp->data.landExp.rhs,
                                              tempGenerator, labelGenerator),
                          tempIRExpCreate(resultTemp, BYTE_WIDTH),
                          typeofExpression(exp->data.landExp.rhs),
                          exp->data.landExp.resultType, &e->data.eseq.stms);
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(falseCaseLabel));
      irStmVectorInsert(&e->data.eseq.stms,
                        moveIRStmCreate(tempIRExpCreate(resultTemp, BYTE_WIDTH),
                                        byteConstIRExpCreate(0), BYTE_WIDTH));
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(endLabel));
      return e;
    }
    case NT_LOREXP: {
      size_t resultTemp = tempGeneratorGenerate(tempGenerator);
      IRExp *e = eseqIRExpCreate(tempIRExpCreate(resultTemp, BYTE_WIDTH));

      char *trueCaseLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *falseCaseLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *endLabel =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);

      translateBranch(exp->data.lorExp.lhs, trueCaseLabel, falseCaseLabel,
                      &e->data.eseq.stms);
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(trueCaseLabel));
      irStmVectorInsert(&e->data.eseq.stms,
                        moveIRStmCreate(tempIRExpCreate(resultTemp, BYTE_WIDTH),
                                        byteConstIRExpCreate(1), BYTE_WIDTH));
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(falseCaseLabel));
      translateAssignment(translateExpression(exp->data.lorExp.rhs,
                                              tempGenerator, labelGenerator),
                          tempIRExpCreate(resultTemp, BYTE_WIDTH),
                          typeofExpression(exp->data.lorExp.rhs),
                          exp->data.lorExp.resultType, &e->data.eseq.stms);
      irStmVectorInsert(&e->data.eseq.stms, labelIRStmCreate(endLabel));
      return e;
    }
    case NT_STRUCTACCESSEXP: {
      IRExp *e = translateExpression(exp->data.structAccessExp.base,
                                     tempGenerator, labelGenerator);
      switch (e->kind) {
        case IE_MEM: {
          // if MEM(x), becomes MEM(BINOP(+ x offset))
          IRExp *ptr = e->data.mem.ptr;
          e->data.mem.ptr = binopIRExpCreate(
              IB_LONGADD, ptr,
              ulongConstIRExpCreate(exp->data.structAccessExp.offset));
          break;
        }
        case IE_MEM_TEMP: {
          // if MEM_TEMP(n), becomes MEM_TEMP_OFFSET(MEM_TEMP(n) offset)
          e = memTempOffsetIRExpCreate(
              e, ulongConstIRExpCreate(exp->data.structAccessExp.offset));
          break;
        }
        default: {
          error(__FILE__, __LINE__,
                "attempted to access part of a non-composite value");
        }
      }

      return e;
    }
    case NT_STRUCTPTRACCESSEXP: {
      return memIRExpCreate(binopIRExpCreate(
          IB_LONGADD,
          translateExpression(exp->data.structPtrAccessExp.base, tempGenerator,
                              labelGenerator),
          ulongConstIRExpCreate(exp->data.structPtrAccessExp.offset)));
    }
    case NT_FNCALLEXP: {
    }
    case NT_CONSTEXP: {
    }
    case NT_AGGREGATEINITEXP: {
    }
    case NT_CASTEXP: {
    }
    case NT_SIZEOFTYPEEXP: {
    }
    case NT_SIZEOFEXPEXP: {
    }
    case NT_ID: {
    }
    default: {
      error(__FILE__, __LINE__,
            "encountered a non-expression in an expression position");
    }
  }
}

// statements
static void translateStmt(Node *stmt, IRStmVector *out, Frame *frame,
                          Access *outArg, char const *breakLabel,
                          char const *continueLabel, char const *exitLabel,
                          LabelGenerator *labelGenerator,
                          TempGenerator *tempGenerator,
                          Type const *returnType) {
  if (stmt == NULL) {
    return;
  }

  switch (stmt->type) {
    case NT_COMPOUNDSTMT: {
      NodeList *stmts = stmt->data.compoundStmt.statements;
      for (size_t idx = 0; idx < stmts->size; idx++) {
        translateStmt(stmts->elements[idx], out, frame, outArg, breakLabel,
                      continueLabel, exitLabel, labelGenerator, tempGenerator,
                      returnType);
      }
      break;
    }
    case NT_IFSTMT: {
      if (stmt->data.ifStmt.elseStmt == NULL) {
        char *trueCase =
            labelGenerator->vtable->generateCodeLabel(labelGenerator);
        char *end = labelGenerator->vtable->generateCodeLabel(labelGenerator);
        translateBranch(stmt->data.ifStmt.condition, trueCase, end, out);
        irStmVectorInsert(out, labelIRStmCreate(trueCase));
        translateStmt(stmt->data.ifStmt.thenStmt, out, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempGenerator, returnType);
        irStmVectorInsert(out, labelIRStmCreate(end));
      } else {
        char *trueCase =
            labelGenerator->vtable->generateCodeLabel(labelGenerator);
        char *falseCase =
            labelGenerator->vtable->generateCodeLabel(labelGenerator);
        char *end = labelGenerator->vtable->generateCodeLabel(labelGenerator);
        translateBranch(stmt->data.ifStmt.condition, trueCase, falseCase, out);
        irStmVectorInsert(out, labelIRStmCreate(trueCase));
        translateStmt(stmt->data.ifStmt.thenStmt, out, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempGenerator, returnType);
        irStmVectorInsert(out, jumpIRStmCreate(end));
        irStmVectorInsert(out, labelIRStmCreate(falseCase));
        translateStmt(stmt->data.ifStmt.elseStmt, out, frame, outArg,
                      breakLabel, continueLabel, exitLabel, labelGenerator,
                      tempGenerator, returnType);
        irStmVectorInsert(out, labelIRStmCreate(end));
      }
      break;
    }
    case NT_WHILESTMT: {
      char *loopStart =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *loopBody =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *loopEnd = labelGenerator->vtable->generateCodeLabel(labelGenerator);
      irStmVectorInsert(out, labelIRStmCreate(loopStart));
      translateBranch(stmt->data.whileStmt.condition, loopBody, loopEnd, out);
      irStmVectorInsert(out, labelIRStmCreate(loopBody));
      translateStmt(stmt->data.whileStmt.body, out, frame, outArg, loopEnd,
                    loopStart, exitLabel, labelGenerator, tempGenerator,
                    returnType);
      irStmVectorInsert(out, jumpIRStmCreate(loopStart));
      irStmVectorInsert(out, labelIRStmCreate(loopEnd));
      break;
    }
    case NT_DOWHILESTMT: {
      char *loopStart =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *loopContinue =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *loopEnd = labelGenerator->vtable->generateCodeLabel(labelGenerator);
      irStmVectorInsert(out, labelIRStmCreate(loopStart));
      translateStmt(stmt->data.doWhileStmt.body, out, frame, outArg, loopEnd,
                    loopContinue, exitLabel, labelGenerator, tempGenerator,
                    returnType);
      irStmVectorInsert(out, labelIRStmCreate(loopContinue));
      translateBranch(stmt->data.doWhileStmt.condition, loopStart, loopEnd,
                      out);
      irStmVectorInsert(out, labelIRStmCreate(loopEnd));
      break;
    }
    case NT_FORSTMT: {
      char *loopStart =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *loopBody =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *loopContinue =
          labelGenerator->vtable->generateCodeLabel(labelGenerator);
      char *loopEnd = labelGenerator->vtable->generateCodeLabel(labelGenerator);
      Node *initialize = stmt->data.forStmt.initialize;
      if (initialize != NULL) {
        if (initialize->type == NT_VARDECL) {
          translateStmt(stmt->data.forStmt.initialize, out, frame, outArg,
                        breakLabel, continueLabel, exitLabel, labelGenerator,
                        tempGenerator, returnType);
        } else {
          irStmVectorInsert(
              out, expIRStmCreate(translateExpression(initialize, tempGenerator,
                                                      labelGenerator)));
        }
      }
      irStmVectorInsert(out, labelIRStmCreate(loopStart));
      translateBranch(stmt->data.forStmt.condition, loopBody, loopEnd, out);
      irStmVectorInsert(out, labelIRStmCreate(loopBody));
      translateStmt(stmt->data.forStmt.body, out, frame, outArg, loopEnd,
                    loopContinue, exitLabel, labelGenerator, tempGenerator,
                    returnType);
      irStmVectorInsert(out, labelIRStmCreate(loopContinue));
      if (stmt->data.forStmt.update != NULL) {
        irStmVectorInsert(out, expIRStmCreate(translateExpression(
                                   stmt->data.forStmt.update, tempGenerator,
                                   labelGenerator)));
      }
      irStmVectorInsert(out, jumpIRStmCreate(loopStart));
      irStmVectorInsert(out, labelIRStmCreate(loopEnd));
      break;
    }
    case NT_SWITCHSTMT: {
      // TODO: write this
      break;
    }
    case NT_BREAKSTMT: {
      irStmVectorInsert(out, jumpIRStmCreate(breakLabel));
      break;
    }
    case NT_CONTINUESTMT: {
      irStmVectorInsert(out, jumpIRStmCreate(continueLabel));
      break;
    }
    case NT_RETURNSTMT: {
      if (stmt->data.returnStmt.value != NULL) {
        translateAssignment(
            translateExpression(stmt->data.returnStmt.value, tempGenerator,
                                labelGenerator),
            outArg->vtable->valueExp(outArg, frame->vtable->fpExp()),
            typeofExpression(stmt->data.returnStmt.value), returnType, out);
      }
      irStmVectorInsert(out, jumpIRStmCreate(exitLabel));
      break;
    }
    case NT_ASMSTMT: {
      irStmVectorInsert(out,
                        asmIRStmCreate(strdup(stmt->data.asmStmt.assembly)));
      break;
    }
    case NT_EXPRESSIONSTMT: {
      irStmVectorInsert(out, expIRStmCreate(translateExpression(
                                 stmt->data.expressionStmt.expression,
                                 tempGenerator, labelGenerator)));
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
        info->data.var.access = frame->vtable->allocLocal(
            frame, typeSizeof(info->data.var.type), info->data.var.escapes);

        if (initializer != NULL) {
          translateAssignment(
              translateExpression(initializer, tempGenerator, labelGenerator),
              info->data.var.access->vtable->valueExp(info->data.var.access,
                                                      frame->vtable->fpExp()),
              typeofExpression(initializer), info->data.var.type, out);
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

// top level stuff
static void translateGlobalVar(Node *varDecl, FragmentVector *fragments,
                               char const *moduleName,
                               LabelGenerator *labelGenerator) {
  NodePairList *idValuePairs = varDecl->data.varDecl.idValuePairs;
  for (size_t idx = 0; idx < idValuePairs->size; idx++) {
    Node *id = idValuePairs->firstElements[idx];
    Node *initializer = idValuePairs->secondElements[idx];
    char *mangledName = mangleVarName(moduleName, id->data.id.id);
    Fragment *f;
    if (initializer == NULL || !constantNotZero(initializer)) {
      f = bssDataFragmentCreate(mangledName,
                                typeSizeof(id->data.id.symbol->data.var.type));
    } else if (id->data.id.symbol->data.var.type->kind == K_CONST) {
      f = roDataFragmentCreate(mangledName);
      constantToData(initializer, &f->data.roData.data, fragments,
                     labelGenerator);
    } else {
      f = dataFragmentCreate(mangledName);
      constantToData(initializer, &f->data.data.data, fragments,
                     labelGenerator);
    }

    fragmentVectorInsert(fragments, f);
  }

  return;
}
static void translateFunction(Node *function, FragmentVector *fragments,
                              char const *moduleName, FrameCtor frameCtor,
                              LabelGenerator *labelGenerator,
                              TempGenerator *tempGenerator) {
  Frame *frame = frameCtor();
  Fragment *fragment = functionFragmentCreate(
      mangleFunctionName(
          moduleName, function->data.function.id->data.id.id,
          &function->data.function.id->data.id.overload->argumentTypes),
      frame);
  IRStmVector *body = irStmVectorCreate();

  Node *statements = function->data.function.body;
  Access *outArg = frame->vtable->allocOutArg(
      frame,
      typeSizeof(function->data.function.id->data.id.overload->returnType));
  for (size_t idx = 0; idx < function->data.function.formals->size; idx++) {
    Node *id = function->data.function.formals->secondElements[idx];
    SymbolInfo *info = id->data.id.symbol;
    info->data.var.access = frame->vtable->allocInArg(
        frame, typeSizeof(info->data.var.type), info->data.var.escapes);
  }

  char *exitLabel = labelGenerator->vtable->generateCodeLabel(labelGenerator);
  translateStmt(statements, body, frame, outArg, NULL, NULL, exitLabel,
                labelGenerator, tempGenerator,
                function->data.function.id->data.id.overload->returnType);

  outArg->vtable->dtor(outArg);

  body = frame->vtable->generateEntryExit(frame, body, exitLabel);
  irStmVectorUninit(&fragment->data.function.body);
  memcpy(&fragment->data.function.body, body, sizeof(IRStmVector));

  fragmentVectorInsert(fragments, fragment);
  return;
}
static void translateBody(Node *body, FragmentVector *fragments,
                          char const *moduleName, FrameCtor frameCtor,
                          LabelGenerator *labelGenerator,
                          TempGenerator *tempGenerator) {
  switch (body->type) {
    case NT_VARDECL: {
      translateGlobalVar(body, fragments, moduleName, labelGenerator);
      return;
    }
    case NT_FUNCTION: {
      translateFunction(body, fragments, moduleName, frameCtor, labelGenerator,
                        tempGenerator);
      return;
    }
    default: { return; }
  }
}

// file level stuff
static char *codeFilenameToAssembyFilename(char const *codeFilename) {
  size_t len = strlen(codeFilename);
  char *assemblyFilename = strncpy(malloc(len), codeFilename, len);
  assemblyFilename[len - 2] = 's';
  assemblyFilename[len - 1] = '\0';
  return assemblyFilename;
}
static FragmentVector *translateFile(Node *file, FrameCtor frameCtor,
                                     LabelGenerator *labelGenerator,
                                     TempGenerator *tempGenerator) {
  FragmentVector *fragments = fragmentVectorCreate();

  NodeList *bodies = file->data.file.bodies;
  for (size_t idx = 0; idx < bodies->size; idx++) {
    translateBody(bodies->elements[idx], fragments,
                  file->data.module.id->data.id.id, frameCtor, labelGenerator,
                  tempGenerator);
  }

  return fragments;
}

void translate(FileFragmentVectorMap *fragments, ModuleAstMapPair *asts,
               FrameCtor frameCtor, GlobalAccessCtor globalAccessCtor,
               LabelGeneratorCtor labelGeneratorCtor) {
  fileFragmentVectorMapInit(fragments);

  for (size_t idx = 0; idx < asts->decls.capacity; idx++) {
    if (asts->decls.keys[idx] != NULL) {
      Node *file = asts->decls.values[idx];
      addGlobalAccesses(file->data.file.symbols,
                        file->data.file.module->data.module.id->data.id.id,
                        globalAccessCtor);
    }
  }
  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      addGlobalAccesses(file->data.file.symbols,
                        file->data.file.module->data.module.id->data.id.id,
                        globalAccessCtor);
    }
  }

  TempGenerator tempGenerator;
  tempGeneratorInit(&tempGenerator);

  for (size_t idx = 0; idx < asts->codes.capacity; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      Node *file = asts->codes.values[idx];
      fileFragmentVectorMapPut(
          fragments, codeFilenameToAssembyFilename(file->data.file.filename),
          translateFile(file, frameCtor, labelGeneratorCtor(), &tempGenerator));
    }
  }

  tempGeneratorUninit(&tempGenerator);
}