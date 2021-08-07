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

#include "ast/dump.h"

#include <stdio.h>
#include <stdlib.h>

#include "util/conversions.h"
#include "util/string.h"

static char const *const BINOP_NAMES[] = {
    "SEQ",           "ASSIGN",       "MULASSIGN",
    "DIVASSIGN",     "MODASSIGN",    "ADDASSIGN",
    "SUBASSIGN",     "LSHIFTASSIGN", "ARSHIFTASSIGN",
    "LRSHIFTASSIGN", "BITANDASSIGN", "BITXORASSIGN",
    "BITORASSIGN",   "LANDASSIGN",   "LORASSIGN",
    "LAND",          "LOR",          "BITAND",
    "BITOR",         "BITXOR",       "EQ",
    "NEQ",           "LT",           "GT",
    "LTEQ",          "GTEQ",         "LSHIFT",
    "ARSHIFT",       "LRSHIFT",      "ADD",
    "SUB",           "MUL",          "DIV",
    "MOD",           "FIELD",        "PTRFIELD",
    "ARRAY",         "CAST",
};

static char const *const UNOP_NAMES[] = {
    "DEREF",      "ADDROF",       "PREINC",    "PREDEC",  "NEG",
    "LNOT",       "BITNOT",       "POSTINC",   "POSTDEC", "NEGASSIGN",
    "LNOTASSIGN", "BITNOTASSIGN", "SIZEOFEXP", "PARENS",
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

static void stabEntryDump(FILE *where, SymbolTableEntry *entry) {
  switch (entry->kind) {
    case SK_VARIABLE: {
      char *typeStr = typeToString(entry->data.variable.type);
      fprintf(where, "VARIABLE(%s, %zu, %zu, %s)", entry->file->inputFilename,
              entry->line, entry->character, typeStr);
      free(typeStr);
      break;
    }
    case SK_FUNCTION: {
      char *returnType = typeToString(entry->data.function.returnType);
      char *argTypes = typeVectorToString(&entry->data.function.argumentTypes);
      fprintf(where, "FUNCTION(%s, %zu, %zu, %s(%s))",
              entry->file->inputFilename, entry->line, entry->character,
              returnType, argTypes);
      free(returnType);
      free(argTypes);
      break;
    }
    case SK_OPAQUE: {
      if (entry->data.opaqueType.definition == NULL)
        fprintf(where, "OPAQUE(%s, %zu, %zu, REFERENCES())",
                entry->file->inputFilename, entry->line, entry->character);
      else
        fprintf(where, "OPAQUE(%s, %zu, %zu, REFERENCES(%s, %zu, %zu))",
                entry->file->inputFilename, entry->line, entry->character,
                entry->data.opaqueType.definition->file->inputFilename,
                entry->data.opaqueType.definition->line,
                entry->data.opaqueType.definition->character);
      break;
    }
    case SK_STRUCT: {
      fprintf(where, "STRUCT(%s, %zu, %zu", entry->file->inputFilename,
              entry->line, entry->character);
      for (size_t idx = 0; idx < entry->data.structType.fieldNames.size;
           ++idx) {
        char *typeStr =
            typeToString(entry->data.structType.fieldTypes.elements[idx]);
        fprintf(where, ", FIELD(%s, %s)", typeStr,
                (char *)entry->data.structType.fieldNames.elements[idx]);
        free(typeStr);
      }
      fprintf(where, ")");
      break;
    }
    case SK_UNION: {
      fprintf(where, "UNION(%s, %zu, %zu", entry->file->inputFilename,
              entry->line, entry->character);
      for (size_t idx = 0; idx < entry->data.unionType.optionNames.size;
           ++idx) {
        char *typeStr =
            typeToString(entry->data.unionType.optionTypes.elements[idx]);
        fprintf(where, ", OPTION(%s, %s)", typeStr,
                (char *)entry->data.unionType.optionNames.elements[idx]);
        free(typeStr);
      }
      fprintf(where, ")");
      break;
    }
    case SK_ENUM: {
      fprintf(where, "ENUM(%s, %zu, %zu", entry->file->inputFilename,
              entry->line, entry->character);
      for (size_t idx = 0; idx < entry->data.enumType.constantNames.size;
           ++idx) {
        SymbolTableEntry *constEntry =
            entry->data.enumType.constantValues.elements[idx];
        if (constEntry->data.enumConst.signedness) {
          fprintf(
              where, ", CONSTANT(%s, %ld)",
              (char const *)entry->data.enumType.constantNames.elements[idx],
              constEntry->data.enumConst.data.signedValue);
        } else {
          fprintf(
              where, ", CONSTANT(%s, %lu)",
              (char const *)entry->data.enumType.constantNames.elements[idx],
              constEntry->data.enumConst.data.unsignedValue);
        }
      }
      fprintf(where, ")");
      break;
    }
    case SK_TYPEDEF: {
      char *typeStr = typeToString(entry->data.typedefType.actual);
      fprintf(where, "TYEPDEF(%s, %zu, %zu, %s)", entry->file->inputFilename,
              entry->line, entry->character, typeStr);
      free(typeStr);
      break;
    }
    case SK_ENUMCONST: {
      break;  // not printed!
    }
  }
}

static void stabDump(FILE *where, HashMap *stab) {
  if (stab == NULL) {
    fprintf(where, "(null)");
    return;
  }

  fprintf(where, "STAB(");
  bool started = true;
  for (size_t idx = 0; idx < stab->capacity; ++idx) {
    if (stab->keys[idx] != NULL) {
      SymbolTableEntry *entry = stab->values[idx];
      if (started) {
        started = false;
      } else {
        fprintf(where, ", ");
      }
      fprintf(where, "ENTRY(%s, ", stab->keys[idx]);
      stabEntryDump(where, entry);
      fprintf(where, ")");
    }
  }
  fprintf(where, ")");
}

static void nodeDump(FILE *where, Node *n) {
  if (n == NULL) {
    fprintf(where, "(null)");
    return;
  }

  switch (n->type) {
    case NT_FILE: {
      fprintf(where, "FILE(%zu, %zu, ", n->line, n->character);
      stabDump(where, n->data.file.stab);
      fprintf(where, ", ");
      nodeDump(where, n->data.file.module);
      for (size_t idx = 0; idx < n->data.file.imports->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.file.imports->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.file.bodies->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.file.bodies->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_MODULE: {
      fprintf(where, "MODULE(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.module.id);
      fprintf(where, ")");
      break;
    }
    case NT_IMPORT: {
      fprintf(where, "IMPORT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.import.id);
      fprintf(where, ")");
      break;
    }
    case NT_FUNDEFN: {
      fprintf(where, "FUNDEFN(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.funDefn.returnType);
      fprintf(where, ", ");
      nodeDump(where, n->data.funDefn.name);
      for (size_t idx = 0; idx < n->data.funDefn.argTypes->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.funDefn.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funDefn.argNames->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.funDefn.argNames->elements[idx]);
      }
      fprintf(where, ", ");
      stabDump(where, n->data.funDefn.argStab);
      fprintf(where, ", ");
      nodeDump(where, n->data.funDefn.body);
      fprintf(where, ")");
      break;
    }
    case NT_VARDEFN: {
      fprintf(where, "VARDEFN(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.varDefn.type);
      for (size_t idx = 0; idx < n->data.varDefn.names->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.varDefn.names->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.varDefn.initializers->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.varDefn.initializers->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_FUNDECL: {
      fprintf(where, "FUNDECL(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.funDecl.returnType);
      fprintf(where, ", ");
      nodeDump(where, n->data.funDecl.name);
      for (size_t idx = 0; idx < n->data.funDecl.argTypes->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.funDecl.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funDecl.argNames->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.funDecl.argNames->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_VARDECL: {
      fprintf(where, "VARDECL(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.varDecl.type);
      for (size_t idx = 0; idx < n->data.varDecl.names->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.varDecl.names->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_OPAQUEDECL: {
      fprintf(where, "OPAQUEDECL(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.opaqueDecl.name);
      fprintf(where, ")");
      break;
    }
    case NT_STRUCTDECL: {
      fprintf(where, "STRUCTDECL(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.structDecl.name);
      for (size_t idx = 0; idx < n->data.structDecl.fields->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.structDecl.fields->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_UNIONDECL: {
      fprintf(where, "UNIONDECL(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.unionDecl.name);
      for (size_t idx = 0; idx < n->data.unionDecl.options->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.unionDecl.options->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_ENUMDECL: {
      fprintf(where, "ENUMDECL(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.enumDecl.name);
      for (size_t idx = 0; idx < n->data.enumDecl.constantNames->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.enumDecl.constantNames->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.enumDecl.constantValues->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.enumDecl.constantValues->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_TYPEDEFDECL: {
      fprintf(where, "TYPEDEFDECL(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.typedefDecl.name);
      fprintf(where, ", ");
      nodeDump(where, n->data.typedefDecl.originalType);
      fprintf(where, ")");
      break;
    }
    case NT_COMPOUNDSTMT: {
      fprintf(where, "COMPOUNDSTMT(%zu, %zu, ", n->line, n->character);
      stabDump(where, n->data.compoundStmt.stab);
      for (size_t idx = 0; idx < n->data.compoundStmt.stmts->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.compoundStmt.stmts->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_IFSTMT: {
      fprintf(where, "IFSTMT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.ifStmt.predicate);
      fprintf(where, ", ");
      nodeDump(where, n->data.ifStmt.consequent);
      fprintf(where, ", ");
      stabDump(where, n->data.ifStmt.consequentStab);
      fprintf(where, ", ");
      nodeDump(where, n->data.ifStmt.alternative);
      fprintf(where, ", ");
      stabDump(where, n->data.ifStmt.alternativeStab);
      fprintf(where, ")");
      break;
    }
    case NT_WHILESTMT: {
      fprintf(where, "WHILESTMT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.whileStmt.condition);
      fprintf(where, ", ");
      nodeDump(where, n->data.whileStmt.body);
      fprintf(where, ", ");
      stabDump(where, n->data.whileStmt.bodyStab);
      fprintf(where, ")");
      break;
    }
    case NT_DOWHILESTMT: {
      fprintf(where, "DOWHILESTMT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.doWhileStmt.body);
      fprintf(where, ", ");
      stabDump(where, n->data.doWhileStmt.bodyStab);
      fprintf(where, ", ");
      nodeDump(where, n->data.doWhileStmt.condition);
      fprintf(where, ")");
      break;
    }
    case NT_FORSTMT: {
      fprintf(where, "FORSTMT(%zu, %zu, ", n->line, n->character);
      stabDump(where, n->data.forStmt.loopStab);
      fprintf(where, ", ");
      nodeDump(where, n->data.forStmt.initializer);
      fprintf(where, ", ");
      nodeDump(where, n->data.forStmt.condition);
      fprintf(where, ", ");
      nodeDump(where, n->data.forStmt.increment);
      fprintf(where, ", ");
      nodeDump(where, n->data.forStmt.body);
      fprintf(where, ", ");
      stabDump(where, n->data.forStmt.bodyStab);
      fprintf(where, ")");
      break;
    }
    case NT_SWITCHSTMT: {
      fprintf(where, "SWITCHSTMT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.switchStmt.condition);
      for (size_t idx = 0; idx < n->data.switchStmt.cases->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.switchStmt.cases->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_BREAKSTMT: {
      fprintf(where, "BREAKSTMT(%zu, %zu)", n->line, n->character);
      break;
    }
    case NT_CONTINUESTMT: {
      fprintf(where, "CONTINUESTMT(%zu, %zu)", n->line, n->character);
      break;
    }
    case NT_RETURNSTMT: {
      fprintf(where, "RETURNSTMT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.returnStmt.value);
      fprintf(where, ")");
      break;
    }
    case NT_VARDEFNSTMT: {
      fprintf(where, "VARDEFNSTMT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.varDefnStmt.type);
      for (size_t idx = 0; idx < n->data.varDefnStmt.names->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.varDefnStmt.names->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.varDefnStmt.initializers->size;
           ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.varDefnStmt.initializers->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_EXPRESSIONSTMT: {
      fprintf(where, "EXPRESSIONSTMT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.expressionStmt.expression);
      fprintf(where, ")");
      break;
    }
    case NT_NULLSTMT: {
      fprintf(where, "NULLSTMT(%zu, %zu)", n->line, n->character);
      break;
    }
    case NT_SWITCHCASE: {
      fprintf(where, "SWITCHCASE(%zu, %zu", n->line, n->character);
      for (size_t idx = 0; idx < n->data.switchCase.values->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.switchCase.values->elements[idx]);
      }
      fprintf(where, ", ");
      nodeDump(where, n->data.switchCase.body);
      fprintf(where, ", ");
      stabDump(where, n->data.switchCase.bodyStab);
      fprintf(where, ")");
      break;
    }
    case NT_SWITCHDEFAULT: {
      fprintf(where, "SWITCHDEFAULT(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.switchDefault.body);
      fprintf(where, ", ");
      stabDump(where, n->data.switchDefault.bodyStab);
      fprintf(where, ")");
      break;
    }
    case NT_BINOPEXP: {
      fprintf(where, "BINOPEXP(%zu, %zu, %s, ", n->line, n->character,
              BINOP_NAMES[n->data.binOpExp.op]);
      nodeDump(where, n->data.binOpExp.lhs);
      fprintf(where, ", ");
      nodeDump(where, n->data.binOpExp.rhs);
      fprintf(where, ")");
      break;
    }
    case NT_TERNARYEXP: {
      fprintf(where, "TERNARYEXP(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.ternaryExp.predicate);
      fprintf(where, ", ");
      nodeDump(where, n->data.ternaryExp.consequent);
      fprintf(where, ", ");
      nodeDump(where, n->data.ternaryExp.alternative);
      fprintf(where, ")");
      break;
    }
    case NT_UNOPEXP: {
      fprintf(where, "UNOPEXP(%zu, %zu, %s, ", n->line, n->character,
              UNOP_NAMES[n->data.unOpExp.op]);
      nodeDump(where, n->data.unOpExp.target);
      fprintf(where, ")");
      break;
    }
    case NT_SIZEOFTYPEEXP: {
      fprintf(where, "SIZEOFTYPEEXP(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.sizeofTypeExp.targetNode);
      fprintf(where, ")");
      break;
    }
    case NT_FUNCALLEXP: {
      fprintf(where, "FUNCALLEXP(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.funCallExp.function);
      for (size_t idx = 0; idx < n->data.funCallExp.arguments->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.funCallExp.arguments->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_LITERAL: {
      fprintf(where, "LITERAL(%zu, %zu, ", n->line, n->character);
      switch (n->data.literal.literalType) {
        case LT_UBYTE: {
          fprintf(where, "UBYTE(%hhu)", n->data.literal.data.ubyteVal);
          break;
        }
        case LT_BYTE: {
          fprintf(where, "BYTE(%hhd)", n->data.literal.data.byteVal);
          break;
        }
        case LT_USHORT: {
          fprintf(where, "USHORT(%hu)", n->data.literal.data.ushortVal);
          break;
        }
        case LT_SHORT: {
          fprintf(where, "SHORT(%hd)", n->data.literal.data.shortVal);
          break;
        }
        case LT_UINT: {
          fprintf(where, "UINT(%u)", n->data.literal.data.uintVal);
          break;
        }
        case LT_INT: {
          fprintf(where, "INT(%d)", n->data.literal.data.intVal);
          break;
        }
        case LT_ULONG: {
          fprintf(where, "ULONG(%lu)", n->data.literal.data.ulongVal);
          break;
        }
        case LT_LONG: {
          fprintf(where, "LONG(%ld)", n->data.literal.data.longVal);
          break;
        }
        case LT_FLOAT: {
          fprintf(where, "FLOAT(%E)",
                  (double)bitsToFloat(n->data.literal.data.floatBits));
          break;
        }
        case LT_DOUBLE: {
          fprintf(where, "DOUBLE(%lE)",
                  bitsToDouble(n->data.literal.data.doubleBits));
          break;
        }
        case LT_STRING: {
          char *escaped = escapeTString(n->data.literal.data.stringVal);
          fprintf(where, "STRING(%s)", escaped);
          free(escaped);
          break;
        }
        case LT_CHAR: {
          char *escaped = escapeTChar(n->data.literal.data.charVal);
          fprintf(where, "CHAR(%s)", escaped);
          free(escaped);
          break;
        }
        case LT_WSTRING: {
          char *escaped = escapeTWString(n->data.literal.data.wstringVal);
          fprintf(where, "WSTRING(%s)", escaped);
          free(escaped);
          break;
        }
        case LT_WCHAR: {
          char *escaped = escapeTWChar(n->data.literal.data.wcharVal);
          fprintf(where, "WCHAR(%s)", escaped);
          free(escaped);
          break;
        }
        case LT_BOOL: {
          fprintf(where, "BOOL(%s)",
                  n->data.literal.data.boolVal ? "true" : "false");
          break;
        }
        case LT_NULL: {
          fprintf(where, "NULL()");
          break;
        }
        case LT_AGGREGATEINIT: {
          fprintf(where, "AGGREGATEINIT(");
          for (size_t idx = 0;
               idx < n->data.literal.data.aggregateInitVal->size; ++idx) {
            if (idx != 0) fprintf(where, ", ");
            nodeDump(where,
                     n->data.literal.data.aggregateInitVal->elements[idx]);
          }
          fprintf(where, ")");
          break;
        }
      }
      fprintf(where, ")");
      break;
    }
    case NT_KEYWORDTYPE: {
      fprintf(where, "KEYWORDTYPE(%zu, %zu, %s)", n->line, n->character,
              TYPEKEYWORD_NAMES[n->data.keywordType.keyword]);
      break;
    }
    case NT_MODIFIEDTYPE: {
      fprintf(where, "MODIFIEDTYPE(%zu, %zu, %s, ", n->line, n->character,
              TYPEMODIFIER_NAMES[n->data.modifiedType.modifier]);
      nodeDump(where, n->data.modifiedType.baseType);
      fprintf(where, ")");
      break;
    }
    case NT_ARRAYTYPE: {
      fprintf(where, "ARRAYTYPE(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.arrayType.baseType);
      fprintf(where, ", ");
      nodeDump(where, n->data.arrayType.size);
      fprintf(where, ")");
      break;
    }
    case NT_FUNPTRTYPE: {
      fprintf(where, "FUNPTRTYPE(%zu, %zu, ", n->line, n->character);
      nodeDump(where, n->data.funPtrType.returnType);
      for (size_t idx = 0; idx < n->data.funPtrType.argTypes->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.funPtrType.argTypes->elements[idx]);
      }
      for (size_t idx = 0; idx < n->data.funPtrType.argNames->size; ++idx) {
        fprintf(where, ", ");
        nodeDump(where, n->data.funPtrType.argNames->elements[idx]);
      }
      fprintf(where, ")");
      break;
    }
    case NT_SCOPEDID: {
      char *idString = stringifyId(n);
      if (n->data.scopedId.entry == NULL)
        fprintf(where, "SCOPEDID(%zu, %zu, %s, REFERENCES())", n->line,
                n->character, idString);
      else
        fprintf(
            where, "SCOPEDID(%zu, %zu, %s, REFERENCES(%s, %zu, %zu))", n->line,
            n->character, idString, n->data.scopedId.entry->file->inputFilename,
            n->data.scopedId.entry->line, n->data.scopedId.entry->character);
      free(idString);
      break;
    }
    case NT_ID: {
      if (n->data.id.entry == NULL)
        fprintf(where, "ID(%zu, %zu, %s, REFERENCES())", n->line, n->character,
                n->data.id.id);
      else
        fprintf(where, "ID(%zu, %zu, %s, REFERENCES(%s, %zu, %zu))", n->line,
                n->character, n->data.id.id,
                n->data.id.entry->file->inputFilename, n->data.id.entry->line,
                n->data.id.entry->character);
      break;
    }
    case NT_UNPARSED: {
      break;  // not ever encountered!
    }
  }
}

void astDump(FILE *where, FileListEntry *entry) {
  fprintf(where, "%s (%s):\n", entry->inputFilename,
          entry->isCode ? "code" : "declaration");
  nodeDump(where, entry->ast);
  fprintf(where, "\n");
}