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

// Implementation of type checking

#include "typecheck/buildSymbolTable.h"

#include <stdlib.h>

// helpers
static Type *astToType(Node const *ast, Report *report, Options const *options,
                       Environment const *env, char const *filename) {
  switch (ast->type) {
    case NT_KEYWORDTYPE: {
      return keywordTypeCreate(ast->data.typeKeyword.type - TK_VOID + K_VOID);
    }
    case NT_ID: {
      SymbolInfo *info = environmentLookup(env, report, ast->data.id.id,
                                           ast->line, ast->character, filename);
      return info == NULL
                 ? NULL
                 : referneceTypeCreate(
                       info->data.type.kind - TDK_STRUCT + K_STRUCT, info);
    }
    case NT_CONSTTYPE: {
      if (ast->data.constType.target->type == NT_CONSTTYPE) {
        switch (optionsGet(options, optionWDuplicateDeclSpecifier)) {
          case O_WT_ERROR: {
            reportError(report,
                        "%s:%zu:%zu: error: duplciate 'const' specifier",
                        filename, ast->line, ast->character);
            return NULL;
          }
          case O_WT_WARN: {
            reportWarning(report,
                          "%s:%zu:%zu: warning: duplciate 'const' specifier",
                          filename, ast->line, ast->character);
            return astToType(ast->data.constType.target, report, options, env,
                             filename);
          }
          case O_WT_IGNORE: {
            return astToType(ast->data.constType.target, report, options, env,
                             filename);
          }
        }
      }

      Type *subType =
          astToType(ast->data.constType.target, report, options, env, filename);
      return subType == NULL ? NULL : modifierTypeCreate(K_PTR, subType);
    }
    case NT_ARRAYTYPE: {
      Node const *sizeConst = ast->data.arrayType.size;
      switch (sizeConst->data.constExp.type) {
        case CT_UBYTE:
        case CT_USHORT:
        case CT_UINT:
        case CT_ULONG: {
          break;
        }
        default: {
          reportError(report,
                      "%s:%zu:%zu: error: expected an unsigned integer for an "
                      "array size, but found %s",
                      filename, sizeConst->line, sizeConst->character,
                      constTypeToString(sizeConst->data.constExp.type));
          return NULL;
        }
      }
      size_t size;
      switch (sizeConst->data.constExp.type) {
        case CT_UBYTE: {
          size = sizeConst->data.constExp.value.ubyteVal;
          break;
        }
        case CT_USHORT: {
          size = sizeConst->data.constExp.value.ushortVal;
          break;
        }
        case CT_UINT: {
          size = sizeConst->data.constExp.value.uintVal;
          break;
        }
        case CT_ULONG: {
          size = sizeConst->data.constExp.value.ulongVal;
          break;
        }
        default: {
          return NULL;  // error: mutation in const ptr
        }
      }
      Type *subType = astToType(ast->data.arrayType.element, report, options,
                                env, filename);
      return subType == NULL ? NULL : arrayTypeCreate(subType, size);
    }
    case NT_PTRTYPE: {
      Type *subType =
          astToType(ast->data.ptrType.target, report, options, env, filename);
      return subType == NULL ? NULL : modifierTypeCreate(K_PTR, subType);
    }
    case NT_FNPTRTYPE: {
      Type *retType = astToType(ast->data.fnPtrType.returnType, report, options,
                                env, filename);
      if (retType == NULL) {
        return NULL;
      }

      TypeVector *argTypes = typeVectorCreate();
      for (size_t idx = 0; idx < ast->data.fnPtrType.argTypes->size; idx++) {
        Type *argType = astToType(ast->data.fnPtrType.argTypes->elements[idx],
                                  report, options, env, filename);
        if (argType == NULL) {
          typeDestroy(retType);
        }

        typeVectorInsert(argTypes, argType);
      }

      return functionPtrTypeCreate(retType, argTypes);
    }
    default: {
      return NULL;  // error: not syntactically valid
    }
  }
}

// expression

// statement

// top level

// file level stuff
static void buildStabDecl(Report *report, Options const *options,
                          ModuleAstMap const *decls, Node const *ast) {
  // TODO: write this
}
static void buildStabCode(Report *report, Options const *options,
                          ModuleAstMap const *decls, Node const *ast) {
  // TODO: write this
}

void buildSymbolTables(Report *report, Options const *options,
                       ModuleAstMapPair const *asts) {
  for (size_t idx = 0; idx < asts->decls.size; idx++) {
    if (asts->decls.keys[idx] != NULL) {
      buildStabDecl(report, options, &asts->decls, asts->decls.values[idx]);
    }
  }
  for (size_t idx = 0; idx < asts->codes.size; idx++) {
    if (asts->codes.keys[idx] != NULL) {
      buildStabCode(report, options, &asts->decls, asts->decls.values[idx]);
    }
  }
}