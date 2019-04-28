// Copyright 2019 Justin Hu
//
// This file is part of the T Language Compiler.

// Implementation for the printers

#include "ast/printer.h"

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
void printNodeStructure(Node const *node) {
  if (node == NULL) return;  // base case

  // polymorphically, recursively print
  switch (node->type) {
    case NT_PROGRAM: {
      printf("PROGRAM(");
      printNodeStructure(node->data.program.module);
      for (size_t idx = 0; idx < node->data.program.imports->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.program.imports->elements[idx]);
      }
      for (size_t idx = 0; idx < node->data.program.bodies->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.program.bodies->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_MODULE: {
      printf("MODULE(");
      printNodeStructure(node->data.module.id);
      printf(")");
      break;
    }
    case NT_IMPORT: {
      printf("IMPORT(");
      printNodeStructure(node->data.import.id);
      printf(")");
      break;
    }
    case NT_FUNDECL: {
      printf("FUNDECL(");
      printNodeStructure(node->data.funDecl.returnType);
      printf(" ");
      printNodeStructure(node->data.funDecl.id);
      for (size_t idx = 0; idx < node->data.funDecl.paramTypes->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.funDecl.paramTypes->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_VARDECL: {
      printf("VARDECL(");
      printNodeStructure(node->data.varDecl.type);
      for (size_t idx = 0; idx < node->data.varDecl.ids->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.varDecl.ids->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_STRUCTDECL: {
      printf("STRUCTDECL(");
      printNodeStructure(node->data.structDecl.id);
      for (size_t idx = 0; idx < node->data.structDecl.decls->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.structDecl.decls->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_STRUCTFORWARDDECL: {
      printf("STRUCTFORWARDDECL(");
      printNodeStructure(node->data.structForwardDecl.id);
      printf(")");
      break;
    }
    case NT_UNIONDECL: {
      printf("UNIONDECL(");
      printNodeStructure(node->data.unionDecl.id);
      for (size_t idx = 0; idx < node->data.unionDecl.opts->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.unionDecl.opts->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_UNIONFORWARDDECL: {
      printf("UNIONFORWARDDECL(");
      printNodeStructure(node->data.unionForwardDecl.id);
      printf(")");
      break;
    }
    case NT_ENUMDECL: {
      printf("ENUMDECL(");
      printNodeStructure(node->data.enumDecl.id);
      for (size_t idx = 0; idx < node->data.enumDecl.elements->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.enumDecl.elements->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_ENUMFORWARDDECL: {
      printf("ENUMFORWARDDECL(");
      printNodeStructure(node->data.enumForwardDecl.id);
      printf(")");
      break;
    }
    case NT_TYPEDEFDECL: {
      printf("TYPEDEFDECL(");
      printNodeStructure(node->data.typedefDecl.type);
      printf(" ");
      printNodeStructure(node->data.typedefDecl.id);
      printf(")");
      break;
    }
    case NT_FUNCTION: {
      printf("FUNCTION(");
      printNodeStructure(node->data.function.returnType);
      printf(" ");
      printNodeStructure(node->data.function.id);
      for (size_t idx = 0; idx < node->data.function.formals->size; idx++) {
        printf(" FORMAL(");
        printNodeStructure(node->data.function.formals->firstElements[idx]);
        if (node->data.function.formals->secondElements[idx] != NULL) {
          printf(" ");
          printNodeStructure(node->data.function.formals->secondElements[idx]);
        }
        printf(")");
      }
      printf(" ");
      printNodeStructure(node->data.function.body);
      printf(")");
      break;
    }
    case NT_COMPOUNDSTMT: {
      printf("COMPOUNDSTMT(");
      for (size_t idx = 0; idx < node->data.compoundStmt.statements->size;
           idx++) {
        if (idx != 0) printf(" ");
        printNodeStructure(node->data.compoundStmt.statements->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_IFSTMT: {
      printf("IFSTMT(");
      printNodeStructure(node->data.ifStmt.condition);
      printf(" ");
      printNodeStructure(node->data.ifStmt.thenStmt);
      if (node->data.ifStmt.elseStmt != NULL) {
        printf(" ");
        printNodeStructure(node->data.ifStmt.elseStmt);
      }
      printf(")");
      break;
    }
    case NT_WHILESTMT: {
      printf("WHILESTMT(");
      printNodeStructure(node->data.whileStmt.condition);
      printf(" ");
      printNodeStructure(node->data.whileStmt.body);
      printf(")");
      break;
    }
    case NT_DOWHILESTMT: {
      printf("DOWHILESTMT(");
      printNodeStructure(node->data.doWhileStmt.body);
      printf(" ");
      printNodeStructure(node->data.doWhileStmt.condition);
      printf(")");
      break;
    }
    case NT_FORSTMT: {
      printf("FORSTMT(");
      printNodeStructure(node->data.forStmt.initialize);
      printf(" ");
      printNodeStructure(node->data.forStmt.condition);
      printf(" ");
      printNodeStructure(node->data.forStmt.update);
      printf(" ");
      printNodeStructure(node->data.forStmt.body);
      printf(")");
      break;
    }
    case NT_SWITCHSTMT: {
      printf("SWITCHSTMT(");
      printNodeStructure(node->data.switchStmt.onWhat);
      printf(" ");
      for (size_t idx = 0; idx < node->data.switchStmt.cases->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.switchStmt.cases->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_NUMCASE: {
      printf("NUMCASE(");
      printNodeStructure(node->data.numCase.constVal);
      printf(" ");
      printNodeStructure(node->data.numCase.body);
      printf(")");
      break;
    }
    case NT_DEFAULTCASE: {
      printf("DEFAULTCASE(");
      printNodeStructure(node->data.defaultCase.body);
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
        printNodeStructure(node->data.returnStmt.value);
        printf(")");
      }
      break;
    }
    case NT_VARDECLSTMT: {
      printf("VARDECLSTMT(");
      printNodeStructure(node->data.varDeclStmt.type);
      printf(" ");
      for (size_t idx = 0; idx < node->data.varDeclStmt.idValuePairs->size;
           idx++) {
        printf(" DECL(");
        printNodeStructure(
            node->data.varDeclStmt.idValuePairs->firstElements[idx]);
        if (node->data.varDeclStmt.idValuePairs->secondElements[idx] != NULL) {
          printf(" ");
          printNodeStructure(
              node->data.varDeclStmt.idValuePairs->secondElements[idx]);
        }
        printf(")");
      }
      printf(")");
      break;
    }
    case NT_ASMSTMT: {
      printf("ASMSTMT(");
      printNodeStructure(node->data.asmStmt.assembly);
      printf(")");
      break;
    }
    case NT_EXPRESSIONSTMT: {
      printf("EXPRESSIONSTMT(");
      printNodeStructure(node->data.expressionStmt.expression);
      printf(")");
      break;
    }
    case NT_NULLSTMT: {
      printf("NULLSTMT");
      break;
    }
    case NT_SEQEXP: {
      printf("SEQEXP(");
      printNodeStructure(node->data.seqExp.first);
      printf(" ");
      printNodeStructure(node->data.seqExp.second);
      printf(")");
      break;
    }
    case NT_BINOPEXP: {
      printf("BINOPEXP(");
      switch (node->data.binopExp.op) {
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
      printNodeStructure(node->data.binopExp.lhs);
      printf(" ");
      printNodeStructure(node->data.binopExp.rhs);
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
      printNodeStructure(node->data.unOpExp.target);
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
      printNodeStructure(node->data.compOpExp.lhs);
      printf(" ");
      printNodeStructure(node->data.compOpExp.rhs);
      printf(")");
      break;
    }
    case NT_LANDASSIGNEXP: {
      printf("LANDASSIGNEXP(");
      printNodeStructure(node->data.landAssignExp.lhs);
      printf(" ");
      printNodeStructure(node->data.landAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_LORASSIGNEXP: {
      printf("LORASSIGNEXP(");
      printNodeStructure(node->data.lorAssignExp.lhs);
      printf(" ");
      printNodeStructure(node->data.lorAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_TERNARYEXP: {
      printf("TERNARYEXP(");
      printNodeStructure(node->data.ternaryExp.condition);
      printf(" ");
      printNodeStructure(node->data.ternaryExp.thenExp);
      printf(" ");
      printNodeStructure(node->data.ternaryExp.elseExp);
      printf(")");
      break;
    }
    case NT_LANDEXP: {
      printf("LANDEXP(");
      printNodeStructure(node->data.landExp.lhs);
      printf(" ");
      printNodeStructure(node->data.landExp.rhs);
      printf(")");
      break;
    }
    case NT_LOREXP: {
      printf("LOREXP(");
      printNodeStructure(node->data.lorExp.lhs);
      printf(" ");
      printNodeStructure(node->data.lorExp.rhs);
      printf(")");
      break;
    }
    case NT_STRUCTACCESSEXP: {
      printf("STRUCTACCESSEXP(");
      printNodeStructure(node->data.structAccessExp.base);
      printf(" ");
      printNodeStructure(node->data.structAccessExp.element);
      printf(")");
      break;
    }
    case NT_STRUCTPTRACCESSEXP: {
      printf("STRUCTPTRACCESSEXP(");
      printNodeStructure(node->data.structPtrAccessExp.base);
      printf(" ");
      printNodeStructure(node->data.structPtrAccessExp.element);
      printf(")");
      break;
    }
    case NT_FNCALLEXP: {
      printf("FNCALLEXP(");
      printNodeStructure(node->data.fnCallExp.who);
      for (size_t idx = 0; idx < node->data.fnCallExp.args->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.fnCallExp.args->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_IDEXP: {
      printf("ID(%s)", node->data.idExp.id);
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
          printf("CONST(%s)", node->data.constExp.value.stringVal);
          break;
        }
        case CT_CHAR: {
          printf("CONST(%c)", node->data.constExp.value.charVal);
          break;
        }
        case CT_WSTRING: {
          printf("CONST(%ls)",
                 (wchar_t const *)node->data.constExp.value.wstringVal);
          break;
        }
        case CT_WCHAR: {
          printf("CONST(%lc)", (wchar_t)node->data.constExp.value.wcharVal);
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
        printNodeStructure(node->data.aggregateInitExp.elements->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_CASTEXP: {
      printf("CASTEXP(");
      printNodeStructure(node->data.castExp.toWhat);
      printf(" ");
      printNodeStructure(node->data.castExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFTYPEEXP: {
      printf("SIZEOFTYPEEXP(");
      printNodeStructure(node->data.sizeofTypeExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFEXPEXP: {
      printf("SIZEOFEXPEXP(");
      printNodeStructure(node->data.sizeofExpExp.target);
      printf(")");
      break;
    }
    case NT_KEYWORDTYPE: {
      switch (node->data.typeKeyword.type) {
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
    case NT_IDTYPE: {
      printf("IDTYPE(%s)", node->data.idType.id);
      break;
    }
    case NT_CONSTTYPE: {
      printf("CONSTTYPE(");
      printNodeStructure(node->data.constType.target);
      printf(")");
      break;
    }
    case NT_ARRAYTYPE: {
      printf("ARRAYTYPE(");
      printNodeStructure(node->data.arrayType.element);
      printf(" ");
      printNodeStructure(node->data.arrayType.size);
      printf(")");
      break;
    }
    case NT_PTRTYPE: {
      printf("PTRTYPE(");
      printNodeStructure(node->data.ptrType.target);
      printf(")");
      break;
    }
    case NT_FNPTRTYPE: {
      printf("FNPTRTYPE(");
      printNodeStructure(node->data.fnPtrType.returnType);
      for (size_t idx = 0; idx < node->data.fnPtrType.argTypes->size; idx++) {
        printf(" ");
        printNodeStructure(node->data.fnPtrType.argTypes->elements[idx]);
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

void printNode(Node const *node) {
  if (node == NULL) return;  // base case

  // recursively, polymorphically print
  switch (node->type) {
    case NT_PROGRAM: {
      printNode(node->data.program.module);
      printf("\n");
      for (size_t idx = 0; idx < node->data.program.imports->size; idx++) {
        printNode(node->data.program.imports->elements[idx]);
      }
      if (node->data.program.imports->size != 0) printf("\n");
      for (size_t idx = 0; idx < node->data.program.bodies->size; idx++) {
        printNode(node->data.program.bodies->elements[idx]);
      }
      break;
    }
    case NT_MODULE: {
      printf("module ");
      printNode(node->data.module.id);
      printf(";\n");
      break;
    }
    case NT_IMPORT: {
      printf("using ");
      printNode(node->data.import.id);
      printf(";\n");
      break;
    }
    case NT_FUNDECL: {
      printNode(node->data.funDecl.returnType);
      printf(" ");
      printNode(node->data.funDecl.id);
      printf("(");
      for (size_t idx = 0; idx < node->data.funDecl.paramTypes->size; idx++) {
        printNode(node->data.funDecl.paramTypes->elements[idx]);
        if (idx != node->data.funDecl.paramTypes->size - 1) {
          printf(", ");
        }
      }
      printf(");\n");
      break;
    }
    case NT_VARDECL: {
      printNode(node->data.varDecl.type);
      printf(" ");
      for (size_t idx = 0; idx < node->data.varDecl.ids->size; idx++) {
        printNode(node->data.varDecl.ids->elements[idx]);
        if (idx != node->data.varDecl.ids->size - 1) {
          printf(", ");
        }
        printf(";\n");
      }
      break;
    }
    case NT_STRUCTDECL: {
      printf("struct ");
      printNode(node->data.structDecl.id);
      printf(" {\n");
      for (size_t idx = 0; idx < node->data.structDecl.decls->size; idx++) {
        printNode(node->data.structDecl.decls->elements[idx]);
        printf("\n");
      }
      printf("};\n");
      break;
    }
    case NT_STRUCTFORWARDDECL: {
      printf("struct ");
      printNode(node->data.structForwardDecl.id);
      printf(";\n");
      break;
    }
    case NT_UNIONDECL: {
      printf("union ");
      printNode(node->data.unionDecl.id);
      printf(" {\n");
      for (size_t idx = 0; idx < node->data.unionDecl.opts->size; idx++) {
        printNode(node->data.unionDecl.opts->elements[idx]);
        printf("\n");
      }
      printf("};\n");
      break;
    }
    case NT_UNIONFORWARDDECL: {
      printf("union ");
      printNode(node->data.unionForwardDecl.id);
      printf(";\n");
      break;
    }
    case NT_ENUMDECL: {
      printf("enum ");
      printNode(node->data.enumDecl.id);
      printf(" {\n");
      for (size_t idx = 0; idx < node->data.enumDecl.elements->size; idx++) {
        printNode(node->data.enumDecl.elements->elements[idx]);
        printf(",\n");
      }
      printf("};\n");
      break;
    }
    case NT_ENUMFORWARDDECL: {
      printf("enum ");
      printNode(node->data.enumForwardDecl.id);
      printf(";\n");
      break;
    }
    case NT_TYPEDEFDECL: {
      printf("typedef ");
      printNode(node->data.typedefDecl.type);
      printf(" ");
      printNode(node->data.typedefDecl.id);
      printf(";");
      break;
    }
    case NT_FUNCTION: {
      printNode(node->data.function.returnType);
      printf(" ");
      printNode(node->data.function.id);
      printf("(");
      for (size_t idx = 0; idx < node->data.function.formals->size; idx++) {
        printNode(node->data.function.formals->firstElements[idx]);
        if (node->data.function.formals->secondElements[idx] != NULL) {
          printf(" ");
          printNode(node->data.function.formals->secondElements[idx]);
        }
        if (idx != node->data.function.formals->size - 1) {
          printf(", ");
        }
      }
      printf(") ");
      printNode(node->data.function.body);
      printf("\n");
      break;
    }
    case NT_COMPOUNDSTMT: {
      printf("{\n");
      for (size_t idx = 0; idx < node->data.compoundStmt.statements->size;
           idx++) {
        printNode(node->data.compoundStmt.statements->elements[idx]);
      }
      printf("}");
      break;
    }
    case NT_IFSTMT: {
      printf("if (");
      printNode(node->data.ifStmt.condition);
      printf(") ");
      printNode(node->data.ifStmt.thenStmt);
      if (node->data.ifStmt.elseStmt != NULL) {
        printf("else ");
        printNode(node->data.ifStmt.elseStmt);
      }
      printf("\n");
      break;
    }
    case NT_WHILESTMT: {
      printf("while (");
      printNode(node->data.whileStmt.condition);
      printf(") ");
      printNode(node->data.whileStmt.body);
      printf("\n");
      break;
    }
    case NT_DOWHILESTMT: {
      printf("do ");
      printNode(node->data.doWhileStmt.body);
      printf("\nwhile (");
      printNode(node->data.doWhileStmt.condition);
      printf(")\n");
      break;
    }
    case NT_FORSTMT: {
      printf("for (");
      printNode(node->data.forStmt.initialize);
      printf(" ");
      printNode(node->data.forStmt.condition);
      printf("; ");
      printNode(node->data.forStmt.update);
      printf(") ");
      printNode(node->data.forStmt.body);
      printf("\n");
      break;
    }
    case NT_SWITCHSTMT: {
      printf("switch (");
      printNode(node->data.switchStmt.onWhat);
      printf(") {\n");
      for (size_t idx = 0; idx < node->data.switchStmt.cases->size; idx++) {
        printNode(node->data.switchStmt.cases->elements[idx]);
      }
      printf("}\n");
      break;
    }
    case NT_NUMCASE: {
      printf("case ");
      printNode(node->data.numCase.constVal);
      printf(": ");
      printNode(node->data.numCase.body);
      printf("\n");
      break;
    }
    case NT_DEFAULTCASE: {
      printf("default: ");
      printNode(node->data.defaultCase.body);
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
        printNode(node->data.returnStmt.value);
      }
      printf(";");
      break;
    }
    case NT_VARDECLSTMT: {
      printNode(node->data.varDeclStmt.type);
      printf(" ");
      for (size_t idx = 0; idx < node->data.varDeclStmt.idValuePairs->size;
           idx++) {
        printNode(node->data.varDeclStmt.idValuePairs->firstElements[idx]);
        if (node->data.varDeclStmt.idValuePairs->secondElements[idx] != NULL) {
          printf(" = ");
          printNode(node->data.varDeclStmt.idValuePairs->secondElements[idx]);
        }
        if (idx != node->data.varDeclStmt.idValuePairs->size - 1) {
          printf(", ");
        }
      }
      printf(";\n");
      break;
    }
    case NT_ASMSTMT: {
      printf("asm ");
      printNode(node->data.asmStmt.assembly);
      printf(";\n");
      break;
    }
    case NT_EXPRESSIONSTMT: {
      printNode(node->data.expressionStmt.expression);
      printf(";\n");
      break;
    }
    case NT_NULLSTMT: {
      printf(";\n");
      break;
    }
    case NT_SEQEXP: {
      printf("(");
      printNode(node->data.seqExp.first);
      printf(", ");
      printNode(node->data.seqExp.second);
      printf(")");
      break;
    }
    case NT_BINOPEXP: {
      if (node->data.binopExp.op == BO_ARRAYACCESS) {
        printf("(");
        printNode(node->data.binopExp.lhs);
        printf("[");
        printNode(node->data.binopExp.rhs);
        printf("])");
        break;
      }
      printf("(");
      printNode(node->data.binopExp.lhs);
      switch (node->data.binopExp.op) {
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
        case BO_ARRAYACCESS:
          abort();
      }
      printNode(node->data.binopExp.rhs);
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
        case UO_POSTDEC:
          break;
      }
      printNode(node->data.unOpExp.target);
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
        case UO_BITNOT:
          break;
      }
      break;
    }
    case NT_COMPOPEXP: {
      printf("(");
      printNode(node->data.compOpExp.lhs);
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
      printNode(node->data.compOpExp.rhs);
      printf(")");
      break;
    }
    case NT_LANDASSIGNEXP: {
      printf("(");
      printNode(node->data.landAssignExp.lhs);
      printf(" &&= ");
      printNode(node->data.landAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_LORASSIGNEXP: {
      printf("(");
      printNode(node->data.lorAssignExp.lhs);
      printf(" ||= ");
      printNode(node->data.lorAssignExp.rhs);
      printf(")");
      break;
    }
    case NT_TERNARYEXP: {
      printf("(");
      printNode(node->data.ternaryExp.condition);
      printf(" ? ");
      printNode(node->data.ternaryExp.thenExp);
      printf(" : ");
      printNode(node->data.ternaryExp.elseExp);
      printf(")");
      break;
    }
    case NT_LANDEXP: {
      printf("(");
      printNode(node->data.landExp.lhs);
      printf(" && ");
      printNode(node->data.landExp.rhs);
      printf(")");
      break;
    }
    case NT_LOREXP: {
      printf("(");
      printNode(node->data.lorExp.lhs);
      printf(" || ");
      printNode(node->data.lorExp.rhs);
      printf(")");
      break;
    }
    case NT_STRUCTACCESSEXP: {
      printf("(");
      printNode(node->data.structAccessExp.base);
      printf(".");
      printNode(node->data.structAccessExp.element);
      printf(")");
      break;
    }
    case NT_STRUCTPTRACCESSEXP: {
      printf("(");
      printNode(node->data.structPtrAccessExp.base);
      printf("->");
      printNode(node->data.structPtrAccessExp.element);
      printf(")");
      break;
    }
    case NT_FNCALLEXP: {
      printf("(");
      printNode(node->data.fnCallExp.who);
      printf("(");
      for (size_t idx = 0; idx < node->data.fnCallExp.args->size; idx++) {
        printNode(node->data.fnCallExp.args->elements[idx]);
        if (idx != node->data.fnCallExp.args->size - 1) {
          printf(", ");
        }
      }
      printf("))");
      break;
    }
    case NT_IDEXP: {
      printf("%s", node->data.idExp.id);
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
          printf("%s", node->data.constExp.value.stringVal);
          break;
        }
        case CT_CHAR: {
          printf("%c", node->data.constExp.value.charVal);
          break;
        }
        case CT_WSTRING: {
          printf("%ls", (wchar_t const *)node->data.constExp.value.wstringVal);
          break;
        }
        case CT_WCHAR: {
          printf("%lc", (wchar_t)node->data.constExp.value.wcharVal);
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
        printNode(node->data.aggregateInitExp.elements->elements[idx]);
        if (idx != node->data.aggregateInitExp.elements->size - 1) {
          printf(", ");
        }
      }
      printf(">");
      break;
    }
    case NT_CASTEXP: {
      printf("cast[");
      printNode(node->data.castExp.toWhat);
      printf("](");
      printNode(node->data.castExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFTYPEEXP: {
      printf("sizeof(");
      printNode(node->data.sizeofTypeExp.target);
      printf(")");
      break;
    }
    case NT_SIZEOFEXPEXP: {
      printf("sizeof(");
      printNode(node->data.sizeofExpExp.target);
      printf(")");
      break;
    }
    case NT_KEYWORDTYPE: {
      switch (node->data.typeKeyword.type) {
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
    case NT_IDTYPE: {
      printf("%s", node->data.idType.id);
      break;
    }
    case NT_CONSTTYPE: {
      printNode(node->data.constType.target);
      printf(" const");
      break;
    }
    case NT_ARRAYTYPE: {
      printNode(node->data.arrayType.element);
      printf("[");
      printNode(node->data.arrayType.size);
      printf("]");
      break;
    }
    case NT_PTRTYPE: {
      printNode(node->data.ptrType.target);
      printf("*");
      break;
    }
    case NT_FNPTRTYPE: {
      printNode(node->data.fnPtrType.returnType);
      printf("(");
      for (size_t idx = 0; idx < node->data.fnPtrType.argTypes->size; idx++) {
        printNode(node->data.fnPtrType.argTypes->elements[idx]);
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