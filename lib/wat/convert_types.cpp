// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/errcode.h"
#include "converter.h"
#include "wat/wat_util.h"

#include <unordered_map>
#include <unordered_set>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

// typedef ::= ( type id? subtype )
// subtype  ::= ( sub final? typeidx* comptype )
//            | comptype   (abbreviation for final subtype)
// comptype ::= ( func functype ) | ( struct structtype )
//            | ( array arraytype )
Expect<void> Converter::convertTypedef(Node N, AST::Module &Mod) {
  Cursor C(N);
  // Skip the keyword "type" and the optional id.
  if (peekType(C) == NodeType::Keyword) {
    C.next();
  }
  if (peekType(C) == NodeType::Id) {
    C.next();
  }
  // The subtype sexpr or comptype sexpr is mandatory. Pass 1 already reserved
  // an index for this type. Without the sexpr, the type section and the symbol
  // table lose their agreement.
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedEnd);
  }
  Node Child = C.node();
  if (nodeType(Child) != NodeType::Sexpr) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  Cursor FC(Child);
  auto KW = peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
  uint32_t TIdx =
      static_cast<uint32_t>(Mod.getTypeSection().getContent().size());
  if (KW == "sub"sv) {
    EXPECTED_TRY(auto SubTy, convertSubType(Child, TIdx));
    Mod.getTypeSection().getContent().push_back(std::move(SubTy));
  } else if (KW == "func"sv || KW == "struct"sv || KW == "array"sv) {
    AST::SubType SubTy;
    SubTy.setFinal(true);
    if (KW == "func"sv) {
      EXPECTED_TRY(auto FuncTy, convertFuncType(Child));
      SubTy.getCompositeType().setFunctionType(std::move(FuncTy));
    } else if (KW == "struct"sv) {
      EXPECTED_TRY(auto Fields, convertStructFields(Child, TIdx));
      SubTy.getCompositeType().setStructType(std::move(Fields));
    } else if (KW == "array"sv) {
      EXPECTED_TRY(auto Field, convertArrayField(Child));
      SubTy.getCompositeType().setArrayType(std::move(Field));
    }
    Mod.getTypeSection().getContent().push_back(std::move(SubTy));
  } else {
    // This is not a sub, func, struct, or array comptype. Reject it, and do
    // not drop the reserved type index.
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  return {};
}

// rectype ::= ( rec typedef+ )
Expect<void> Converter::convertRecType(Node N, AST::Module &Mod) {
  uint32_t StartIdx =
      static_cast<uint32_t>(Mod.getTypeSection().getContent().size());
  uint32_t RecSize = 0;

  // Convert each (type ...) child.
  Cursor C(N);
  while (C.valid()) {
    Node Child = C.node();
    C.next();
    Cursor FC(Child);
    auto KW = peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
    if (nodeType(Child) == NodeType::Sexpr && KW == "type"sv) {
      EXPECTED_TRY(convertTypedef(Child, Mod));
      ++RecSize;
    }
  }

  // Give the recursive info to every type of this rec group.
  if (RecSize > 1) {
    auto &Types = Mod.getTypeSection().getContent();
    for (uint32_t I = 0; I < RecSize; ++I) {
      Types[StartIdx + I].setRecursiveInfo(I, RecSize);
    }
  }
  return {};
}

