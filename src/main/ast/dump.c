// Copyright 2020 Justin Hu
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

#include "ast/dump.h"

#include <stdio.h>
#include <stdlib.h>

#include "util/conversions.h"
#include "util/string.h"

static char const *const BINOP_NAMES[] = {
    "SEQ",           "ASSIGN",       "MULASSIGN",    "DIVASSIGN",
    "ADDASSIGN",     "SUBASSIGN",    "LSHIFTASSIGN", "ARSHIFTASSIGN",
    "LRSHIFTASSIGN", "BITANDASSIGN", "BITXORASSIGN", "BITORASSIGN",
    "LANDASSIGN",    "LORASSIGN",    "LAND",         "LOR",
    "BITAND",        "BITOR",        "BITXOR",       "EQ",
    "NEQ",           "LT",           "GT",           "LTEQ",
    "GTEQ",          "SPACESHIP",    "LSHIFT",       "ARSHIFT",
    "LRSHIFT",       "ADD",          "SUB",          "MUL",
    "DIV",           "MOD",          "FIELD",        "PTRFIELD",
    "ARRAY",         "CAST",
};

static char const *const UNOP_NAMES[] = {
    "DEREF",      "ADDROF",       "PREINC",    "PREDEC",     "NEG",
    "LNOT",       "BITNOT",       "POSTINC",   "POSTDEC",    "NEGASSIGN",
    "LNOTASSIGN", "BITNOTASSIGN", "SIZEOFEXP", "SIZEOFTYPE", "PARENS",
};

static char const *const TYPEKEYWORD_NAMES[] = {
    "void", "ubyte", "byte",  "char", "ushort", "short",  "uint",
    "int",  "wchar", "ulong", "long", "float",  "double", "bool",
};

static char const *const TYPEMODIFIER_NAMES[] = {
    "CONST",
    "VOLATILE",
    "POINTER",
};

static void stabEntryDump(SymbolTableEntry *entry) {
  switch (entry->kind) {
    case SK_VARIABLE: {
      char *typeStr = typeToString(entry->data.variable.type);
      printf("VARIABLE(%s)", typeStr);
      free(typeStr);
      break;
    }
    case SK_FUNCTION: {
      char *returnType = typeToString(entry->data.function.returnType);
      char *argTypes = typeVectorToString(&entry->data.function.argumentTypes);
      printf("FUNCTION(%s, %s)", returnType, argTypes);
      free(returnType);
      free(argTypes);
      break;
    }
    case SK_OPAQUE: {
      printf("OPAQUE()");
      break;
    }
    case SK_STRUCT: {
      printf("STRUCT(");
      for (size_t idx = 0; idx < entry->data.structType.fieldNames.size;
           ++idx) {
        char *typeStr =
            typeToString(entry->data.structType.fieldTypes.elements[idx]);
        printf(", FIELD(%s, %s)", typeStr,
               (char *)entry->data.structType.fieldNames.elements[idx]);
        free(typeStr);
      }
      printf(")");
      break;
    }
    case SK_UNION: {
      printf("UNION(");
      for (size_t idx = 0; idx < entry->data.unionType.optionNames.size;
           ++idx) {
        char *typeStr =
            typeToString(entry->data.unionType.optionTypes.elements[idx]);
        printf(", OPTION(%s, %s)", typeStr,
               (char *)entry->data.unionType.optionNames.elements[idx]);
        free(typeStr);
      }
      printf(")");
      break;
    }
    case SK_ENUM: {
      printf("ENUM(");
      for (size_t idx = 0; idx < entry->data.enumType.constantNames.size;
           ++idx) {
        SymbolTableEntry *constEntry =
            entry->data.enumType.constantValues.elements[idx];
        if (constEntry->data.enumConst.signedness) {
          printf(", CONSTANT(%s, %ld)",
                 (char const *)entry->data.enumType.constantNames.elements[idx],
                 constEntry->data.enumConst.data.signedValue);
        } else {
          printf(", CONSTANT(%s, %lu)",
                 (char const *)entry->data.enumType.constantNames.elements[idx],
                 constEntry->data.enumConst.data.unsignedValue);
        }
      }
      printf(")");
      break;
    }
    case SK_TYPEDEF: {
      char *typeStr = typeToString(entry->data.typedefType.actual);
      printf("TYEPDEF(%s)", typeStr);
      free(typeStr);
      break;
    }
    case SK_ENUMCONST: {
      break;  // not printed!
    }
  }
}

