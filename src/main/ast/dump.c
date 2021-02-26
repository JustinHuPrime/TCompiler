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

static void stabEntryDumpStructure(SymbolTableEntry *entry) {
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

static void stabDumpStructure(HashMap *stab) {
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
      stabEntryDumpStructure(entry);
      printf(")");
    }
  }
  printf(")");
}

static void nodeDumpStructure(Node *n) {
  if (n == NULL) {
    printf("(null)");
    return;
  }

  switch (n->type) {
    case NT_FILE: {
      printf("FILE(");
      stabDumpStructure(n->data.file.stab);
      printf(", ");
      nodeDumpStructure(n->data.file.module);
      for (size_t idx = 0; idx < n->data.file.imports->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.file.imports->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.file.bodies->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.file.bodies->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_MODULE: {
      printf("MODULE(");
      nodeDumpStructure(n->data.module.id);
      printf(")");
      break;
    }
    case NT_IMPORT: {
      printf("IMPORT(");
      nodeDumpStructure(n->data.import.id);
      printf(")");
      break;
    }
    case NT_FUNDEFN: {
      printf("FUNDEFN(");
      nodeDumpStructure(n->data.funDefn.returnType);
      printf(", ");
      nodeDumpStructure(n->data.funDefn.name);
      for (size_t idx = 0; idx < n->data.funDefn.argTypes->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.funDefn.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funDefn.argNames->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.funDefn.argNames->elements[idx]);
      }
      printf(", ");
      stabDumpStructure(n->data.funDefn.argStab);
      printf(", ");
      nodeDumpStructure(n->data.funDefn.body);
      printf(")");
      break;
    }
    case NT_VARDEFN: {
      printf("VARDEFN(");
      nodeDumpStructure(n->data.varDefn.type);
      for (size_t idx = 0; idx < n->data.varDefn.names->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.varDefn.names->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.varDefn.initializers->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.varDefn.initializers->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_FUNDECL: {
      printf("FUNDECL(");
      nodeDumpStructure(n->data.funDecl.returnType);
      printf(", ");
      nodeDumpStructure(n->data.funDecl.name);
      for (size_t idx = 0; idx < n->data.funDecl.argTypes->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.funDecl.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funDecl.argNames->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.funDecl.argNames->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_VARDECL: {
      printf("VARDECL(");
      nodeDumpStructure(n->data.varDecl.type);
      for (size_t idx = 0; idx < n->data.varDecl.names->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.varDecl.names->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_OPAQUEDECL: {
      printf("OPAQUEDECL(");
      nodeDumpStructure(n->data.opaqueDecl.name);
      printf(")");
      break;
    }
    case NT_STRUCTDECL: {
      printf("STRUCTDECL(");
      nodeDumpStructure(n->data.structDecl.name);
      for (size_t idx = 0; idx < n->data.structDecl.fields->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.structDecl.fields->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_UNIONDECL: {
      printf("UNIONDECL(");
      nodeDumpStructure(n->data.unionDecl.name);
      for (size_t idx = 0; idx < n->data.unionDecl.options->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.unionDecl.options->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_ENUMDECL: {
      printf("ENUMDECL(");
      nodeDumpStructure(n->data.enumDecl.name);
      for (size_t idx = 0; idx < n->data.enumDecl.constantNames->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.enumDecl.constantNames->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.enumDecl.constantValues->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.enumDecl.constantValues->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_TYPEDEFDECL: {
      printf("TYPEDEFDECL(");
      nodeDumpStructure(n->data.typedefDecl.name);
      printf(", ");
      nodeDumpStructure(n->data.typedefDecl.originalType);
      printf(")");
      break;
    }
    case NT_COMPOUNDSTMT: {
      printf("COMPOUNDSTMT(");
      stabDumpStructure(n->data.compoundStmt.stab);
      for (size_t idx = 0; idx < n->data.compoundStmt.stmts->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.compoundStmt.stmts->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_IFSTMT: {
      printf("IFSTMT(");
      nodeDumpStructure(n->data.ifStmt.predicate);
      printf(", ");
      nodeDumpStructure(n->data.ifStmt.consequent);
      printf(", ");
      stabDumpStructure(n->data.ifStmt.consequentStab);
      printf(", ");
      nodeDumpStructure(n->data.ifStmt.alternative);
      printf(", ");
      stabDumpStructure(n->data.ifStmt.alternativeStab);
      printf(")");
      break;
    }
    case NT_WHILESTMT: {
      printf("WHILESTMT(");
      nodeDumpStructure(n->data.whileStmt.condition);
      printf(", ");
      nodeDumpStructure(n->data.whileStmt.body);
      printf(", ");
      stabDumpStructure(n->data.whileStmt.bodyStab);
      printf(")");
      break;
    }
    case NT_DOWHILESTMT: {
      printf("DOWHILESTMT(");
      nodeDumpStructure(n->data.whileStmt.body);
      printf(", ");
      stabDumpStructure(n->data.whileStmt.bodyStab);
      printf(", ");
      nodeDumpStructure(n->data.whileStmt.condition);
      printf(")");
      break;
    }
    case NT_FORSTMT: {
      printf("FORSTMT(");
      stabDumpStructure(n->data.forStmt.loopStab);
      printf(", ");
      nodeDumpStructure(n->data.forStmt.initializer);
      printf(", ");
      nodeDumpStructure(n->data.forStmt.condition);
      printf(", ");
      nodeDumpStructure(n->data.forStmt.increment);
      printf(", ");
      nodeDumpStructure(n->data.forStmt.body);
      printf(", ");
      stabDumpStructure(n->data.forStmt.bodyStab);
      printf(")");
      break;
    }
    case NT_SWITCHSTMT: {
      printf("SWITCHSTMT(");
      nodeDumpStructure(n->data.switchStmt.condition);
      for (size_t idx = 0; idx < n->data.switchStmt.cases->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.switchStmt.cases->elements[idx]);
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
      nodeDumpStructure(n->data.returnStmt.value);
      printf(")");
      break;
    }
    case NT_ASMSTMT: {
      printf("ASMSTMT(");
      nodeDumpStructure(n->data.asmStmt.assembly);
      printf(")");
      break;
    }
    case NT_VARDEFNSTMT: {
      printf("VARDEFNSTMT(");
      nodeDumpStructure(n->data.varDefnStmt.type);
      for (size_t idx = 0; idx < n->data.varDefnStmt.names->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.varDefnStmt.names->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.varDefnStmt.initializers->size;
           ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.varDefnStmt.initializers->elements[idx]);
      }
      printf(")");
      break;
    }
    case NT_EXPRESSIONSTMT: {
      printf("EXPRESSIONSTMT(");
      nodeDumpStructure(n->data.expressionStmt.expression);
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
        nodeDumpStructure(n->data.switchCase.values->elements[idx]);
      }
      printf(", ");
      nodeDumpStructure(n->data.switchCase.body);
      printf(", ");
      stabDumpStructure(n->data.switchCase.bodyStab);
      printf(")");
      break;
    }
    case NT_SWITCHDEFAULT: {
      printf("SWITCHDEFAULT(");
      nodeDumpStructure(n->data.switchDefault.body);
      printf(", ");
      stabDumpStructure(n->data.switchDefault.bodyStab);
      printf(")");
      break;
    }
    case NT_BINOPEXP: {
      printf("BINOPEXP(%s, ", BINOP_NAMES[n->data.binOpExp.op]);
      nodeDumpStructure(n->data.binOpExp.lhs);
      printf(", ");
      nodeDumpStructure(n->data.binOpExp.rhs);
      printf(")");
      break;
    }
    case NT_TERNARYEXP: {
      printf("TERNARYEXP(");
      nodeDumpStructure(n->data.ternaryExp.predicate);
      printf(", ");
      nodeDumpStructure(n->data.ternaryExp.consequent);
      printf(", ");
      nodeDumpStructure(n->data.ternaryExp.alternative);
      printf(")");
      break;
    }
    case NT_UNOPEXP: {
      printf("UNOPEXP(%s, ", UNOP_NAMES[n->data.unOpExp.op]);
      nodeDumpStructure(n->data.unOpExp.target);
      printf(")");
      break;
    }
    case NT_FUNCALLEXP: {
      printf("FUNCALLEXP(");
      nodeDumpStructure(n->data.funCallExp.function);
      for (size_t idx = 0; idx < n->data.funCallExp.arguments->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.funCallExp.arguments->elements[idx]);
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
            nodeDumpStructure(
                n->data.literal.data.aggregateInitVal->elements[idx]);
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
      nodeDumpStructure(n->data.modifiedType.baseType);
      printf(")");
      break;
    }
    case NT_ARRAYTYPE: {
      printf("ARRAYTYPE(");
      nodeDumpStructure(n->data.arrayType.baseType);
      printf(", ");
      nodeDumpStructure(n->data.arrayType.size);
      printf(")");
      break;
    }
    case NT_FUNPTRTYPE: {
      printf("FUNPTRTYPE(");
      nodeDumpStructure(n->data.funPtrType.returnType);
      for (size_t idx = 0; idx < n->data.funPtrType.argTypes->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.funPtrType.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funPtrType.argNames->size; ++idx) {
        printf(", ");
        nodeDumpStructure(n->data.funPtrType.argNames->elements[idx]);
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

void astDumpStructure(FileListEntry *entry) {
  printf("%s (%s):\n", entry->inputFilename,
         entry->isCode ? "code" : "declaration");
  nodeDumpStructure(entry->ast);
  printf("\n");
}

void astDumpPretty(FileListEntry *entry) {
  printf("%s (%s):\n", entry->inputFilename,
         entry->isCode ? "code" : "declaration");
  // TODO
}