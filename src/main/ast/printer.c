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

// Implementation for the printers

#include "ast/printer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static double punToDouble(uint64_t bits) {
  union {
    uint64_t i;
    double d;
  } u;
  u.i = bits;
  return u.d;
}
static double punToFloatAsDouble(uint32_t bits) {
  union {
    uint32_t i;
    float f;
  } u;
  u.i = bits;
  return u.f;
}
void nodePrintStructure(Node const *node) {
  if (node == NULL) return;  // base case

  // polymorphically, recursively print
  switch (node->type) {
    case NT_FILE: {
      printf("FILE(");
      nodePrintStructure(node->data.file.module);
      for (size_t idx = 0; idx < node->data.file.imports->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.file.imports->elements[idx]);
      }
      for (size_t idx = 0; idx < node->data.file.bodies->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.file.bodies->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_MODULE: {
      printf("MODULE(");
      nodePrintStructure(node->data.module.id);
      printf(")");
      break;
    }
    case NT_IMPORT: {
      printf("IMPORT(");
      nodePrintStructure(node->data.import.id);
      printf(")");
      break;
    }
    case NT_FUNDECL: {
      printf("FUNDECL(");
      nodePrintStructure(node->data.funDecl.returnType);
      printf(" ");
      nodePrintStructure(node->data.funDecl.id);
      for (size_t idx = 0; idx < node->data.funDecl.params->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.funDecl.params->firstElements[idx]);
        if (node->data.funDecl.params->secondElements[idx] != NULL) {
          printf(" ");
          nodePrintStructure(node->data.funDecl.params->secondElements[idx]);
        }
      }
      printf(")");
      break;
    }
    case NT_FIELDDECL: {
      printf("FIELDDECL(");
      nodePrintStructure(node->data.fieldDecl.type);
      for (size_t idx = 0; idx < node->data.fieldDecl.ids->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.fieldDecl.ids->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_STRUCTDECL: {
      printf("STRUCTDECL(");
      nodePrintStructure(node->data.structDecl.id);
      for (size_t idx = 0; idx < node->data.structDecl.decls->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.structDecl.decls->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_STRUCTFORWARDDECL: {
      printf("STRUCTFORWARDDECL(");
      nodePrintStructure(node->data.structForwardDecl.id);
      printf(")");
      break;
    }
    case NT_UNIONDECL: {
      printf("UNIONDECL(");
      nodePrintStructure(node->data.unionDecl.id);
      for (size_t idx = 0; idx < node->data.unionDecl.opts->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.unionDecl.opts->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_UNIONFORWARDDECL: {
      printf("UNIONFORWARDDECL(");
      nodePrintStructure(node->data.unionForwardDecl.id);
      printf(")");
      break;
    }
    case NT_ENUMDECL: {
      printf("ENUMDECL(");
      nodePrintStructure(node->data.enumDecl.id);
      for (size_t idx = 0; idx < node->data.enumDecl.elements->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.enumDecl.elements->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_ENUMFORWARDDECL: {
      printf("ENUMFORWARDDECL(");
      nodePrintStructure(node->data.enumForwardDecl.id);
      printf(")");
      break;
    }
    case NT_TYPEDEFDECL: {
      printf("TYPEDEFDECL(");
      nodePrintStructure(node->data.typedefDecl.type);
      printf(" ");
      nodePrintStructure(node->data.typedefDecl.id);
      printf(")");
      break;
    }
    case NT_FUNCTION: {
      printf("FUNCTION(");
      nodePrintStructure(node->data.function.returnType);
      printf(" ");
      nodePrintStructure(node->data.function.id);
      for (size_t idx = 0; idx < node->data.function.formals->size; idx++) {
        printf(" FORMAL(");
        nodePrintStructure(node->data.function.formals->firstElements[idx]);
        if (node->data.function.formals->secondElements[idx] != NULL) {
          printf(" ");
          nodePrintStructure(node->data.function.formals->secondElements[idx]);
          if (node->data.function.formals->thirdElements[idx] != NULL) {
            printf(" ");
            nodePrint(node->data.function.formals->thirdElements[idx]);
          }
        }
        printf(")");
      }
      printf(" ");
      nodePrintStructure(node->data.function.body);
      printf(")");
      break;
    }
    case NT_VARDECL: {
      printf("VARDECL(");
      nodePrintStructure(node->data.varDecl.type);
      printf(" ");
      for (size_t idx = 0; idx < node->data.varDecl.idValuePairs->size; idx++) {
        printf(" DECL(");
        nodePrintStructure(node->data.varDecl.idValuePairs->firstElements[idx]);
        if (node->data.varDecl.idValuePairs->secondElements[idx] != NULL) {
          printf(" ");
          nodePrintStructure(
              node->data.varDecl.idValuePairs->secondElements[idx]);
        }
        printf(")");
      }
      printf(")");
      break;
    }
    case NT_COMPOUNDSTMT: {
      printf("COMPOUNDSTMT(");
      for (size_t idx = 0; idx < node->data.compoundStmt.statements->size;
           idx++) {
        if (idx != 0) printf(" ");
        nodePrintStructure(node->data.compoundStmt.statements->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_IFSTMT: {
      printf("IFSTMT(");
      nodePrintStructure(node->data.ifStmt.condition);
      printf(" ");
      nodePrintStructure(node->data.ifStmt.thenStmt);
      if (node->data.ifStmt.elseStmt != NULL) {
        printf(" ");
        nodePrintStructure(node->data.ifStmt.elseStmt);
      }
      printf(")");
      break;
    }
    case NT_WHILESTMT: {
      printf("WHILESTMT(");
      nodePrintStructure(node->data.whileStmt.condition);
      printf(" ");
      nodePrintStructure(node->data.whileStmt.body);
      printf(")");
      break;
    }
    case NT_DOWHILESTMT: {
      printf("DOWHILESTMT(");
      nodePrintStructure(node->data.doWhileStmt.body);
      printf(" ");
      nodePrintStructure(node->data.doWhileStmt.condition);
      printf(")");
      break;
    }
    case NT_FORSTMT: {
      printf("FORSTMT(");
      nodePrintStructure(node->data.forStmt.initialize);
      printf(" ");
      nodePrintStructure(node->data.forStmt.condition);
      printf(" ");
      nodePrintStructure(node->data.forStmt.update);
      printf(" ");
      nodePrintStructure(node->data.forStmt.body);
      printf(")");
      break;
    }
    case NT_SWITCHSTMT: {
      printf("SWITCHSTMT(");
      nodePrintStructure(node->data.switchStmt.onWhat);
      printf(" ");
      for (size_t idx = 0; idx < node->data.switchStmt.cases->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.switchStmt.cases->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_NUMCASE: {
      printf("NUMCASE(");
      for (size_t idx = 0; idx < node->data.numCase.constVals->size; idx++) {
        if (idx != 0) printf(" ");
        nodePrintStructure(node->data.numCase.constVals->elements[idx]);
      }
      printf(" ");
      nodePrintStructure(node->data.numCase.body);
      printf(")");
      break;
    }
    case NT_DEFAULTCASE: {
      printf("DEFAULTCASE(");
      nodePrintStructure(node->data.defaultCase.body);
      printf(")");
      break;
    }
    case NT_BREAKSTMT: {
      printf("BREAKSTMT");
      break;
    }
    case NT_CONTINUESTMT: {
      printf("CONTINUESTMT");
      break;
    }
    case NT_RETURNSTMT: {
      printf("RETURNSTMT");
      if (node->data.returnStmt.value != NULL) {
        printf("(");
        nodePrintStructure(node->data.returnStmt.value);
        printf(")");
      }
      break;
    }
    case NT_ASMSTMT: {
      printf("ASMSTMT(");
      nodePrintStructure(node->data.asmStmt.assembly);
      printf(")");
      break;
    }
    case NT_EXPRESSIONSTMT: {
      printf("EXPRESSIONSTMT(");
      nodePrintStructure(node->data.expressionStmt.expression);
      printf(")");
      break;
    }
    case NT_NULLSTMT: {
      printf("NULLSTMT");
      break;
    }
    case NT_SEQEXP: {
      printf("SEQEXP(");
      nodePrintStructure(node->data.seqExp.first);
      printf(" ");
      nodePrintStructure(node->data.seqExp.rest);
      printf(")");
      break;
    }
    case NT_BINOPEXP: {
      printf("BINOPEXP(");
      switch (node->data.binOpExp.op) {
        case BO_ASSIGN: {
          printf("ASSIGN");
          break;
        }
        case BO_MULASSIGN: {
          printf("MULASSIGN");
          break;
        }
        case BO_DIVASSIGN: {
          printf("DVIASSIGN");
          break;
        }
        case BO_MODASSIGN: {
          printf("MODASSIGN");
          break;
        }
        case BO_ADDASSIGN: {
          printf("ADDASSIGN");
          break;
        }
        case BO_SUBASSIGN: {
          printf("SUBASSIGN");
          break;
        }
        case BO_LSHIFTASSIGN: {
          printf("LSHIFTASSIGN");
          break;
        }
        case BO_LRSHIFTASSIGN: {
          printf("LRSHIFTASSIGN");
          break;
        }
        case BO_ARSHIFTASSIGN: {
          printf("ARSHIFTASSIGN");
          break;
        }
        case BO_BITANDASSIGN: {
          printf("BITANDASSIGN");
          break;
        }
        case BO_BITXORASSIGN: {
          printf("BITXORASSIGN");
          break;
        }
        case BO_BITORASSIGN: {
          printf("BITORASSIGN");
          break;
        }
        case BO_BITAND: {
          printf("BITAND");
          break;
        }
        case BO_BITOR: {
          printf("BITOR");
          break;
        }
        case BO_BITXOR: {
          printf("BITXOR");
          break;
        }
        case BO_SPACESHIP: {
          printf("SPACESHIP");
          break;
        }
        case BO_LSHIFT: {
          printf("LSHIFT");
          break;
        }
        case BO_LRSHIFT: {
          printf("LRSHIFT");
          break;
        }
        case BO_ARSHIFT: {
          printf("ARSHIFT");
          break;
        }
        case BO_ADD: {
          printf("ADD");
          break;
        }
        case BO_SUB: {
          printf("SUB");
          break;
        }
        case BO_MUL: {
          printf("MUL");
          break;
        }
        case BO_DIV: {
          printf("DIV");
          break;
        }
        case BO_MOD: {
          printf("MOD");
          break;
        }
        case BO_ARRAYACCESS: {
          printf("ARRAYACCESS");
          break;
        }
      }
      printf(" ");
      nodePrintStructure(node->data.binOpExp.lhs);
      printf(" ");
      nodePrintStructure(node->data.binOpExp.rhs);
      printf(")");
      break;
    }
    case NT_UNOPEXP: {
      printf("UNOPEXP(");
      switch (node->data.unOpExp.op) {
        case UO_DEREF: {
          printf("DEREF");
          break;
        }
        case UO_ADDROF: {
          printf("ADDROF");
          break;
        }
        case UO_PREINC: {
          printf("PREINC");
          break;
        }
        case UO_PREDEC: {
          printf("PREDEC");
          break;
        }
        case UO_UPLUS: {
          printf("UPLUS");
          break;
        }
        case UO_NEG: {
          printf("NEG");
          break;
        }
        case UO_LNOT: {
          printf("LNOT");
          break;
        }
        case UO_BITNOT: {
          printf("BITNOT");
          break;
        }
        case UO_POSTINC: {
          printf("POSTINC");
          break;
        }
        case UO_POSTDEC: {
          printf("POSTDEC");
          break;
        }
      }
      printf(" ");
      nodePrintStructure(node->data.unOpExp.target);
      printf(")");
      break;
    }
    case NT_COMPOPEXP: {
      printf("COMPOPEXP(");
      switch (node->data.compOpExp.op) {
        case CO_EQ: {
          printf("EQ");
          break;
        }
        case CO_NEQ: {
          printf("NEQ");
          break;
        }
        case CO_LT: {
          printf("LT");
          break;
        }
        case CO_GT: {
          printf("GT");
          break;
        }
        case CO_LTEQ: {
          printf("LTEQ");
          break;
        }
        case CO_GTEQ: {
          printf("GTEQ");
          break;
        }
      }
      printf(" ");
      nodePrintStructure(node->data.compOpExp.lhs);
      printf(" ");
      nodePrintStructure(node->data.compOpExp.rhs);
      printf(")");
      break;
    }
    case NT_LANDASSIGNEXP: {
      printf("LANDASSIGNEXP(");
      nodePrintStructure(node->data.landAssignExp.lhs);
      printf(" ");
      nodePrintStructure(node->data.landAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_LORASSIGNEXP: {
      printf("LORASSIGNEXP(");
      nodePrintStructure(node->data.lorAssignExp.lhs);
      printf(" ");
      nodePrintStructure(node->data.lorAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_TERNARYEXP: {
      printf("TERNARYEXP(");
      nodePrintStructure(node->data.ternaryExp.condition);
      printf(" ");
      nodePrintStructure(node->data.ternaryExp.thenExp);
      printf(" ");
      nodePrintStructure(node->data.ternaryExp.elseExp);
      printf(")");
      break;
    }
    case NT_LANDEXP: {
      printf("LANDEXP(");
      nodePrintStructure(node->data.landExp.lhs);
      printf(" ");
      nodePrintStructure(node->data.landExp.rhs);
      printf(")");
      break;
    }
    case NT_LOREXP: {
      printf("LOREXP(");
      nodePrintStructure(node->data.lorExp.lhs);
      printf(" ");
      nodePrintStructure(node->data.lorExp.rhs);
      printf(")");
      break;
    }
    case NT_STRUCTACCESSEXP: {
      printf("STRUCTACCESSEXP(");
      nodePrintStructure(node->data.structAccessExp.base);
      printf(" ");
      nodePrintStructure(node->data.structAccessExp.element);
      printf(")");
      break;
    }
    case NT_STRUCTPTRACCESSEXP: {
      printf("STRUCTPTRACCESSEXP(");
      nodePrintStructure(node->data.structPtrAccessExp.base);
      printf(" ");
      nodePrintStructure(node->data.structPtrAccessExp.element);
      printf(")");
      break;
    }
    case NT_FNCALLEXP: {
      printf("FNCALLEXP(");
      nodePrintStructure(node->data.fnCallExp.who);
      for (size_t idx = 0; idx < node->data.fnCallExp.args->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.fnCallExp.args->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_CONSTEXP: {
      switch (node->data.constExp.type) {
        case CT_UBYTE: {
          printf("CONST(%hhu)", node->data.constExp.value.ubyteVal);
          break;
        }
        case CT_BYTE: {
          printf("CONST(%hhd)", node->data.constExp.value.byteVal);
          break;
        }
        case CT_USHORT: {
          printf("CONST(%hu)", node->data.constExp.value.ushortVal);
          break;
        }
        case CT_SHORT: {
          printf("CONST(%hd)", node->data.constExp.value.shortVal);
          break;
        }
        case CT_UINT: {
          printf("CONST(%u)", node->data.constExp.value.uintVal);
          break;
        }
        case CT_INT: {
          printf("CONST(%d)", node->data.constExp.value.intVal);
          break;
        }
        case CT_ULONG: {
          printf("CONST(%lu)", node->data.constExp.value.ulongVal);
          break;
        }
        case CT_LONG: {
          printf("CONST(%ld)", node->data.constExp.value.longVal);
          break;
        }
        case CT_FLOAT: {
          printf("CONST(%e)",
                 punToFloatAsDouble(node->data.constExp.value.floatBits));
          break;
        }
        case CT_DOUBLE: {
          printf("CONST(%e)",
                 punToDouble(node->data.constExp.value.doubleBits));
          break;
        }
        case CT_STRING: {
          printf("CONST(\"%s\")", node->data.constExp.value.stringVal);
          break;
        }
        case CT_CHAR: {
          printf("CONST('%c')", node->data.constExp.value.charVal);
          break;
        }
        case CT_WSTRING: {
          printf("CONST(\"%ls\"w)",
                 (wchar_t const *)node->data.constExp.value.wstringVal);
          break;
        }
        case CT_WCHAR: {
          printf("CONST('%lc'w)", (wchar_t)node->data.constExp.value.wcharVal);
          break;
        }
        case CT_BOOL: {
          printf(node->data.constExp.value.boolVal ? "CONST(true)"
                                                   : "CONST(false)");
          break;
        }
        case CT_RANGE_ERROR: {
          printf("RANGE_ERROR");
          break;
        }
      }
      break;
    }
    case NT_AGGREGATEINITEXP: {
      printf("AGGREGATEINITEXP(");
      for (size_t idx = 0; idx < node->data.aggregateInitExp.elements->size;
           idx++) {
        if (idx != 0) printf(" ");
        nodePrintStructure(node->data.aggregateInitExp.elements->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_CASTEXP: {
      printf("CASTEXP(");
      nodePrintStructure(node->data.castExp.toWhat);
      printf(" ");
      nodePrintStructure(node->data.castExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFTYPEEXP: {
      printf("SIZEOFTYPEEXP(");
      nodePrintStructure(node->data.sizeofTypeExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFEXPEXP: {
      printf("SIZEOFEXPEXP(");
      nodePrintStructure(node->data.sizeofExpExp.target);
      printf(")");
      break;
    }
    case NT_KEYWORDTYPE: {
      switch (node->data.keywordType.type) {
        case TK_VOID: {
          printf("KEYWORDTYPE(void)");
          break;
        }
        case TK_UBYTE: {
          printf("KEYWORDTYPE(ubyte)");
          break;
        }
        case TK_CHAR: {
          printf("KEYWORDTYPE(char)");
          break;
        }
        case TK_BYTE: {
          printf("KEYWORDTYPE(byte)");
          break;
        }
        case TK_USHORT: {
          printf("KEYWORDTYPE(ushort)");
          break;
        }
        case TK_SHORT: {
          printf("KEYWORDTYPE(short)");
          break;
        }
        case TK_UINT: {
          printf("KEYWORDTYPE(uint)");
          break;
        }
        case TK_INT: {
          printf("KEYWORDTYPE(int)");
          break;
        }
        case TK_WCHAR: {
          printf("KEYWORDTYPE(wchar)");
          break;
        }
        case TK_ULONG: {
          printf("KEYWORDTYPE(ulong)");
          break;
        }
        case TK_LONG: {
          printf("KEYWORDTYPE(long)");
          break;
        }
        case TK_FLOAT: {
          printf("KEYWORDTYPE(float)");
          break;
        }
        case TK_DOUBLE: {
          printf("KEYWORDTYPE(double)");
          break;
        }
        case TK_BOOL: {
          printf("KEYWORDTYPE(bool)");
          break;
        }
      }
      break;
    }
    case NT_CONSTTYPE: {
      printf("CONSTTYPE(");
      nodePrintStructure(node->data.constType.target);
      printf(")");
      break;
    }
    case NT_ARRAYTYPE: {
      printf("ARRAYTYPE(");
      nodePrintStructure(node->data.arrayType.element);
      printf(" ");
      nodePrintStructure(node->data.arrayType.size);
      printf(")");
      break;
    }
    case NT_PTRTYPE: {
      printf("PTRTYPE(");
      nodePrintStructure(node->data.ptrType.target);
      printf(")");
      break;
    }
    case NT_FNPTRTYPE: {
      printf("FNPTRTYPE(");
      nodePrintStructure(node->data.fnPtrType.returnType);
      for (size_t idx = 0; idx < node->data.fnPtrType.argTypes->size; idx++) {
        printf(" ");
        nodePrintStructure(node->data.fnPtrType.argTypes->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_ID: {
      printf("ID(%s)", node->data.id.id);
      break;
    }
  }
}

void nodePrint(Node const *node) {
  if (node == NULL) return;  // base case

  // recursively, polymorphically print
  switch (node->type) {
    case NT_FILE: {
      nodePrint(node->data.file.module);
      printf("\n");
      for (size_t idx = 0; idx < node->data.file.imports->size; idx++) {
        nodePrint(node->data.file.imports->elements[idx]);
      }
      if (node->data.file.imports->size != 0) printf("\n");
      for (size_t idx = 0; idx < node->data.file.bodies->size; idx++) {
        nodePrint(node->data.file.bodies->elements[idx]);
      }
      break;
    }
    case NT_MODULE: {
      printf("module ");
      nodePrint(node->data.module.id);
      printf(";\n");
      break;
    }
    case NT_IMPORT: {
      printf("using ");
      nodePrint(node->data.import.id);
      printf(";\n");
      break;
    }
    case NT_FUNDECL: {
      nodePrint(node->data.funDecl.returnType);
      printf(" ");
      nodePrint(node->data.funDecl.id);
      printf("(");
      for (size_t idx = 0; idx < node->data.funDecl.params->size; idx++) {
        nodePrint(node->data.funDecl.params->firstElements[idx]);
        if (node->data.funDecl.params->secondElements[idx] != NULL) {
          printf(" = ");
          nodePrint(node->data.funDecl.params->secondElements[idx]);
        }
        if (idx != node->data.funDecl.params->size - 1) {
          printf(", ");
        }
      }
      printf(");\n");
      break;
    }
    case NT_FIELDDECL: {
      nodePrint(node->data.fieldDecl.type);
      printf(" ");
      for (size_t idx = 0; idx < node->data.fieldDecl.ids->size; idx++) {
        nodePrint(node->data.fieldDecl.ids->elements[idx]);
        if (idx != node->data.fieldDecl.ids->size - 1) {
          printf(", ");
        }
        printf(";\n");
      }
      break;
    }
    case NT_STRUCTDECL: {
      printf("struct ");
      nodePrint(node->data.structDecl.id);
      printf(" {\n");
      for (size_t idx = 0; idx < node->data.structDecl.decls->size; idx++) {
        nodePrint(node->data.structDecl.decls->elements[idx]);
        printf("\n");
      }
      printf("};\n");
      break;
    }
    case NT_STRUCTFORWARDDECL: {
      printf("struct ");
      nodePrint(node->data.structForwardDecl.id);
      printf(";\n");
      break;
    }
    case NT_UNIONDECL: {
      printf("union ");
      nodePrint(node->data.unionDecl.id);
      printf(" {\n");
      for (size_t idx = 0; idx < node->data.unionDecl.opts->size; idx++) {
        nodePrint(node->data.unionDecl.opts->elements[idx]);
        printf("\n");
      }
      printf("};\n");
      break;
    }
    case NT_UNIONFORWARDDECL: {
      printf("union ");
      nodePrint(node->data.unionForwardDecl.id);
      printf(";\n");
      break;
    }
    case NT_ENUMDECL: {
      printf("enum ");
      nodePrint(node->data.enumDecl.id);
      printf(" {\n");
      for (size_t idx = 0; idx < node->data.enumDecl.elements->size; idx++) {
        nodePrint(node->data.enumDecl.elements->elements[idx]);
        printf(",\n");
      }
      printf("};\n");
      break;
    }
    case NT_ENUMFORWARDDECL: {
      printf("enum ");
      nodePrint(node->data.enumForwardDecl.id);
      printf(";\n");
      break;
    }
    case NT_TYPEDEFDECL: {
      printf("typedef ");
      nodePrint(node->data.typedefDecl.type);
      printf(" ");
      nodePrint(node->data.typedefDecl.id);
      printf(";");
      break;
    }
    case NT_FUNCTION: {
      nodePrint(node->data.function.returnType);
      printf(" ");
      nodePrint(node->data.function.id);
      printf("(");
      for (size_t idx = 0; idx < node->data.function.formals->size; idx++) {
        nodePrint(node->data.function.formals->firstElements[idx]);
        if (node->data.function.formals->secondElements[idx] != NULL) {
          printf(" ");
          nodePrint(node->data.function.formals->secondElements[idx]);
          if (node->data.function.formals->thirdElements[idx] != NULL) {
            printf(" = ");
            nodePrint(node->data.function.formals->thirdElements[idx]);
          }
        }
        if (idx != node->data.function.formals->size - 1) {
          printf(", ");
        }
      }
      printf(") ");
      nodePrint(node->data.function.body);
      printf("\n");
      break;
    }
    case NT_VARDECL: {
      nodePrint(node->data.varDecl.type);
      printf(" ");
      for (size_t idx = 0; idx < node->data.varDecl.idValuePairs->size; idx++) {
        nodePrint(node->data.varDecl.idValuePairs->firstElements[idx]);
        if (node->data.varDecl.idValuePairs->secondElements[idx] != NULL) {
          printf(" = ");
          nodePrint(node->data.varDecl.idValuePairs->secondElements[idx]);
        }
        if (idx != node->data.varDecl.idValuePairs->size - 1) {
          printf(", ");
        }
      }
      printf(";\n");
      break;
    }
    case NT_COMPOUNDSTMT: {
      printf("{\n");
      for (size_t idx = 0; idx < node->data.compoundStmt.statements->size;
           idx++) {
        nodePrint(node->data.compoundStmt.statements->elements[idx]);
      }
      printf("\n}");
      break;
    }
    case NT_IFSTMT: {
      printf("if (");
      nodePrint(node->data.ifStmt.condition);
      printf(") ");
      nodePrint(node->data.ifStmt.thenStmt);
      if (node->data.ifStmt.elseStmt != NULL) {
        printf("else ");
        nodePrint(node->data.ifStmt.elseStmt);
      }
      printf("\n");
      break;
    }
    case NT_WHILESTMT: {
      printf("while (");
      nodePrint(node->data.whileStmt.condition);
      printf(") ");
      nodePrint(node->data.whileStmt.body);
      printf("\n");
      break;
    }
    case NT_DOWHILESTMT: {
      printf("do ");
      nodePrint(node->data.doWhileStmt.body);
      printf("\nwhile (");
      nodePrint(node->data.doWhileStmt.condition);
      printf(")\n");
      break;
    }
    case NT_FORSTMT: {
      printf("for (");
      nodePrint(node->data.forStmt.initialize);
      printf(" ");
      nodePrint(node->data.forStmt.condition);
      printf("; ");
      nodePrint(node->data.forStmt.update);
      printf(") ");
      nodePrint(node->data.forStmt.body);
      printf("\n");
      break;
    }
    case NT_SWITCHSTMT: {
      printf("switch (");
      nodePrint(node->data.switchStmt.onWhat);
      printf(") {\n");
      for (size_t idx = 0; idx < node->data.switchStmt.cases->size; idx++) {
        nodePrint(node->data.switchStmt.cases->elements[idx]);
      }
      printf("}\n");
      break;
    }
    case NT_NUMCASE: {
      for (size_t idx = 0; idx < node->data.numCase.constVals->size; idx++) {
        printf("case ");
        nodePrint(node->data.numCase.constVals->elements[idx]);
        if (idx == node->data.numCase.constVals->size - 1) {
          printf(":\n");
        } else {
          printf(": ");
        }
      }
      nodePrint(node->data.numCase.body);
      printf("\n");
      break;
    }
    case NT_DEFAULTCASE: {
      printf("default: ");
      nodePrint(node->data.defaultCase.body);
      printf("\n");
      break;
    }
    case NT_BREAKSTMT: {
      printf("break;\n");
      break;
    }
    case NT_CONTINUESTMT: {
      printf("continue;\n");
      break;
    }
    case NT_RETURNSTMT: {
      printf("return");
      if (node->data.returnStmt.value != NULL) {
        printf(" ");
        nodePrint(node->data.returnStmt.value);
      }
      printf(";");
      break;
    }
    case NT_ASMSTMT: {
      printf("asm ");
      nodePrint(node->data.asmStmt.assembly);
      printf(";\n");
      break;
    }
    case NT_EXPRESSIONSTMT: {
      nodePrint(node->data.expressionStmt.expression);
      printf(";\n");
      break;
    }
    case NT_NULLSTMT: {
      printf(";\n");
      break;
    }
    case NT_SEQEXP: {
      printf("(");
      nodePrint(node->data.seqExp.first);
      printf(", ");
      nodePrint(node->data.seqExp.rest);
      printf(")");
      break;
    }
    case NT_BINOPEXP: {
      if (node->data.binOpExp.op == BO_ARRAYACCESS) {
        printf("(");
        nodePrint(node->data.binOpExp.lhs);
        printf("[");
        nodePrint(node->data.binOpExp.rhs);
        printf("])");
        break;
      }
      printf("(");
      nodePrint(node->data.binOpExp.lhs);
      switch (node->data.binOpExp.op) {
        case BO_ASSIGN: {
          printf(" = ");
          break;
        }
        case BO_MULASSIGN: {
          printf(" *= ");
          break;
        }
        case BO_DIVASSIGN: {
          printf(" /= ");
          break;
        }
        case BO_MODASSIGN: {
          printf(" %%= ");
          break;
        }
        case BO_ADDASSIGN: {
          printf(" += ");
          break;
        }
        case BO_SUBASSIGN: {
          printf(" -= ");
          break;
        }
        case BO_LSHIFTASSIGN: {
          printf(" <<= ");
          break;
        }
        case BO_LRSHIFTASSIGN: {
          printf(" >>= ");
          break;
        }
        case BO_ARSHIFTASSIGN: {
          printf(" >>>= ");
          break;
        }
        case BO_BITANDASSIGN: {
          printf(" &= ");
          break;
        }
        case BO_BITXORASSIGN: {
          printf(" ^= ");
          break;
        }
        case BO_BITORASSIGN: {
          printf(" |= ");
          break;
        }
        case BO_BITAND: {
          printf(" & ");
          break;
        }
        case BO_BITOR: {
          printf(" | ");
          break;
        }
        case BO_BITXOR: {
          printf(" ^ ");
          break;
        }
        case BO_SPACESHIP: {
          printf(" <=> ");
          break;
        }
        case BO_LSHIFT: {
          printf(" << ");
          break;
        }
        case BO_LRSHIFT: {
          printf(" >> ");
          break;
        }
        case BO_ARSHIFT: {
          printf(" >>> ");
          break;
        }
        case BO_ADD: {
          printf(" + ");
          break;
        }
        case BO_SUB: {
          printf(" - ");
          break;
        }
        case BO_MUL: {
          printf(" * ");
          break;
        }
        case BO_DIV: {
          printf(" / ");
          break;
        }
        case BO_MOD: {
          printf(" %% ");
          break;
        }
        case BO_ARRAYACCESS: {
          return;
        }
      }
      nodePrint(node->data.binOpExp.rhs);
      printf(")");
      break;
    }
    case NT_UNOPEXP: {
      printf("(");
      switch (node->data.unOpExp.op) {
        case UO_DEREF: {
          printf("*");
          break;
        }
        case UO_ADDROF: {
          printf("&");
          break;
        }
        case UO_PREINC: {
          printf("++");
          break;
        }
        case UO_PREDEC: {
          printf("--");
          break;
        }
        case UO_UPLUS: {
          printf("+");
          break;
        }
        case UO_NEG: {
          printf("-");
          break;
        }
        case UO_LNOT: {
          printf("!");
          break;
        }
        case UO_BITNOT: {
          printf("~");
          break;
        }
        case UO_POSTINC:
        case UO_POSTDEC: {
          break;
        }
      }
      nodePrint(node->data.unOpExp.target);
      switch (node->data.unOpExp.op) {
        case UO_POSTINC: {
          printf("++");
          break;
        }
        case UO_POSTDEC: {
          printf("--");
          break;
        }
        case UO_DEREF:
        case UO_ADDROF:
        case UO_PREINC:
        case UO_PREDEC:
        case UO_UPLUS:
        case UO_NEG:
        case UO_LNOT:
        case UO_BITNOT: {
          break;
        }
      }
      break;
    }
    case NT_COMPOPEXP: {
      printf("(");
      nodePrint(node->data.compOpExp.lhs);
      switch (node->data.compOpExp.op) {
        case CO_EQ: {
          printf(" == ");
          break;
        }
        case CO_NEQ: {
          printf(" != ");
          break;
        }
        case CO_LT: {
          printf(" < ");
          break;
        }
        case CO_GT: {
          printf(" > ");
          break;
        }
        case CO_LTEQ: {
          printf(" <= ");
          break;
        }
        case CO_GTEQ: {
          printf(" >= ");
          break;
        }
      }
      nodePrint(node->data.compOpExp.rhs);
      printf(")");
      break;
    }
    case NT_LANDASSIGNEXP: {
      printf("(");
      nodePrint(node->data.landAssignExp.lhs);
      printf(" &&= ");
      nodePrint(node->data.landAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_LORASSIGNEXP: {
      printf("(");
      nodePrint(node->data.lorAssignExp.lhs);
      printf(" ||= ");
      nodePrint(node->data.lorAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_TERNARYEXP: {
      printf("(");
      nodePrint(node->data.ternaryExp.condition);
      printf(" ? ");
      nodePrint(node->data.ternaryExp.thenExp);
      printf(" : ");
      nodePrint(node->data.ternaryExp.elseExp);
      printf(")");
      break;
    }
    case NT_LANDEXP: {
      printf("(");
      nodePrint(node->data.landExp.lhs);
      printf(" && ");
      nodePrint(node->data.landExp.rhs);
      printf(")");
      break;
    }
    case NT_LOREXP: {
      printf("(");
      nodePrint(node->data.lorExp.lhs);
      printf(" || ");
      nodePrint(node->data.lorExp.rhs);
      printf(")");
      break;
    }
    case NT_STRUCTACCESSEXP: {
      printf("(");
      nodePrint(node->data.structAccessExp.base);
      printf(".");
      nodePrint(node->data.structAccessExp.element);
      printf(")");
      break;
    }
    case NT_STRUCTPTRACCESSEXP: {
      printf("(");
      nodePrint(node->data.structPtrAccessExp.base);
      printf("->");
      nodePrint(node->data.structPtrAccessExp.element);
      printf(")");
      break;
    }
    case NT_FNCALLEXP: {
      printf("(");
      nodePrint(node->data.fnCallExp.who);
      printf("(");
      for (size_t idx = 0; idx < node->data.fnCallExp.args->size; idx++) {
        nodePrint(node->data.fnCallExp.args->elements[idx]);
        if (idx != node->data.fnCallExp.args->size - 1) {
          printf(", ");
        }
      }
      printf("))");
      break;
    }
    case NT_CONSTEXP: {
      switch (node->data.constExp.type) {
        case CT_UBYTE: {
          printf("%hhu", node->data.constExp.value.ubyteVal);
          break;
        }
        case CT_BYTE: {
          printf("%hhd", node->data.constExp.value.byteVal);
          break;
        }
        case CT_USHORT: {
          printf("%hu", node->data.constExp.value.ushortVal);
          break;
        }
        case CT_SHORT: {
          printf("%hd", node->data.constExp.value.ushortVal);
          break;
        }
        case CT_UINT: {
          printf("%u", node->data.constExp.value.uintVal);
          break;
        }
        case CT_INT: {
          printf("%d", node->data.constExp.value.intVal);
          break;
        }
        case CT_ULONG: {
          printf("%lu", node->data.constExp.value.ulongVal);
          break;
        }
        case CT_LONG: {
          printf("%ld", node->data.constExp.value.longVal);
          break;
        }
        case CT_FLOAT: {
          printf("%e", punToFloatAsDouble(node->data.constExp.value.floatBits));
          break;
        }
        case CT_DOUBLE: {
          printf("%e", punToDouble(node->data.constExp.value.doubleBits));
          break;
        }
        case CT_STRING: {
          printf("\"%s\"", node->data.constExp.value.stringVal);
          break;
        }
        case CT_CHAR: {
          printf("'%c'", node->data.constExp.value.charVal);
          break;
        }
        case CT_WSTRING: {
          printf("\"%ls\"w",
                 (wchar_t const *)node->data.constExp.value.wstringVal);
          break;
        }
        case CT_WCHAR: {
          printf("'%lc'w", (wchar_t)node->data.constExp.value.wcharVal);
          break;
        }
        case CT_BOOL: {
          printf(node->data.constExp.value.boolVal ? "true" : "false");
          break;
        }
        case CT_RANGE_ERROR: {
          printf("<RANGE_ERROR>");
          break;
        }
      }
      break;
    }
    case NT_AGGREGATEINITEXP: {
      printf("<");
      for (size_t idx = 0; idx < node->data.aggregateInitExp.elements->size;
           idx++) {
        nodePrint(node->data.aggregateInitExp.elements->elements[idx]);
        if (idx != node->data.aggregateInitExp.elements->size - 1) {
          printf(", ");
        }
      }
      printf(">");
      break;
    }
    case NT_CASTEXP: {
      printf("cast[");
      nodePrint(node->data.castExp.toWhat);
      printf("](");
      nodePrint(node->data.castExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFTYPEEXP: {
      printf("sizeof(");
      nodePrint(node->data.sizeofTypeExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFEXPEXP: {
      printf("sizeof(");
      nodePrint(node->data.sizeofExpExp.target);
      printf(")");
      break;
    }
    case NT_KEYWORDTYPE: {
      switch (node->data.keywordType.type) {
        case TK_VOID: {
          printf("void");
          break;
        }
        case TK_UBYTE: {
          printf("ubyte");
          break;
        }
        case TK_BYTE: {
          printf("byte");
          break;
        }
        case TK_CHAR: {
          printf("char");
          break;
        }
        case TK_USHORT: {
          printf("ushort");
          break;
        }
        case TK_SHORT: {
          printf("short");
          break;
        }
        case TK_UINT: {
          printf("uint");
          break;
        }
        case TK_INT: {
          printf("int");
          break;
        }
        case TK_WCHAR: {
          printf("wchar");
          break;
        }
        case TK_ULONG: {
          printf("ulong");
          break;
        }
        case TK_LONG: {
          printf("long");
          break;
        }
        case TK_FLOAT: {
          printf("float");
          break;
        }
        case TK_DOUBLE: {
          printf("double");
          break;
        }
        case TK_BOOL: {
          printf("bool");
          break;
        }
      }
      break;
    }
    case NT_CONSTTYPE: {
      nodePrint(node->data.constType.target);
      printf(" const");
      break;
    }
    case NT_ARRAYTYPE: {
      nodePrint(node->data.arrayType.element);
      printf("[");
      nodePrint(node->data.arrayType.size);
      printf("]");
      break;
    }
    case NT_PTRTYPE: {
      nodePrint(node->data.ptrType.target);
      printf("*");
      break;
    }
    case NT_FNPTRTYPE: {
      nodePrint(node->data.fnPtrType.returnType);
      printf("(");
      for (size_t idx = 0; idx < node->data.fnPtrType.argTypes->size; idx++) {
        nodePrint(node->data.fnPtrType.argTypes->elements[idx]);
        if (idx != node->data.fnPtrType.argTypes->size - 1) {
          printf(", ");
        }
      }
      printf(")");
      break;
    }
    case NT_ID: {
      printf("%s", node->data.id.id);
      break;
    }
  }
}