// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void>
Loader::loadNameAttributes(std::string &Name,
                           std::vector<std::string> &Implements,
                           std::vector<std::string> &ExternalIds,
                           std::vector<std::string> &VersionSuffixes) {
  // nameattributes ::= 0x00 len:<u32> en:<externname> => en (if len = |en|)
  //                  | 0x01 len:<u32> en:<externname> => en (if len = |en|)
  //                  | 0x02 len:<u32> en:<externname> a*:vec(<attribute>)
  // attribute      ::= 0x00 len:<u32> in:<interfacename> => (implements in)
  //                  | 0x01 len:<u32> vs:<semversuffix>  => (versionsuffix vs)
  //                  | 0x02 n:<name>                     => (external-id n)

  // Error messages will be handled in the parent scope.
  EXPECTED_TRY(auto B, FMgr.readByte());
  if (B > 0x02) {
    return Unexpect(ErrCode::Value::MalformedName);
  }
  EXPECTED_TRY(Name, FMgr.readName());
  if (B == 0x02) {
    EXPECTED_TRY(auto Cnt, FMgr.readU32());
    for (uint32_t I = 0; I < Cnt; ++I) {
      EXPECTED_TRY(auto Opt, FMgr.readByte());
      if (Opt > 0x02) {
        return Unexpect(ErrCode::Value::MalformedName);
      }
      EXPECTED_TRY(auto Value, FMgr.readName());
      // Every kind keeps its values, so validation can hold each of them to
      // at most one occurrence.
      if (Opt == 0x00) {
        Implements.push_back(std::move(Value));
      } else if (Opt == 0x01) {
        VersionSuffixes.push_back(std::move(Value));
      } else {
        ExternalIds.push_back(std::move(Value));
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
    const auto Code = static_cast<ComponentTypeCode>(Val & INT64_C(0x7F));
    switch (Code) {
    case ComponentTypeCode::Bool:
    case ComponentTypeCode::S8:
    case ComponentTypeCode::U8:
    case ComponentTypeCode::S16:
    case ComponentTypeCode::U16:
    case ComponentTypeCode::S32:
    case ComponentTypeCode::U32:
    case ComponentTypeCode::S64:
    case ComponentTypeCode::U64:
    case ComponentTypeCode::F32:
    case ComponentTypeCode::F64:
    case ComponentTypeCode::Char:
    case ComponentTypeCode::String:
    case ComponentTypeCode::ErrContext:
      Ty.setCode(Code);
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
