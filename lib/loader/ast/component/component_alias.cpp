// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadAlias(Alias &Alias) {
  // alias ::= s:<sort> t:<aliastarget>     => (alias t (s))
  if (auto Res = loadSort(Alias.getSort()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Alias));
    return Unexpect(Res);
  }
  if (auto Res = loadAliasTarget(Alias.getTarget()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Alias));
    return Unexpect(Res);
  }
  return {};
}

Expect<void> Loader::loadAliasTarget(AliasTarget &AliasTarget) {
  // aliastarget ::= 0x00 i:<instanceidx> n:<string>          => export i n
  //               | 0x01 i:<core:instanceidx> n:<core:name>  => core export i n
  //               | 0x02 ct:<u32> idx:<u32>                  => outer ct idx
  auto Res = FMgr.readByte();
  if (!Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
    return Unexpect(Res);
  }
  switch (*Res) {
  case 0x00:
  case 0x01: {
    AliasTargetExport &Ex = AliasTarget.emplace<AliasTargetExport>();
    if (auto V = FMgr.readU32(); !V) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(V);
    } else {
      Ex.getInstanceIdx() = *V;
    }
    if (auto V = FMgr.readName(); !V) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(V);
    } else {
      Ex.getName() = *V;
    }
    break;
  }
  case 0x02: {
    AliasTargetOuter &Out = AliasTarget.emplace<AliasTargetOuter>();
    if (auto V = FMgr.readU32(); !V) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(V);
    } else {
      Out.getComponent() = *V;
    }
    if (auto V = FMgr.readU32(); !V) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(V);
    } else {
      Out.getIndex() = *V;
    }
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
