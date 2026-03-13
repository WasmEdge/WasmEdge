// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/errcode.h"
#include "converter.h"

using namespace std::string_view_literals;

namespace WasmEdge::WAT {

// (type $id? deftype)
// deftype = func_type | struct_type | array_type | sub_type
Expect<void> Converter::convertTypeDefinition(Node N, AST::Module &Mod) {
  // Find the deftype child (skip identifier if present)
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "identifier"sv) {
      continue;
    }
    // This is the deftype: func_type, struct_type, array_type, or sub_type
    if (Type == "sub_type"sv) {
      EXPECTED_TRY(auto SubTy, convertSubType(Child));
      Mod.getTypeSection().getContent().push_back(std::move(SubTy));
    } else if (Type == "func_type"sv) {
      EXPECTED_TRY(auto FuncTy, convertFuncType(Child));
      // Wrap in a SubType (final by default)
      Mod.getTypeSection().getContent().emplace_back(std::move(FuncTy));
    } else if (Type == "struct_type"sv || Type == "array_type"sv) {
      // Build a SubType with the composite type
      AST::SubType SubTy;
      SubTy.setFinal(true);
      if (Type == "struct_type"sv) {
        EXPECTED_TRY(auto Fields, convertStructFields(Child));
        SubTy.getCompositeType().setStructType(std::move(Fields));
      } else {
        EXPECTED_TRY(auto Field, convertArrayField(Child));
        SubTy.getCompositeType().setArrayType(std::move(Field));
      }
      Mod.getTypeSection().getContent().push_back(std::move(SubTy));
    }
  }
  return {};
}

// (rec type_definition+)
Expect<void> Converter::convertRecType(Node N, AST::Module &Mod) {
  // Count the type definitions in this rec group
  uint32_t RecSize = 0;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    if (nodeType(N.namedChild(I)) == "type_definition"sv) {
      RecSize++;
    }
  }

  uint32_t StartIdx =
      static_cast<uint32_t>(Mod.getTypeSection().getContent().size());

  // Convert each type definition
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "type_definition"sv) {
      EXPECTED_TRY(convertTypeDefinition(Child, Mod));
    }
  }

  // Set recursive info on all types in this rec group
  if (RecSize > 1) {
    auto &Types = Mod.getTypeSection().getContent();
    for (uint32_t I = 0; I < RecSize; ++I) {
      Types[StartIdx + I].setRecursiveInfo(I, RecSize);
    }
  }

  return {};
}

// Helper: convert struct fields from a struct_type node
// (struct field*)
// field = (field $id? storage_type) | (field $id? mut_type)
Expect<std::vector<AST::FieldType>> Converter::convertStructFields(Node N) {
  std::vector<AST::FieldType> Fields;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) != "field"sv) {
      continue;
    }
    // Each field may contain an identifier, a storage type, or a mut_type
    for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
      Node FChild = Child.namedChild(J);
      auto FType = nodeType(FChild);
      if (FType == "identifier"sv) {
        continue;
      }
      if (FType == "mut_type"sv) {
        // (mut storage_type)
        EXPECTED_TRY(auto VT, convertStorageType(FChild.namedChild(0)));
        Fields.emplace_back(VT, ValMut::Var);
      } else {
        // Direct storage type (valtype or packed_type)
        EXPECTED_TRY(auto VT, convertStorageType(FChild));
        Fields.emplace_back(VT, ValMut::Const);
      }
    }
  }
  return Fields;
}

