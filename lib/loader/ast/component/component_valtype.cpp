// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadExternName(std::string &Name) {
  // importname' ::= 0x00 len:<u32> in:<importname> => in (if len = |in|)
  // exportname' ::= 0x00 len:<u32> en:<exportname> => en (if len = |en|)

  // Error messages will be handled in the parent scope.
  EXPECTED_TRY(auto B, FMgr.readByte());
  if (B != 0x00) {
    return Unexpect(ErrCode::Value::MalformedName);
  }
  EXPECTED_TRY(Name, FMgr.readName());
  return {};
}

Expect<void> Loader::loadType(AST::Component::ValueType &Ty) {
  // valtype ::= i:<typeidx>       => i
  //           | pvt:<primvaltype> => pvt

  EXPECTED_TRY(int64_t Val, FMgr.readS33().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_ValueType);
  }));
  if (Val < 0) {
    // PrimValType case.
    if (Val < -64) {
      // For checking the invalid s33 value which is larger than 1 byte.
      return logLoadError(ErrCode::Value::MalformedValType,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_ValueType);
    }
    AST::Component::PrimValType PVT = static_cast<AST::Component::PrimValType>(
        static_cast<uint8_t>(Val & INT64_C(0x7F)));
    switch (PVT) {
    case AST::Component::PrimValType::Bool:
    case AST::Component::PrimValType::S8:
    case AST::Component::PrimValType::U8:
    case AST::Component::PrimValType::S16:
    case AST::Component::PrimValType::U16:
    case AST::Component::PrimValType::S32:
    case AST::Component::PrimValType::U32:
    case AST::Component::PrimValType::S64:
    case AST::Component::PrimValType::U64:
    case AST::Component::PrimValType::F32:
    case AST::Component::PrimValType::F64:
    case AST::Component::PrimValType::Char:
    case AST::Component::PrimValType::String:
    case AST::Component::PrimValType::ErrorContext:
      Ty.setCode(PVT);
      break;
    default:
      return logLoadError(ErrCode::Value::MalformedValType,
                          FMgr.getLastOffset(), ASTNodeAttr::Comp_ValueType);
    }
  } else {
    // Type index case.
    Ty.setTypeIndex(static_cast<uint32_t>(Val));
  }
  return {};
}

Expect<void> Loader::loadType(AST::Component::LabelValType &Ty) {
  // labelvaltype ::= l:<label'> t:<valtype>
  // label'       ::= len:<u32> l:<label>    => l (if len = |l|)

  EXPECTED_TRY(std::string Label, FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_LabelValType);
  }));
  Ty.setLabel(Label);

  AST::Component::ValueType VT;
  EXPECTED_TRY(loadType(VT).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_LabelValType);
  }));
  Ty.setValType(VT);

  return {};
}

} // namespace Loader
} // namespace WasmEdge
