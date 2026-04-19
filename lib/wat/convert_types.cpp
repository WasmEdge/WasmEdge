// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/errcode.h"
#include "converter.h"
#include "wat/wat_util.h"

#include <utility>

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
  auto KW = peekKeyword(FC);
  uint32_t TIdx =
      static_cast<uint32_t>(Mod.getTypeSection().getContent().size());
  // The binary loader accepts only a func type code before the GC proposal.
  if (KW == "sub"sv || KW == "struct"sv || KW == "array"sv) {
    EXPECTED_TRY(needProposal(Proposal::GC, ErrCode::Value::IntegerTooLong));
  }
  if (KW == "sub"sv) {
    EXPECTED_TRY(auto SubTy, convertSubType(Child, TIdx));
    Mod.getTypeSection().getContent().push_back(std::move(SubTy));
  } else {
    // This is a comptype. convertCompType rejects a keyword that is not func,
    // struct, or array, so that the reserved type index is not dropped
    // without an error.
    AST::SubType SubTy;
    SubTy.setFinal(true);
    EXPECTED_TRY(convertCompType(Child, KW, TIdx, SubTy.getCompositeType()));
    Mod.getTypeSection().getContent().push_back(std::move(SubTy));
  }
  C.next();
  EXPECTED_TRY(expectEnd(C));
  return {};
}