// Helper: convert array field from an array_type node
// (array storage_type | mut_type)
Expect<AST::FieldType> Converter::convertArrayField(Node N) {
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "mut_type"sv) {
      EXPECTED_TRY(auto VT, convertStorageType(Child.namedChild(0)));
      return AST::FieldType(VT, ValMut::Var);
    } else {
      // storage_type (valtype or packed_type)
      EXPECTED_TRY(auto VT, convertStorageType(Child));
      return AST::FieldType(VT, ValMut::Const);
    }
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// Helper: convert a storage type (valtype or packed_type)
Expect<ValType> Converter::convertStorageType(Node N) {
  auto Type = nodeType(N);
  if (Type == "packed_type"sv) {
    auto Text = nodeText(N);
    if (Text == "i8"sv) {
      return ValType(TypeCode::I8);
    } else if (Text == "i16"sv) {
      return ValType(TypeCode::I16);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  // Otherwise it's a valtype
  return convertValType(N);
}

// (sub final? type_idx* (func_type | struct_type | array_type))
Expect<AST::SubType> Converter::convertSubType(Node N) {
  AST::SubType SubTy;
  // Default: not final (sub without final keyword is open)
  bool IsFinal = false;

  // Check for 'final' keyword among anonymous children
  for (uint32_t I = 0; I < N.childCount(); ++I) {
    Node Child = N.child(I);
    if (nodeText(Child) == "final"sv) {
      IsFinal = true;
      break;
    }
  }
  SubTy.setFinal(IsFinal);

  // Process named children
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "identifier"sv || Type == "nat"sv) {
      // Super type index
      EXPECTED_TRY(auto Idx, Syms.resolveType(nodeText(Child)));
      SubTy.getSuperTypeIndices().push_back(Idx);
    } else if (Type == "func_type"sv) {
      EXPECTED_TRY(auto FuncTy, convertFuncType(Child));
      SubTy.getCompositeType().setFunctionType(std::move(FuncTy));
    } else if (Type == "struct_type"sv) {
      EXPECTED_TRY(auto Fields, convertStructFields(Child));
      SubTy.getCompositeType().setStructType(std::move(Fields));
    } else if (Type == "array_type"sv) {
      EXPECTED_TRY(auto Field, convertArrayField(Child));
      SubTy.getCompositeType().setArrayType(std::move(Field));
    }
  }

  return SubTy;
}

// (func (param $id? valtype+)* (result valtype+)*)
Expect<AST::FunctionType> Converter::convertFuncType(Node N) {
  AST::FunctionType FuncTy;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    if (Type == "param"sv) {
      // (param $id? valtype+)
      for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
        Node PChild = Child.namedChild(J);
        if (nodeType(PChild) == "identifier"sv) {
          continue;
        }
        EXPECTED_TRY(auto VT, convertValType(PChild));
        FuncTy.getParamTypes().push_back(VT);
      }
    } else if (Type == "result"sv) {
      // (result valtype+)
      for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
        EXPECTED_TRY(auto VT, convertValType(Child.namedChild(J)));
        FuncTy.getReturnTypes().push_back(VT);
      }
    }
  }
  return FuncTy;
}