// Parse a (mut storagetype) sexpr. The function returns {ValType, Var}.
Expect<AST::FieldType> Converter::parseMutField(Node MutNode) {
  Cursor MC(MutNode);
  MC.next(); // Skip the "mut" keyword.
  if (!MC.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  Node MChild = MC.node();
  EXPECTED_TRY(auto VT, convertStorageType(MChild));
  return AST::FieldType(VT, ValMut::Var);
}

// structtype ::= (field id? fieldtype)* | storagetype*
// fieldtype  ::= storagetype | ( mut storagetype )
// storagetype ::= valtype | packtype
// packtype    ::= i8 | i16
Expect<std::vector<AST::FieldType>>
Converter::convertStructFields(Node N, uint32_t TypeIdx) {
  std::vector<AST::FieldType> Fields;
  std::unordered_set<std::string, Hash::Hash> DupCheck;

  Cursor SC(N);
  SC.next(); // Skip the "struct" keyword.
  while (SC.valid()) {
    Node Child = SC.node();
    SC.next();
    auto CType = nodeType(Child);
    if (CType == NodeType::Keyword) {
      // A bare storage-type keyword, such as (struct i32), is not valid. An
      // arraytype has an abbreviation without the "field" keyword, but a
      // structtype has no such abbreviation. Each member needs a (field ...)
      // wrapper.
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    } else if (CType == NodeType::Sexpr) {
      Cursor CC(Child);
      auto KW = peekType(CC) == NodeType::Keyword ? nodeText(CC.node()) : ""sv;
      if (KW == "field"sv) {
        // The form is (field $id? fieldtype) or (field storagetype+).
        std::string FieldName;
        CC.next(); // Skip the "field" keyword.
        while (CC.valid()) {
          Node FChild = CC.node();
          CC.next();
          auto FType = nodeType(FChild);
          if (FType == NodeType::Id) {
            EXPECTED_TRY(FieldName, decodeIdentifier(nodeText(FChild)));
            if (!DupCheck.insert(FieldName).second) {
              return Unexpect(ErrCode::Value::WatDuplicateField);
            }
            Syms.FieldNames[TypeIdx][FieldName] =
                static_cast<uint32_t>(Fields.size());
          } else if (FType == NodeType::Sexpr) {
            Cursor FC(FChild);
            auto FKWI =
                peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
            if (FKWI == "mut"sv) {
              EXPECTED_TRY(auto FT, parseMutField(FChild));
              Fields.push_back(FT);
            } else {
              EXPECTED_TRY(auto VT, convertStorageType(FChild));
              Fields.emplace_back(VT, ValMut::Const);
            }
            FieldName.clear();
          } else {
            EXPECTED_TRY(auto VT, convertStorageType(FChild));
            Fields.emplace_back(VT, ValMut::Const);
            FieldName.clear();
          }
        }
      } else {
        // Reject a group that is not a (field ...), for example a bare
        // (mut ...) or (ref ...). Each member of a structtype needs a
        // (field id? fieldtype) wrapper.
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }
  }
  return Fields;
}

// arraytype ::= fieldtype
// fieldtype ::= storagetype | ( mut storagetype )
Expect<AST::FieldType> Converter::convertArrayField(Node N) {
  Cursor AC(N);
  AC.next(); // Skip the "array" keyword.
  while (AC.valid()) {
    Node Child = AC.node();
    AC.next();
    auto CType = nodeType(Child);
    if (CType == NodeType::Keyword) {
      EXPECTED_TRY(auto VT, convertStorageType(Child));
      return AST::FieldType(VT, ValMut::Const);
    }
    if (CType == NodeType::Sexpr) {
      Cursor FC(Child);
      auto KW = peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
      if (KW == "mut"sv) {
        return parseMutField(Child);
      }
      EXPECTED_TRY(auto VT, convertStorageType(Child));
      return AST::FieldType(VT, ValMut::Const);
    }
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// storagetype ::= valtype | i8 | i16
Expect<ValType> Converter::convertStorageType(Node N) {
  auto Type = nodeType(N);
  if (Type == NodeType::Keyword) {
    auto Text = nodeText(N);
    if (Text == "i8"sv) {
      return ValType(TypeCode::I8);
    } else if (Text == "i16"sv) {
      return ValType(TypeCode::I16);
    }
  }
  return convertValType(N);
}

// subtype ::= ( sub final? typeidx* comptype )
Expect<AST::SubType> Converter::convertSubType(Node N, uint32_t TypeIdx) {
  AST::SubType SubTy;
  Cursor C(N);

  // Consume the 'sub' keyword.
  if (peekType(C) == NodeType::Keyword && nodeText(C.node()) == "sub"sv) {
    C.next();
  }

  // Look for the 'final' keyword. A 'sub' without 'final' gives an open type.
  bool IsFinal = false;
  if (peekType(C) == NodeType::Keyword && nodeText(C.node()) == "final"sv) {
    IsFinal = true;
    C.next();
  }
  SubTy.setFinal(IsFinal);

  // Consume the super type indices, which are Id or U, until a comptype sexpr
  // comes next.
  while (C.valid()) {
    auto PType = peekType(C);
    if (PType == NodeType::Id || PType == NodeType::U) {
      EXPECTED_TRY(auto Idx, Syms.resolveType(nodeText(C.node())));
      SubTy.getSuperTypeIndices().push_back(Idx);
      C.next();
    } else {
      break;
    }
  }

  // Consume the comptype sexpr.
  if (peekType(C) == NodeType::Sexpr) {
    Node Child = C.node();
    Cursor FC(Child);
    auto KW = peekType(FC) == NodeType::Keyword ? nodeText(FC.node()) : ""sv;
    if (KW == "func"sv) {
      EXPECTED_TRY(auto FuncTy, convertFuncType(Child));
      SubTy.getCompositeType().setFunctionType(std::move(FuncTy));
    } else if (KW == "struct"sv) {
      EXPECTED_TRY(auto Fields, convertStructFields(Child, TypeIdx));
      SubTy.getCompositeType().setStructType(std::move(Fields));
    } else if (KW == "array"sv) {
      EXPECTED_TRY(auto Field, convertArrayField(Child));
      SubTy.getCompositeType().setArrayType(std::move(Field));
    }
  }

  return SubTy;
}

// functype ::= (param id? valtype*)* (result valtype*)*
Expect<AST::FunctionType> Converter::convertFuncType(Node N) {
  AST::FunctionType FuncTy;
  bool SeenResult = false;

  Cursor FC(N);
  FC.next(); // Skip the first "func" keyword.
  while (FC.valid()) {
    Node Child = FC.node();
    FC.next();
    if (nodeType(Child) != NodeType::Sexpr) {
      continue;
    }
    Cursor CC(Child);
    auto KW = peekType(CC) == NodeType::Keyword ? nodeText(CC.node()) : ""sv;
    if (KW == "param"sv) {
      if (SeenResult) {
        // Without the MultiValue proposal, wasm-1.0 reports a result before a
        // parameter. From wasm-2.0, the report is an unexpected token.
        if (!Conf.hasProposal(Proposal::MultiValue)) {
          return Unexpect(ErrCode::Value::WatResultBeforeParam);
        }
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      // The form is (param $id? valtype+).
      Cursor PC(Child);
      PC.next(); // Skip the "param" keyword.
      while (PC.valid()) {
        if (nodeType(PC.node()) == NodeType::Id) {
          PC.next();
          continue; // Skip the name of the parameter.
        }
        EXPECTED_TRY(auto VT, convertValType(PC.node()));
        FuncTy.getParamTypes().push_back(VT);
        PC.next();
      }
    } else if (KW == "result"sv) {
      SeenResult = true;
      // The form is (result valtype+). It does not permit an identifier.
      Cursor RC(Child);
      RC.next(); // Skip the "result" keyword.
      while (RC.valid()) {
        if (nodeType(RC.node()) == NodeType::Id) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        EXPECTED_TRY(auto VT, convertValType(RC.node()));
        FuncTy.getReturnTypes().push_back(VT);
        RC.next();
      }
    }
  }
  return FuncTy;
}

// valtype  ::= numtype | vectype | reftype | typeidx
// numtype  ::= i32 | i64 | f32 | f64
// vectype  ::= v128
// reftype  ::= funcref | externref | ... | ( ref null? heaptype )
Expect<ValType> Converter::convertValType(Node N) {
  auto Type = nodeType(N);
  auto Text = nodeText(N);

  if (Type == NodeType::Keyword) {
    // These are the numtype keywords.
    if (Text == "i32"sv) {
      return ValType(TypeCode::I32);
    } else if (Text == "i64"sv) {
      return ValType(TypeCode::I64);
    } else if (Text == "f32"sv) {
      return ValType(TypeCode::F32);
    } else if (Text == "f64"sv) {
      return ValType(TypeCode::F64);
    }
    // This is the vectype keyword.
    else if (Text == "v128"sv) {
      return ValType(TypeCode::V128);
    }
    // These are the shorthand reftype keywords.
    else if (auto VT = convertRefType(N); VT) {
      return *VT;
    }
    // This type keyword is obsolete.
    if (Text == "anyfunc"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  } else if (Type == NodeType::Sexpr) {
    // The form is (ref null? heaptype).
    return convertRefTypeSexpr(N);
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// reftype ::= funcref | externref | anyref | eqref | i31ref
//           | structref | arrayref | nullfuncref | nullexternref
//           | nullref | exnref | nullexnref
//           | ( ref null? heaptype )
//           | typeidx
Expect<ValType> Converter::convertRefType(Node N) {
  auto Type = nodeType(N);
  auto Text = nodeText(N);
  if (Type == NodeType::Keyword) {
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
    } else if (Text == "nullexnref"sv) {
      return ValType(TypeCode::NullExnRef);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  } else if (Type == NodeType::Sexpr) {
    return convertRefTypeSexpr(N);
  } else if (Type == NodeType::Id || Type == NodeType::U) {
    // A bare type index is a reftype that is not nullable, such as (ref $t).
    // convertValType converts the nullable valtype.
    EXPECTED_TRY(auto Idx, Syms.resolveType(Text));
    return ValType(TypeCode::Ref, Idx);
  }
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// reftype_sexpr ::= ( ref null? heaptype )
Expect<ValType> Converter::convertRefTypeSexpr(Node N) {
  Cursor C(N);
  if (peekType(C) == NodeType::Keyword) {
    auto KW = nodeText(C.node());
    C.next();
    if (KW != "ref"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
  }

  // Look for the 'null' keyword.
  bool IsNull = false;
  if (peekType(C) == NodeType::Keyword && nodeText(C.node()) == "null"sv) {
    IsNull = true;
    C.next();
  }

  // Consume the heaptype, which is a keyword, an Id, or a U.
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  Node HTNode = C.node();
  EXPECTED_TRY(auto HT, convertHeapType(HTNode));
  // A (ref ...) type takes one heap type. Reject the operands that come after
  // it, for example (ref null func extern).
  C.next();
  if (C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  TypeCode RefCode = IsNull ? TypeCode::RefNull : TypeCode::Ref;
  if (HT.getHeapTypeCode() == TypeCode::TypeIndex) {
    return ValType(RefCode, HT.getTypeIndex());
  }
  return ValType(RefCode, HT.getHeapTypeCode());
}

// heaptype ::= func | extern | any | eq | i31 | struct
//            | array | none | noextern | nofunc | exn | noexn
//            | typeidx
Expect<ValType> Converter::convertHeapType(Node N) {
  auto Text = nodeText(N);
  auto Type = nodeType(N);

  // This is the type index, which is an identifier or a number.
  if (Type == NodeType::Id || Type == NodeType::U) {
    EXPECTED_TRY(auto Idx, Syms.resolveType(Text));
    return ValType(TypeCode::RefNull, Idx);
  }

  // These are the abstract heap type keywords.
  static const std::unordered_map<std::string_view, TypeCode, Hash::Hash>
      HeapTypeMap = {
          {"func"sv, TypeCode::FuncRef},
          {"extern"sv, TypeCode::ExternRef},
          {"any"sv, TypeCode::AnyRef},
          {"eq"sv, TypeCode::EqRef},
          {"i31"sv, TypeCode::I31Ref},
          {"struct"sv, TypeCode::StructRef},
          {"array"sv, TypeCode::ArrayRef},
          {"none"sv, TypeCode::NullRef},
          {"noextern"sv, TypeCode::NullExternRef},
          {"nofunc"sv, TypeCode::NullFuncRef},
          {"exn"sv, TypeCode::ExnRef},
          {"noexn"sv, TypeCode::NullExnRef},
      };
  auto It = HeapTypeMap.find(Text);
  if (It != HeapTypeMap.end()) {
    return ValType(TypeCode::RefNull, It->second);
  }

  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

} // namespace WAT
} // namespace WasmEdge
