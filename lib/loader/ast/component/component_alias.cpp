// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadAlias(Alias &Alias) {
  // alias ::= s:<sort> t:<aliastarget>     => (alias t (s))
  return Expect<void>{}
      .and_then([&]() { return loadSort(Alias.getSort()); })
      .and_then([&]() { return loadAliasTarget(Alias.getTarget()); })
      .map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Alias));
        return E;
      });
}

Expect<void> Loader::loadAliasTarget(AliasTarget &AliasTarget) {
  // aliastarget ::= 0x00 i:<instanceidx> n:<string>          => export i n
  //               | 0x01 i:<core:instanceidx> n:<core:name>  => core export i n
  //               | 0x02 ct:<u32> idx:<u32>                  => outer ct idx
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
    return E;
  };
  EXPECTED_TRY(auto B, FMgr.readByte().map_error(ReportError));
  switch (B) {
  case 0x00:
  case 0x01: {
    AliasTargetExport &Ex = AliasTarget.emplace<AliasTargetExport>();
    EXPECTED_TRY(Ex.getInstanceIdx(), FMgr.readU32().map_error(ReportError));
    EXPECTED_TRY(Ex.getName(), FMgr.readName().map_error(ReportError));
    break;
  }
  case 0x02: {
    AliasTargetOuter &Out = AliasTarget.emplace<AliasTargetOuter>();
    EXPECTED_TRY(Out.getComponent(), FMgr.readU32().map_error(ReportError));
    EXPECTED_TRY(Out.getIndex(), FMgr.readU32().map_error(ReportError));
    break;
  }
  default:
    return logLoadError(ErrCode::Value::MalformedAliasTarget,
                        FMgr.getLastOffset(), ASTNodeAttr::AliasTarget);
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
