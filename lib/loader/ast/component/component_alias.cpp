// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadCoreAlias(AST::Component::CoreAlias &Alias) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Alias);
  };
  // core:alias       ::= s:<core:sort> t:<core:aliastarget> => (alias t (s))
  // core:aliastarget ::= 0x01 ct:<u32> idx:<u32>            => outer ct idx

  EXPECTED_TRY(loadCoreSort(Alias.getSort()).map_error(ReportError));
  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  if (Flag != 0x01) {
    return logLoadError(ErrCode::Value::MalformedAliasTarget,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Alias);
  }
  EXPECTED_TRY(uint32_t Ct, FMgr.readU32().map_error(ReportError));
  EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
  Alias.setComponentJump(Ct);
  Alias.setIndex(Idx);
  return {};
}

Expect<void> Loader::loadAlias(AST::Component::Alias &Alias) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Alias);
  };
  // alias       ::= s:<sort> t:<aliastarget>                => (alias t (s))
  // aliastarget ::= 0x00 i:<instanceidx> n:<string>         => export i n
  //               | 0x01 i:<core:instanceidx> n:<core:name> => core export i n
  //               | 0x02 ct:<u32> idx:<u32>                 => outer ct idx

  EXPECTED_TRY(loadSort(Alias.getSort()).map_error(ReportError));
  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00:
  case 0x01: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    EXPECTED_TRY(std::string Name, FMgr.readName().map_error(ReportError));
    Alias.setExport(Idx, Name);
    break;
  }
  case 0x02: {
    EXPECTED_TRY(uint32_t Ct, FMgr.readU32().map_error(ReportError));
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    Alias.setOuter(Ct, Idx);
    break;
  }
  default:
    return logLoadError(ErrCode::Value::MalformedAliasTarget,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_Alias);
  }
  Alias.setTargetType(static_cast<AST::Component::Alias::TargetType>(Flag));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
