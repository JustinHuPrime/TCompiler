// Copyright 2021 Justin Hu
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

#include "ast/typeManip.h"

#include "internalError.h"

bool typeIsBoolean(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      return t->data.keyword.keyword == TK_BOOL;
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsBoolean(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsSwitchable(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_UBYTE:
        case TK_BYTE:
        case TK_CHAR:
        case TK_USHORT:
        case TK_SHORT:
        case TK_UINT:
        case TK_INT:
        case TK_WCHAR:
        case TK_ULONG:
        case TK_LONG: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsSwitchable(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    case TK_REFERENCE: {
      return t->data.reference.entry->kind == SK_ENUMCONST;
    }
    default: {
      return false;
    }
  }
}

bool typeIsNumeric(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_UBYTE:
        case TK_BYTE:
        case TK_USHORT:
        case TK_SHORT:
        case TK_UINT:
        case TK_INT:
        case TK_ULONG:
        case TK_LONG:
        case TK_FLOAT:
        case TK_DOUBLE: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsNumeric(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsIntegral(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_UBYTE:
        case TK_BYTE:
        case TK_USHORT:
        case TK_SHORT:
        case TK_UINT:
        case TK_INT:
        case TK_ULONG:
        case TK_LONG: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsIntegral(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsUnsignedIntegral(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_UBYTE:
        case TK_USHORT:
        case TK_UINT:
        case TK_ULONG: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsUnsignedIntegral(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsSignedIntegral(Type const *t) {
  switch (t->kind) {
    case TK_KEYWORD: {
      switch (t->data.keyword.keyword) {
        case TK_BYTE:
        case TK_SHORT:
        case TK_INT:
        case TK_LONG: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsSignedIntegral(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

/**
 * produce true if `to *` is assignable to `from *`
 *
 * @param to type pointed to to assign to
 * @param from type pointed to to assign from
 * @returns if to * can be asigned to from *
 */
static bool pointerTypeIsAssignable(Type const *toBase, Type const *fromBase) {
  switch (toBase->kind) {
    case TK_KEYWORD: {
      switch (toBase->data.keyword.keyword) {
        case TK_VOID: {
          // row 3
          return true;
        }
        default: {
          // row 4-16
          switch (fromBase->kind) {
            case TK_KEYWORD: {
              switch (fromBase->data.keyword.keyword) {
                case TK_VOID: {
                  return true;
                }
                default: {
                  return toBase->data.keyword.keyword ==
                         fromBase->data.keyword.keyword;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
      }
    }
    case TK_REFERENCE: {
      // row 17-20
      switch (fromBase->kind) {
        case TK_KEYWORD: {
          return fromBase->data.keyword.keyword == TK_VOID;
        }
        case TK_REFERENCE: {
          if (toBase->data.reference.entry->kind == SK_OPAQUE &&
              fromBase->data.reference.entry->kind == SK_OPAQUE) {
            return toBase->data.reference.entry ==
                       fromBase->data.reference.entry ||
                   toBase->data.reference.entry ==
                       fromBase->data.reference.entry->data.opaqueType
                           .definition ||
                   toBase->data.reference.entry->data.opaqueType.definition ==
                       fromBase->data.reference.entry ||
                   toBase->data.reference.entry->data.opaqueType.definition ==
                       fromBase->data.reference.entry->data.opaqueType
                           .definition;
          } else if (toBase->data.reference.entry->kind == SK_OPAQUE) {
            return toBase->data.reference.entry ==
                       fromBase->data.reference.entry ||
                   toBase->data.reference.entry->data.opaqueType.definition ==
                       fromBase->data.reference.entry;
          } else if (fromBase->data.reference.entry->kind == SK_OPAQUE) {
            return toBase->data.reference.entry ==
                       fromBase->data.reference.entry ||
                   toBase->data.reference.entry ==
                       fromBase->data.reference.entry->data.opaqueType
                           .definition;
          } else {
            return toBase->data.reference.entry ==
                   fromBase->data.reference.entry;
          }
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (toBase->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          // row 21, 22
          return pointerTypeIsAssignable(toBase->data.modified.modified,
                                         fromBase);
        }
        case TM_POINTER: {
          // row 24
          return typeEqual(toBase, fromBase) ||
                 (fromBase->kind == TK_KEYWORD &&
                  fromBase->data.keyword.keyword == TK_VOID);
        }
        default: {
          error(__FILE__, __LINE__, "bad typemodifier encountered");
        }
      }
    }
    case TK_ARRAY: {
      // row 23
      return typeEqual(toBase, fromBase) ||
             (fromBase->kind == TK_KEYWORD &&
              fromBase->data.keyword.keyword == TK_VOID);
    }
    case TK_FUNPTR: {
      // row 25
      return typeEqual(toBase, fromBase) ||
             (fromBase->kind == TK_KEYWORD &&
              fromBase->data.keyword.keyword == TK_VOID);
    }
    default: {
      return false;  // can't form a pointer to that anyways
    }
  }
}

bool typeIsInitializable(Type const *to, Type const *from) {
  // implements appendix C
  switch (to->kind) {
    case TK_KEYWORD: {
      switch (to->data.keyword.keyword) {
        case TK_UBYTE: {
          // row 3
          switch (from->kind) {
            case TK_KEYWORD: {
              return from->data.keyword.keyword == TK_UBYTE;
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_BYTE: {
          // row 4
          switch (from->kind) {
            case TK_KEYWORD: {
              return from->data.keyword.keyword == TK_BYTE;
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_CHAR: {
          // row 5
          switch (from->kind) {
            case TK_KEYWORD: {
              return from->data.keyword.keyword == TK_CHAR;
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_USHORT: {
          // row 6
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_UBYTE:
                case TK_USHORT: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_SHORT: {
          // row 7
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_BYTE:
                case TK_UBYTE:
                case TK_SHORT: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_UINT: {
          // row 8
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_UBYTE:
                case TK_USHORT:
                case TK_UINT: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_INT: {
          // row 9
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_UBYTE:
                case TK_BYTE:
                case TK_USHORT:
                case TK_SHORT:
                case TK_INT: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_WCHAR: {
          // row 10
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_CHAR:
                case TK_WCHAR: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_ULONG: {
          // row 11
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_UBYTE:
                case TK_USHORT:
                case TK_UINT:
                case TK_ULONG: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_LONG: {
          // row 12
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_UBYTE:
                case TK_BYTE:
                case TK_USHORT:
                case TK_SHORT:
                case TK_UINT:
                case TK_INT:
                case TK_LONG: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_FLOAT: {
          // row 13
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_UBYTE:
                case TK_BYTE:
                case TK_USHORT:
                case TK_SHORT:
                case TK_UINT:
                case TK_INT:
                case TK_ULONG:
                case TK_LONG:
                case TK_FLOAT: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_DOUBLE: {
          // row 14
          switch (from->kind) {
            case TK_KEYWORD: {
              switch (from->data.keyword.keyword) {
                case TK_UBYTE:
                case TK_BYTE:
                case TK_USHORT:
                case TK_SHORT:
                case TK_UINT:
                case TK_INT:
                case TK_ULONG:
                case TK_LONG:
                case TK_FLOAT:
                case TK_DOUBLE: {
                  return true;
                }
                default: {
                  return false;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_BOOL: {
          // row 15
          switch (from->kind) {
            case TK_KEYWORD: {
              return from->data.keyword.keyword == TK_BOOL;
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case TK_VOID: {
          return false;  // can never assign to void!
        }
        default: {
          error(__FILE__, __LINE__, "bad typekeyword encountered");
        }
      }
    }
    case TK_REFERENCE: {
      switch (to->data.reference.entry->kind) {
        case SK_STRUCT: {
          // row 16
          switch (from->kind) {
            case TK_REFERENCE: {
              switch (from->data.reference.entry->kind) {
                case SK_OPAQUE: {
                  return to->data.reference.entry ==
                             from->data.reference.entry ||
                         to->data.reference.entry ==
                             from->data.reference.entry->data.opaqueType
                                 .definition;
                }
                default: {
                  return to->data.reference.entry == from->data.reference.entry;
                }
              }
            }
            case TK_AGGREGATE: {
              Vector const *types = &from->data.aggregate.types;
              if (to->data.reference.entry->data.structType.fieldTypes.size !=
                  types->size)
                return false;

              for (size_t idx = 0; idx < types->size; ++idx) {
                if (!typeIsInitializable(
                        to->data.reference.entry->data.structType.fieldTypes
                            .elements[idx],
                        types->elements[idx]))
                  return false;
              }

              return true;
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case SK_UNION:
        case SK_ENUM:
        case SK_TYPEDEF: {
          // row 17, 18, 19 (typedef case)
          switch (from->kind) {
            case TK_REFERENCE: {
              switch (from->data.reference.entry->kind) {
                case SK_OPAQUE: {
                  return to->data.reference.entry ==
                             from->data.reference.entry ||
                         to->data.reference.entry ==
                             from->data.reference.entry->data.opaqueType
                                 .definition;
                }
                default: {
                  return to->data.reference.entry == from->data.reference.entry;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        case SK_OPAQUE: {
          // row 19 (opaque to opaque case)
          switch (from->kind) {
            case TK_REFERENCE: {
              switch (from->data.reference.entry->kind) {
                case SK_OPAQUE: {
                  // probably a bit overkill - there's only one opaque reference
                  // per actual type
                  return to->data.reference.entry ==
                             from->data.reference.entry ||
                         to->data.reference.entry ==
                             from->data.reference.entry->data.opaqueType
                                 .definition ||
                         to->data.reference.entry->data.opaqueType.definition ==
                             from->data.reference.entry ||
                         to->data.reference.entry->data.opaqueType.definition ==
                             from->data.reference.entry->data.opaqueType
                                 .definition;
                }
                default: {
                  return to->data.reference.entry ==
                             from->data.reference.entry ||
                         to->data.reference.entry->data.opaqueType.definition ==
                             from->data.reference.entry;
                }
              }
            }
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                default: {
                  return false;
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        default: {
          error(__FILE__, __LINE__, "type to be assigned to isn't a type");
        }
      }
    }
    case TK_MODIFIED: {
      switch (to->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          // row 20, 21
          return typeIsInitializable(to->data.modified.modified, from);
        }
        case TM_POINTER: {
          // row 23
          switch (from->kind) {
            case TK_MODIFIED: {
              switch (from->data.modified.modifier) {
                case TM_CONST:
                case TM_VOLATILE: {
                  return typeIsInitializable(to, from->data.modified.modified);
                }
                case TM_POINTER: {
                  return pointerTypeIsAssignable(to->data.modified.modified,
                                                 from->data.modified.modified);
                }
                default: {
                  error(__FILE__, __LINE__, "bad type modifier encountered");
                }
              }
            }
            default: {
              return false;
            }
          }
        }
        default: {
          error(__FILE__, __LINE__, "bad type modifier encountered");
        }
      }
    }
    case TK_ARRAY: {
      // row 22
      switch (from->kind) {
        case TK_ARRAY: {
          return typeEqual(to, from);
        }
        case TK_AGGREGATE: {
          Vector const *types = &to->data.aggregate.types;
          if (to->data.array.length != types->size) return false;

          for (size_t idx = 0; idx < types->size; ++idx) {
            if (!typeIsInitializable(to->data.array.type, types->elements[idx]))
              return false;
          }

          return true;
        }
        case TK_MODIFIED: {
          switch (from->data.modified.modifier) {
            case TM_CONST:
            case TM_VOLATILE: {
              return typeIsInitializable(to, from->data.modified.modified);
            }
            default: {
              return false;
            }
          }
        }
        default: {
          return false;
        }
      }
    }
    case TK_FUNPTR: {
      // row 24
      switch (from->kind) {
        case TK_FUNPTR: {
          return typeEqual(to, from);
        }
        case TK_MODIFIED: {
          switch (from->data.modified.modifier) {
            case TM_CONST:
            case TM_VOLATILE: {
              return typeIsInitializable(to, from->data.modified.modified);
            }
            default: {
              return false;
            }
          }
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsAssignable(Type const *to, Type const *from) {
  return (to->kind != TK_MODIFIED || to->data.modified.modifier != TM_CONST) &&
         typeIsInitializable(to, from);
}

bool typeIsValuePointer(Type const *t) {
  switch (t->kind) {
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_POINTER: {
          return true;
        }
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsValuePointer(t->data.modified.modified);
        }
        default: {
          error(__FILE__, __LINE__, "invalid typeModifier enum");
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsComparable(Type const *lhs, Type const *rhs) {
  return false;  // TODO
}

bool typeIsCompound(Type const *t) {
  switch (t->kind) {
    case TK_REFERENCE: {
      switch (t->data.reference.entry->kind) {
        case SK_STRUCT:
        case SK_UNION: {
          return true;
        }
        default: {
          return false;
        }
      }
    }
    case TK_MODIFIED: {
      switch (t->data.modified.modifier) {
        case TM_CONST:
        case TM_VOLATILE: {
          return typeIsValuePointer(t->data.modified.modified);
        }
        default: {
          return false;
        }
      }
    }
    default: {
      return false;
    }
  }
}

bool typeIsArray(Type const *t) {
  return false;  // TODO
}

Type *typeMerge(Type const *lhs, Type const *rhs) {
  return NULL;  // TODO
}

Type *typeGetDereferenced(Type const *t) {
  return NULL;  // TODO
}

Type *typeGetArrayElement(Type const *t) {
  return NULL;  // TODO
}

Type *typeCopyCV(Type *to, Type const *from) {
  return NULL;  // TODO
}

size_t typeSizeof(Type const *t) {
  return 0;  // TODO
}

bool typeCastable(Type const *to, Type const *from) {
  return false;  // TODO
}