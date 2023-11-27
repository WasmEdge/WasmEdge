// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC
#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadAlias(AST::Alias &Alias) {
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

Expect<void> Loader::loadAliasTarget(AST::AliasTarget &AliasTarget) {
  // aliastarget ::= 0x00 i:<instanceidx> n:<string>          => export i n
  //               | 0x01 i:<core:instanceidx> n:<core:name>  => core export i n
  //               | 0x02 ct:<u32> idx:<u32>                  => outer ct idx
  auto Res = FMgr.readU32();
  if (!Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
    return Unexpect(Res);
  }
  switch (*Res) {
  case 0x00:
  case 0x01: {
    uint32_t InstanceIndex;
    std::string_view Name;
    if (auto V = FMgr.readU32(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(Res);
    } else {
      InstanceIndex = *V;
    }
    if (auto V = FMgr.readName(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(Res);
    } else {
      Name = *V;
    }
    AliasTarget = AST::AliasTarget::Export(InstanceIndex, Name);
    break;
  }
  case 0x02: {
    uint32_t ComponentIndex;
    uint32_t Index;
    if (auto V = FMgr.readU32(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(Res);
    } else {
      ComponentIndex = *V;
    }
    if (auto V = FMgr.readU32(); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::AliasTarget));
      return Unexpect(Res);
    } else {
      Index = *V;
    }
    AliasTarget = AST::AliasTarget::Outer(ComponentIndex, Index);
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