static void stabDump(HashMap *stab) {
  if (stab == NULL) {
    printf("(null)");
    return;
  }

  printf("STAB(");
  bool started = true;
  for (size_t idx = 0; idx < stab->capacity; ++idx) {
    if (stab->keys[idx] != NULL) {
      SymbolTableEntry *entry = stab->values[idx];
      if (started) {
        started = false;
      } else {
        printf(", ");
      }
      printf("ENTRY(%s, ", stab->keys[idx]);
      stabEntryDump(entry);
      printf(")");
    }
  }
  printf(")");
}

static void nodeDump(Node *n) {
  if (n == NULL) {
    printf("(null)");
    return;
  }

  switch (n->type) {
    case NT_FILE: {
      printf("FILE(");
      stabDump(n->data.file.stab);
      printf(", ");
      nodeDump(n->data.file.module);
      for (size_t idx = 0; idx < n->data.file.imports->size; ++idx) {
        printf(", ");
        nodeDump(n->data.file.imports->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.file.bodies->size; ++idx) {
        printf(", ");
        nodeDump(n->data.file.bodies->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_MODULE: {
      printf("MODULE(");
      nodeDump(n->data.module.id);
      printf(")");
      break;
    }
    case NT_IMPORT: {
      printf("IMPORT(");
      nodeDump(n->data.import.id);
      printf(")");
      break;
    }
    case NT_FUNDEFN: {
      printf("FUNDEFN(");
      nodeDump(n->data.funDefn.returnType);
      printf(", ");
      nodeDump(n->data.funDefn.name);
      for (size_t idx = 0; idx < n->data.funDefn.argTypes->size; ++idx) {
        printf(", ");
        nodeDump(n->data.funDefn.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funDefn.argNames->size; ++idx) {
        printf(", ");
        nodeDump(n->data.funDefn.argNames->elements[idx]);
      }
      printf(", ");
      stabDump(n->data.funDefn.argStab);
      printf(", ");
      nodeDump(n->data.funDefn.body);
      printf(")");
      break;
    }
    case NT_VARDEFN: {
      printf("VARDEFN(");
      nodeDump(n->data.varDefn.type);
      for (size_t idx = 0; idx < n->data.varDefn.names->size; ++idx) {
        printf(", ");
        nodeDump(n->data.varDefn.names->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.varDefn.initializers->size; ++idx) {
        printf(", ");
        nodeDump(n->data.varDefn.initializers->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_FUNDECL: {
      printf("FUNDECL(");
      nodeDump(n->data.funDecl.returnType);
      printf(", ");
      nodeDump(n->data.funDecl.name);
      for (size_t idx = 0; idx < n->data.funDecl.argTypes->size; ++idx) {
        printf(", ");
        nodeDump(n->data.funDecl.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funDecl.argNames->size; ++idx) {
        printf(", ");
        nodeDump(n->data.funDecl.argNames->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_VARDECL: {
      printf("VARDECL(");
      nodeDump(n->data.varDecl.type);
      for (size_t idx = 0; idx < n->data.varDecl.names->size; ++idx) {
        printf(", ");
        nodeDump(n->data.varDecl.names->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_OPAQUEDECL: {
      printf("OPAQUEDECL(");
      nodeDump(n->data.opaqueDecl.name);
      printf(")");
      break;
    }
    case NT_STRUCTDECL: {
      printf("STRUCTDECL(");
      nodeDump(n->data.structDecl.name);
      for (size_t idx = 0; idx < n->data.structDecl.fields->size; ++idx) {
        printf(", ");
        nodeDump(n->data.structDecl.fields->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_UNIONDECL: {
      printf("UNIONDECL(");
      nodeDump(n->data.unionDecl.name);
      for (size_t idx = 0; idx < n->data.unionDecl.options->size; ++idx) {
        printf(", ");
        nodeDump(n->data.unionDecl.options->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_ENUMDECL: {
      printf("ENUMDECL(");
      nodeDump(n->data.enumDecl.name);
      for (size_t idx = 0; idx < n->data.enumDecl.constantNames->size; ++idx) {
        printf(", ");
        nodeDump(n->data.enumDecl.constantNames->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.enumDecl.constantValues->size; ++idx) {
        printf(", ");
        nodeDump(n->data.enumDecl.constantValues->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_TYPEDEFDECL: {
      printf("TYPEDEFDECL(");
      nodeDump(n->data.typedefDecl.name);
      printf(", ");
      nodeDump(n->data.typedefDecl.originalType);
      printf(")");
      break;
    }
    case NT_COMPOUNDSTMT: {
      printf("COMPOUNDSTMT(");
      stabDump(n->data.compoundStmt.stab);
      for (size_t idx = 0; idx < n->data.compoundStmt.stmts->size; ++idx) {
        printf(", ");
        nodeDump(n->data.compoundStmt.stmts->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_IFSTMT: {
      printf("IFSTMT(");
      nodeDump(n->data.ifStmt.predicate);
      printf(", ");
      nodeDump(n->data.ifStmt.consequent);
      printf(", ");
      stabDump(n->data.ifStmt.consequentStab);
      printf(", ");
      nodeDump(n->data.ifStmt.alternative);
      printf(", ");
      stabDump(n->data.ifStmt.alternativeStab);
      printf(")");
      break;
    }
    case NT_WHILESTMT: {
      printf("WHILESTMT(");
      nodeDump(n->data.whileStmt.condition);
      printf(", ");
      nodeDump(n->data.whileStmt.body);
      printf(", ");
      stabDump(n->data.whileStmt.bodyStab);
      printf(")");
      break;
    }
    case NT_DOWHILESTMT: {
      printf("DOWHILESTMT(");
      nodeDump(n->data.whileStmt.body);
      printf(", ");
      stabDump(n->data.whileStmt.bodyStab);
      printf(", ");
      nodeDump(n->data.whileStmt.condition);
      printf(")");
      break;
    }
    case NT_FORSTMT: {
      printf("FORSTMT(");
      stabDump(n->data.forStmt.loopStab);
      printf(", ");
      nodeDump(n->data.forStmt.initializer);
      printf(", ");
      nodeDump(n->data.forStmt.condition);
      printf(", ");
      nodeDump(n->data.forStmt.increment);
      printf(", ");
      nodeDump(n->data.forStmt.body);
      printf(", ");
      stabDump(n->data.forStmt.bodyStab);
      printf(")");
      break;
    }
    case NT_SWITCHSTMT: {
      printf("SWITCHSTMT(");
      nodeDump(n->data.switchStmt.condition);
      for (size_t idx = 0; idx < n->data.switchStmt.cases->size; ++idx) {
        printf(", ");
        nodeDump(n->data.switchStmt.cases->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_BREAKSTMT: {
      printf("BREAKSTMT()");
      break;
    }
    case NT_CONTINUESTMT: {
      printf("CONTINUESTMT()");
      break;
    }
    case NT_RETURNSTMT: {
      printf("RETURNSTMT(");
      nodeDump(n->data.returnStmt.value);
      printf(")");
      break;
    }
    case NT_ASMSTMT: {
      printf("ASMSTMT(");
      nodeDump(n->data.asmStmt.assembly);
      printf(")");
      break;
    }
    case NT_VARDEFNSTMT: {
      printf("VARDEFNSTMT(");
      nodeDump(n->data.varDefnStmt.type);
      for (size_t idx = 0; idx < n->data.varDefnStmt.names->size; ++idx) {
        printf(", ");
        nodeDump(n->data.varDefnStmt.names->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.varDefnStmt.initializers->size;
           ++idx) {
        printf(", ");
        nodeDump(n->data.varDefnStmt.initializers->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_EXPRESSIONSTMT: {
      printf("EXPRESSIONSTMT(");
      nodeDump(n->data.expressionStmt.expression);
      printf(")");
      break;
    }
    case NT_NULLSTMT: {
      printf("NULLSTMT()");
      break;
    }
    case NT_SWITCHCASE: {
      printf("SWITCHCASE(");
      for (size_t idx = 0; idx < n->data.switchCase.values->size; ++idx) {
        if (idx != 0) printf(", ");
        nodeDump(n->data.switchCase.values->elements[idx]);
      }
      printf(", ");
      nodeDump(n->data.switchCase.body);
      printf(", ");
      stabDump(n->data.switchCase.bodyStab);
      printf(")");
      break;
    }
    case NT_SWITCHDEFAULT: {
      printf("SWITCHDEFAULT(");
      nodeDump(n->data.switchDefault.body);
      printf(", ");
      stabDump(n->data.switchDefault.bodyStab);
      printf(")");
      break;
    }
    case NT_BINOPEXP: {
      printf("BINOPEXP(%s, ", BINOP_NAMES[n->data.binOpExp.op]);
      nodeDump(n->data.binOpExp.lhs);
      printf(", ");
      nodeDump(n->data.binOpExp.rhs);
      printf(")");
      break;
    }
    case NT_TERNARYEXP: {
      printf("TERNARYEXP(");
      nodeDump(n->data.ternaryExp.predicate);
      printf(", ");
      nodeDump(n->data.ternaryExp.consequent);
      printf(", ");
      nodeDump(n->data.ternaryExp.alternative);
      printf(")");
      break;
    }
    case NT_UNOPEXP: {
      printf("UNOPEXP(%s, ", UNOP_NAMES[n->data.unOpExp.op]);
      nodeDump(n->data.unOpExp.target);
      printf(")");
      break;
    }
    case NT_FUNCALLEXP: {
      printf("FUNCALLEXP(");
      nodeDump(n->data.funCallExp.function);
      for (size_t idx = 0; idx < n->data.funCallExp.arguments->size; ++idx) {
        printf(", ");
        nodeDump(n->data.funCallExp.arguments->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_LITERAL: {
      printf("LITERAL(");
      switch (n->data.literal.type) {
        case LT_UBYTE: {
          printf("UBYTE(%hhu)", n->data.literal.data.ubyteVal);
          break;
        }
        case LT_BYTE: {
          printf("BYTE(%hhd)", n->data.literal.data.byteVal);
          break;
        }
        case LT_USHORT: {
          printf("USHORT(%hu)", n->data.literal.data.ushortVal);
          break;
        }
        case LT_SHORT: {
          printf("SHORT(%hd)", n->data.literal.data.shortVal);
          break;
        }
        case LT_UINT: {
          printf("UINT(%u)", n->data.literal.data.uintVal);
          break;
        }
        case LT_INT: {
          printf("INT(%d)", n->data.literal.data.intVal);
          break;
        }
        case LT_ULONG: {
          printf("ULONG(%lu)", n->data.literal.data.ulongVal);
          break;
        }
        case LT_LONG: {
          printf("LONG(%ld)", n->data.literal.data.longVal);
          break;
        }
        case LT_FLOAT: {
          printf("FLOAT(%E)",
                 (double)bitsToFloat(n->data.literal.data.floatBits));
          break;
        }
        case LT_DOUBLE: {
          printf("DOUBLE(%lE)", bitsToDouble(n->data.literal.data.doubleBits));
          break;
        }
        case LT_STRING: {
          char *escaped = escapeTString(n->data.literal.data.stringVal);
          printf("STRING(%s)", escaped);
          free(escaped);
          break;
        }
        case LT_CHAR: {
          char *escaped = escapeTChar(n->data.literal.data.charVal);
          printf("CHAR(%s)", escaped);
          free(escaped);
          break;
        }
        case LT_WSTRING: {
          char *escaped = escapeTWString(n->data.literal.data.wstringVal);
          printf("WSTRING(%s)", escaped);
          break;
        }
        case LT_WCHAR: {
          printf("WCHAR(%s)", escapeTWChar(n->data.literal.data.wcharVal));
          break;
        }
        case LT_BOOL: {
          printf("BOOL(%s)", n->data.literal.data.boolVal ? "true" : "false");
          break;
        }
        case LT_NULL: {
          printf("NULL()");
          break;
        }
        case LT_AGGREGATEINIT: {
          printf("AGGREGATEINIT(");
          for (size_t idx = 0;
               idx < n->data.literal.data.aggregateInitVal->size; ++idx) {
            if (idx != 0) printf(", ");
            nodeDump(n->data.literal.data.aggregateInitVal->elements[idx]);
          }
          printf(")");
          break;
        }
      }
      printf(")");
      break;
    }
    case NT_KEYWORDTYPE: {
      printf("KEYWORDTYPE(%s)", TYPEKEYWORD_NAMES[n->data.keywordType.keyword]);
      break;
    }
    case NT_MODIFIEDTYPE: {
      printf("MODIFIEDTYPE(%s, ",
             TYPEMODIFIER_NAMES[n->data.modifiedType.modifier]);
      nodeDump(n->data.modifiedType.baseType);
      printf(")");
      break;
    }
    case NT_ARRAYTYPE: {
      printf("ARRAYTYPE(");
      nodeDump(n->data.arrayType.baseType);
      printf(", ");
      nodeDump(n->data.arrayType.size);
      printf(")");
      break;
    }
    case NT_FUNPTRTYPE: {
      printf("FUNPTRTYPE(");
      nodeDump(n->data.funPtrType.returnType);
      for (size_t idx = 0; idx < n->data.funPtrType.argTypes->size; ++idx) {
        printf(", ");
        nodeDump(n->data.funPtrType.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funPtrType.argNames->size; ++idx) {
        printf(", ");
        nodeDump(n->data.funPtrType.argNames->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_SCOPEDID: {
      char *idString = stringifyId(n);
      printf("SCOPEDID(%s)", idString);
      free(idString);
      break;
    }
    case NT_ID: {
      printf("ID(%s)", n->data.id.id);
      break;
    }
    case NT_UNPARSED: {
      break;  // not ever encountered!
    }
  }
}

void astDump(FileListEntry *entry) {
  printf("%s (%s):\n", entry->inputFilename,
         entry->isCode ? "code" : "declaration");
  nodeDump(entry->ast);
  printf("\n");
}