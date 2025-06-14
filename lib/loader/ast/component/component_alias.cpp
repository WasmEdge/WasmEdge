// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadAlias(AST::Component::Alias &Alias) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Alias);
  };
  // alias ::= s:<sort> t:<aliastarget> => (alias t (s))

  EXPECTED_TRY(loadSort(Alias.getSort()).map_error(ReportError));
  EXPECTED_TRY(loadAliasTarget(Alias.getTarget()).map_error(ReportError));
  return {};
}

Expect<void> Loader::loadAliasTarget(AST::Component::AliasTarget &AliasTarget) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_AliasTarget);
  };
  // aliastarget ::= 0x00 i:<instanceidx> n:<string>         => export i n
  //               | 0x01 i:<core:instanceidx> n:<core:name> => core export i n
  //               | 0x02 ct:<u32> idx:<u32>                 => outer ct idx

  EXPECTED_TRY(auto Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00:
  case 0x01: {
    auto &Ex = AliasTarget.emplace<AST::Component::AliasTargetExport>();
    EXPECTED_TRY(Ex.getInstanceIdx(), FMgr.readU32().map_error(ReportError));
    EXPECTED_TRY(Ex.getName(), FMgr.readName().map_error(ReportError));
    return {};
  }
  case 0x02: {
    auto &Out = AliasTarget.emplace<AST::Component::AliasTargetOuter>();
    EXPECTED_TRY(Out.getComponent(), FMgr.readU32().map_error(ReportError));
    EXPECTED_TRY(Out.getIndex(), FMgr.readU32().map_error(ReportError));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedAliasTarget,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_AliasTarget);
  }
}

} // namespace Loader
} // namespace WasmEdge
