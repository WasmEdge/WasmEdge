// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<bool> Loader::loadExternName(std::string &Name,
                                    std::string &VersionSuffix) {
  // importname' ::= 0x00 len:<u32> in:<importname> => in
  //               | 0x01 len:<u32> in:<importname> => in
  //               | 0x02 len:<u32> in:<importname> opts:vec(<nameopt>)
  //                 => in opts
  // exportname' ::= 0x00 len:<u32> en:<exportname> => en
  //               | 0x01 len:<u32> en:<exportname> => en
  //               | 0x02 len:<u32> in:<importname> opts:vec(<nameopt>)
  //                 => in opts
  // nameopt     ::= 0x00 len:<u32> n:<interfacename> => (implements n)
  //               | 0x01 len:<u32> vs:<semversuffix> => (versionsuffix vs)

  // Error messages will be handled in the parent scope.
  EXPECTED_TRY(auto B, FMgr.readByte());
  if (B != 0x00 && B != 0x01 && B != 0x02) {
    return Unexpect(ErrCode::Value::MalformedName);
  }
  EXPECTED_TRY(Name, FMgr.readName());
  VersionSuffix.clear();
  if (B == 0x02) {
    EXPECTED_TRY(uint32_t OptCnt, FMgr.readU32());
    bool HasVersionSuffix = false;
    for (uint32_t I = 0; I < OptCnt; I++) {
      EXPECTED_TRY(uint8_t Opt, FMgr.readByte());
      switch (Opt) {
      case 0x00: {
        EXPECTED_TRY(std::string ImplementedInterface, FMgr.readName());
        break;
      }
      case 0x01: {
        EXPECTED_TRY(VersionSuffix, FMgr.readName());
        HasVersionSuffix = true;
        break;
      }
      default:
        return Unexpect(ErrCode::Value::MalformedName);
      }
    }
    return HasVersionSuffix;
  }
  return false;
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