// Dispatches numtype/vectype/reftype
Expect<ValType> Converter::convertValType(Node N) {
  auto Type = nodeType(N);
  auto Text = nodeText(N);

  if (Type == "numtype"sv) {
    if (Text == "i32"sv) {
      return ValType(TypeCode::I32);
    } else if (Text == "i64"sv) {
      return ValType(TypeCode::I64);
    } else if (Text == "f32"sv) {
      return ValType(TypeCode::F32);
    } else if (Text == "f64"sv) {
      return ValType(TypeCode::F64);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  } else if (Type == "vectype"sv) {
    if (Text == "v128"sv) {
      return ValType(TypeCode::V128);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  } else if (Type == "reftype"sv || Type == "ref_type_short"sv ||
             Type == "ref_type_full"sv) {
    return convertRefType(N);
  }

  // May be a direct ref_type_short or ref_type_full child
  // Try as reftype
  return convertRefType(N);
}

// Handles (ref null? heaptype) and shorthand reftype keywords
Expect<ValType> Converter::convertRefType(Node N) {
  auto Type = nodeType(N);
  auto Text = nodeText(N);

  // Shorthand forms
  if (Type == "ref_type_short"sv) {
    if (Text == "funcref"sv) {
      return ValType(TypeCode::FuncRef);
    } else if (Text == "externref"sv) {
      return ValType(TypeCode::ExternRef);
    } else if (Text == "anyref"sv) {
      return ValType(TypeCode::AnyRef);
    } else if (Text == "eqref"sv) {
      return ValType(TypeCode::EqRef);
    } else if (Text == "i31ref"sv) {
      return ValType(TypeCode::I31Ref);
    } else if (Text == "structref"sv) {
      return ValType(TypeCode::StructRef);
    } else if (Text == "arrayref"sv) {
      return ValType(TypeCode::ArrayRef);
    } else if (Text == "nullfuncref"sv) {
      return ValType(TypeCode::NullFuncRef);
    } else if (Text == "nullexternref"sv) {
      return ValType(TypeCode::NullExternRef);
    } else if (Text == "nullref"sv) {
      return ValType(TypeCode::NullRef);
    } else if (Text == "exnref"sv) {
      return ValType(TypeCode::ExnRef);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  // If this is a reftype wrapper, descend to its child
  if (Type == "reftype"sv) {
    if (N.namedChildCount() > 0) {
      return convertRefType(N.namedChild(0));
    }
    // Could be an anonymous shorthand — check text
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  // Full form: (ref null? heap_type)
  if (Type == "ref_type_full"sv) {
    bool IsNull = false;
    // Check for 'null' keyword in anonymous children
    for (uint32_t I = 0; I < N.childCount(); ++I) {
      if (nodeText(N.child(I)) == "null"sv) {
        IsNull = true;
        break;
      }
    }

    // Find the heap_type named child
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "heap_type"sv) {
        EXPECTED_TRY(auto HT, convertHeapType(Child));
        // HT is a ValType representing the heap type
        // We need to construct (ref null? heaptype)
        TypeCode RefCode = IsNull ? TypeCode::RefNull : TypeCode::Ref;
        if (HT.getHeapTypeCode() == TypeCode::TypeIndex) {
          // Type index heap type
          return ValType(RefCode, HT.getTypeIndex());
        } else {
          // Abstract heap type
          return ValType(RefCode, HT.getHeapTypeCode());
        }
      }
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// Handles heap type keywords and type indices
Expect<ValType> Converter::convertHeapType(Node N) {
  auto Text = nodeText(N);
  auto Type = nodeType(N);

  // If this is a heap_type wrapper, get the text of the actual content
  // The heap_type node may have a named child (identifier or nat)
  // or may just contain a keyword as an anonymous child
  if (Type == "heap_type"sv) {
    if (N.namedChildCount() > 0) {
      Node Child = N.namedChild(0);
      auto ChildType = nodeType(Child);
      if (ChildType == "identifier"sv || ChildType == "nat"sv) {
        EXPECTED_TRY(auto Idx, Syms.resolveType(nodeText(Child)));
        return ValType(TypeCode::RefNull, Idx);
      }
    }
    // It's a keyword — get text from the anonymous child
    // The heap_type node text includes the keyword
    Text = nodeText(N);
  }

  // Abstract heap types
  if (Text == "func"sv) {
    return ValType(TypeCode::RefNull, TypeCode::FuncRef);
  } else if (Text == "extern"sv) {
    return ValType(TypeCode::RefNull, TypeCode::ExternRef);
  } else if (Text == "any"sv) {
    return ValType(TypeCode::RefNull, TypeCode::AnyRef);
  } else if (Text == "eq"sv) {
    return ValType(TypeCode::RefNull, TypeCode::EqRef);
  } else if (Text == "i31"sv) {
    return ValType(TypeCode::RefNull, TypeCode::I31Ref);
  } else if (Text == "struct"sv) {
    return ValType(TypeCode::RefNull, TypeCode::StructRef);
  } else if (Text == "array"sv) {
    return ValType(TypeCode::RefNull, TypeCode::ArrayRef);
  } else if (Text == "none"sv) {
    return ValType(TypeCode::RefNull, TypeCode::NullRef);
  } else if (Text == "noextern"sv) {
    return ValType(TypeCode::RefNull, TypeCode::NullExternRef);
  } else if (Text == "nofunc"sv) {
    return ValType(TypeCode::RefNull, TypeCode::NullFuncRef);
  } else if (Text == "exn"sv) {
    return ValType(TypeCode::RefNull, TypeCode::ExnRef);
  } else if (Text == "noexn"sv) {
    return ValType(TypeCode::RefNull, TypeCode::NullExnRef);
  }

  // Try as type index (identifier or nat)
  if (Text.size() > 0 &&
      (Text[0] == '$' || (Text[0] >= '0' && Text[0] <= '9'))) {
    EXPECTED_TRY(auto Idx, Syms.resolveType(Text));
    return ValType(TypeCode::RefNull, Idx);
  }

  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

} // namespace WasmEdge::WAT