// comptype ::= ( func functype ) | ( struct structtype ) | ( array arraytype )
// N is the comptype sexpr, and KW is its first keyword.
Expect<void> Converter::convertCompType(Node N, std::string_view KW,
                                        uint32_t TypeIdx,
                                        AST::CompositeType &CompType) {
  if (KW == "func"sv) {
    EXPECTED_TRY(auto FuncTy, convertFuncType(N));
    CompType.setFunctionType(std::move(FuncTy));
  } else if (KW == "struct"sv) {
    EXPECTED_TRY(auto Fields, convertStructFields(N, TypeIdx));
    CompType.setStructType(std::move(Fields));
  } else if (KW == "array"sv) {
    EXPECTED_TRY(auto Field, convertArrayField(N));
    CompType.setArrayType(std::move(Field));
  } else {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  return {};
}

// rectype ::= ( rec typedef+ )
Expect<void> Converter::convertRecType(Node N, AST::Module &Mod) {
  EXPECTED_TRY(needProposal(Proposal::GC, ErrCode::Value::IntegerTooLong));
  uint32_t StartIdx =
      static_cast<uint32_t>(Mod.getTypeSection().getContent().size());
  uint32_t RecSize = 0;

  // Convert each (type ...) child. Any other child is malformed.
  Cursor C(N);
  C.next(); // Skip the "rec" keyword.
  while (C.valid()) {
    if (!sexprMatch(C, "type"sv)) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(convertTypedef(C.node(), Mod));
    ++RecSize;
    C.next();
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

// Convert the operand of a (mut X) sexpr with Convert. The sexpr must have
// exactly one operand.
Expect<ValType>
Converter::convertMutOperand(Node MutNode,
                             Expect<ValType> (Converter::*Convert)(Node)) {
  Cursor MC(MutNode);
  MC.next(); // Skip the "mut" keyword.
  if (!MC.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto VT, (this->*Convert)(MC.node()));
  MC.next();
  EXPECTED_TRY(expectEnd(MC));
  return VT;
}

// Parse a (mut storagetype) sexpr into a mutable field type.
Expect<AST::FieldType> Converter::parseMutField(Node MutNode) {
  EXPECTED_TRY(auto VT,
               convertMutOperand(MutNode, &Converter::convertStorageType));
  return AST::FieldType(VT, ValMut::Var);
}

// structtype ::= (field id? fieldtype)*
// fieldtype  ::= storagetype | ( mut storagetype )
// storagetype ::= valtype | packtype
// packtype    ::= i8 | i16
Expect<std::vector<AST::FieldType>>
Converter::convertStructFields(Node N, uint32_t TypeIdx) {
  std::vector<AST::FieldType> Fields;

  Cursor SC(N);
  SC.next(); // Skip the "struct" keyword.
  for (; SC.valid(); SC.next()) {
    // An arraytype has an abbreviation without the "field" keyword, but a
    // structtype has no such abbreviation. Reject any member that is not a
    // (field ...) group, for example a bare i32, (mut ...), or (ref ...).
    if (!sexprMatch(SC, "field"sv)) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Cursor CC(SC.node());
    // The form is (field id fieldtype) or (field fieldtype*). An id names
    // exactly one field, as with (param ...) and (local ...).
    CC.next(); // Skip the "field" keyword.
    bool HasName = false;
    if (peekType(CC) == NodeType::Id) {
      EXPECTED_TRY(auto FieldName, decodeIdentifier(nodeText(CC.node())));
      if (!Syms.FieldNames[TypeIdx]
               .emplace(std::move(FieldName),
                        static_cast<uint32_t>(Fields.size()))
               .second) {
        return Unexpect(ErrCode::Value::WatDuplicateField);
      }
      HasName = true;
      CC.next();
      if (!CC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }
    while (CC.valid()) {
      EXPECTED_TRY(auto FT, convertFieldType(CC.node()));
      Fields.push_back(FT);
      CC.next();
      if (HasName && CC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }
  }
  return Fields;
}

// fieldtype ::= storagetype | ( mut storagetype )
Expect<AST::FieldType> Converter::convertFieldType(Node N) {
  if (sexprMatch(N, "mut"sv)) {
    return parseMutField(N);
  }
  EXPECTED_TRY(auto VT, convertStorageType(N));
  return AST::FieldType(VT, ValMut::Const);
}

// arraytype ::= fieldtype
// An arraytype holds exactly one fieldtype.
Expect<AST::FieldType> Converter::convertArrayField(Node N) {
  Cursor AC(N);
  AC.next(); // Skip the "array" keyword.
  if (!AC.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto FT, convertFieldType(AC.node()));
  AC.next();
  EXPECTED_TRY(expectEnd(AC));
  return FT;
}

// storagetype ::= valtype | i8 | i16
Expect<ValType> Converter::convertStorageType(Node N) {
  auto Type = nodeType(N);
  if (Type == NodeType::Keyword) {
    auto Text = nodeText(N);
    if (Text == "i8"sv) {
      return ValType(TypeCode::I8);
    }
    if (Text == "i16"sv) {
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
  if (peekKeyword(C) == "sub"sv) {
    C.next();
  }

  // Look for the 'final' keyword. A 'sub' without 'final' gives an open type.
  bool IsFinal = false;
  if (peekKeyword(C) == "final"sv) {
    IsFinal = true;
    C.next();
  }
  SubTy.setFinal(IsFinal);

  // Consume the super type indices, which are Id or U, until a comptype sexpr
  // comes next.
  while (isIndexOrId(C.node())) {
    EXPECTED_TRY(auto Idx, takeTypeIdx(C));
    SubTy.getSuperTypeIndices().push_back(Idx);
  }

  // Consume the comptype sexpr. The comptype is mandatory. Without it, the
  // composite type of the SubType stays indeterminate.
  if (peekType(C) != NodeType::Sexpr) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  Node Child = C.node();
  Cursor FC(Child);
  auto KW = peekKeyword(FC);
  EXPECTED_TRY(convertCompType(Child, KW, TypeIdx, SubTy.getCompositeType()));
  C.next();
  EXPECTED_TRY(expectEnd(C));

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
    // A functype holds only (param ...) and (result ...) groups. Any other
    // child is malformed. A silent drop corrupts the signature.
    if (nodeType(Child) != NodeType::Sexpr) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Cursor CC(Child);
    auto KW = peekKeyword(CC);
    if (KW == "param"sv) {
      if (SeenResult) {
        // Without the MultiValue proposal, wasm-1.0 reports a result before a
        // parameter. From wasm-2.0, the report is an unexpected token.
        if (!Conf.hasProposal(Proposal::MultiValue)) {
          return Unexpect(ErrCode::Value::WatResultBeforeParam);
        }
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      // The form is (param id valtype) or (param valtype*).
      Cursor PC(Child);
      PC.next(); // Skip the "param" keyword.
      EXPECTED_TRY(parseParamList(PC, FuncTy.getParamTypes(),
                                  /*AcceptParamId=*/true, nullptr,
                                  ErrCode::Value::WatUnexpectedToken));
    } else if (KW == "result"sv) {
      SeenResult = true;
      // The form is (result valtype*). It does not permit an identifier.
      Cursor RC(Child);
      RC.next(); // Skip the "result" keyword.
      EXPECTED_TRY(parseResultList(RC, FuncTy.getReturnTypes()));
    } else {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
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
    }
    if (Text == "i64"sv) {
      return ValType(TypeCode::I64);
    }
    if (Text == "f32"sv) {
      return ValType(TypeCode::F32);
    }
    if (Text == "f64"sv) {
      return ValType(TypeCode::F64);
    }
    // This is the vectype keyword.
    if (Text == "v128"sv) {
      EXPECTED_TRY(
          needProposal(Proposal::SIMD, ErrCode::Value::MalformedValType));
      return ValType(TypeCode::V128);
    }
    // These are the shorthand reftype keywords. A proposal error passes
    // through. Only the "not a reftype" error falls through to the tests
    // below.
    if (auto VT = convertRefType(N); VT) {
      return *VT;
    } else if (VT.error() != ErrCode::Value::WatUnexpectedToken) {
      return Unexpect(VT.error());
    }
    // This type keyword is obsolete.
    if (Text == "anyfunc"sv) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  if (Type == NodeType::Sexpr) {
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
    // The gates are the gates of the binary loader. The funcref keyword is a
    // part of the core spec.
    if (Text == "funcref"sv) {
      return ValType(TypeCode::FuncRef);
    }
    if (Text == "externref"sv) {
      EXPECTED_TRY(needProposal(Proposal::ReferenceTypes, refTypeFailCode()));
      return ValType(TypeCode::ExternRef);
    }
    if (Text == "exnref"sv || Text == "nullexnref"sv) {
      EXPECTED_TRY(needProposal(Proposal::ExceptionHandling,
                                ErrCode::Value::MalformedValType));
      return ValType(Text == "exnref"sv ? TypeCode::ExnRef
                                        : TypeCode::NullExnRef);
    }
    static constexpr std::pair<std::string_view, TypeCode> GcRefTypes[] = {
        {"anyref"sv, TypeCode::AnyRef},
        {"eqref"sv, TypeCode::EqRef},
        {"i31ref"sv, TypeCode::I31Ref},
        {"structref"sv, TypeCode::StructRef},
        {"arrayref"sv, TypeCode::ArrayRef},
        {"nullfuncref"sv, TypeCode::NullFuncRef},
        {"nullexternref"sv, TypeCode::NullExternRef},
        {"nullref"sv, TypeCode::NullRef},
    };
    if (const auto Code = lookupKeyword(GcRefTypes, Text)) {
      EXPECTED_TRY(needProposal(Proposal::GC, refTypeFailCode()));
      return ValType(*Code);
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  if (Type == NodeType::Sexpr) {
    return convertRefTypeSexpr(N);
  }
  // A bare type index is not a reftype. The grammar needs (ref $t).
  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

// reftype_sexpr ::= ( ref null? heaptype )
Expect<ValType> Converter::convertRefTypeSexpr(Node N) {
  Cursor C(N);
  if (peekType(C) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  if (peekKeyword(C) != "ref"sv) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  C.next();
  // The binary loader accepts the (ref null? ht) forms only with the Typed
  // Function References proposal.
  EXPECTED_TRY(needProposal(Proposal::FunctionReferences, refTypeFailCode()));

  // Look for the 'null' keyword.
  bool IsNull = false;
  if (peekKeyword(C) == "null"sv) {
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
  EXPECTED_TRY(expectEnd(C));
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

  // This is the type index, which is an identifier or a number.
  if (isIndexOrId(N)) {
    EXPECTED_TRY(needProposal(Proposal::FunctionReferences, refTypeFailCode()));
    EXPECTED_TRY(auto Idx, Syms.resolve(SymbolTable::IndexSpace::Type, Text));
    return ValType(TypeCode::RefNull, Idx);
  }

  // These are the abstract heap type keywords, with the proposal that each
  // keyword needs. The gates are the gates of the binary loader.
  static constexpr std::pair<std::string_view, std::pair<TypeCode, Proposal>>
      HeapTypes[] = {
          {"func"sv, {TypeCode::FuncRef, Proposal::ReferenceTypes}},
          {"extern"sv, {TypeCode::ExternRef, Proposal::ReferenceTypes}},
          {"any"sv, {TypeCode::AnyRef, Proposal::GC}},
          {"eq"sv, {TypeCode::EqRef, Proposal::GC}},
          {"i31"sv, {TypeCode::I31Ref, Proposal::GC}},
          {"struct"sv, {TypeCode::StructRef, Proposal::GC}},
          {"array"sv, {TypeCode::ArrayRef, Proposal::GC}},
          {"none"sv, {TypeCode::NullRef, Proposal::GC}},
          {"noextern"sv, {TypeCode::NullExternRef, Proposal::GC}},
          {"nofunc"sv, {TypeCode::NullFuncRef, Proposal::GC}},
          {"exn"sv, {TypeCode::ExnRef, Proposal::ExceptionHandling}},
          {"noexn"sv, {TypeCode::NullExnRef, Proposal::ExceptionHandling}},
      };
  if (const auto Found = lookupKeyword(HeapTypes, Text)) {
    const auto [Code, Prop] = *Found;
    if (Code == TypeCode::FuncRef) {
      // The func heap type is legal with Bulk Memory too, as in the loader.
      if (!Conf.hasProposal(Proposal::BulkMemoryOperations)) {
        EXPECTED_TRY(needProposal(Prop, refTypeFailCode()));
      }
    } else if (Prop == Proposal::ExceptionHandling) {
      EXPECTED_TRY(needProposal(Prop, ErrCode::Value::MalformedValType));
    } else {
      EXPECTED_TRY(needProposal(Prop, refTypeFailCode()));
    }
    return ValType(TypeCode::RefNull, Code);
  }

  return Unexpect(ErrCode::Value::WatUnexpectedToken);
}

} // namespace WAT
} // namespace WasmEdge
