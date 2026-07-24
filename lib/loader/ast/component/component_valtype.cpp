// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadExternName(std::string &Name) {
  std::vector<std::string> Ignored;
  return loadExternName(Name, Ignored);
}

Expect<void> Loader::loadExternName(std::string &Name,
                                    std::vector<std::string> &Implements) {
  // importname' ::= 0x00 len:<u32> in:<importname> => in (if len = |in|)
  //               | 0x02 len:<u32> in:<importname> a*:vec(<attribute>)
  // attribute   ::= 0x00 len:<u32> in:<interfacename> => (implements in)
  //               | 0x01 len:<u32> vs:<semversuffix>  => (versionsuffix vs)
  //               | 0x02 n:<name>                     => (external-id n)

  // Error messages will be handled in the parent scope.
  EXPECTED_TRY(auto B, FMgr.readByte());
  if (B != 0x00 && B != 0x02) {
    return Unexpect(ErrCode::Value::MalformedName);
  }
  EXPECTED_TRY(Name, FMgr.readName());
  if (B == 0x02) {
    EXPECTED_TRY(auto Cnt, FMgr.readU32());
    for (uint32_t I = 0; I < Cnt; ++I) {
      EXPECTED_TRY(auto Opt, FMgr.readByte());
      if (Opt == 0x00) {
        EXPECTED_TRY(auto Impl, FMgr.readName());
        Implements.push_back(std::move(Impl));
      } else if (Opt == 0x02) {
        // The external-id value is ignored during validation and execution.
        EXPECTED_TRY(FMgr.readName());
      } else {
        // versionsuffix is not implemented.
        return Unexpect(ErrCode::Value::MalformedName);
      }
    }
  }
  return {};
}

Expect<void> Loader::loadType(ComponentValType &Ty) {
  // valtype ::= i:<typeidx>       => i
  //           | pvt:<primvaltype> => pvt

  EXPECTED_TRY(int64_t Val, FMgr.readS33().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_ValueType);
  }));
  if (Val < 0) {
    // PrimValType case.
    if (Val < -64) {
      // Check for an invalid s33 value larger than 1 byte.
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
      Ty.setCode(static_cast<ComponentTypeCode>(PVT));
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

  ComponentValType VT;
  EXPECTED_TRY(loadType(VT).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_LabelValType);
  }));
  Ty.setValType(VT);

  return {};
}

} // namespace Loader
} // namespace WasmEdge
