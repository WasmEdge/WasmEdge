// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC
#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadInstantiateArg(AST::InstantiateArg &Arg) {
  // syntax `(with n (instance i))`
  //
  // core:instantiatearg ::= n:<core:name> 0x12 i:<instanceidx>
  auto RName = FMgr.readName();
  if (!RName) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CoreInstance));
    return Unexpect(RName);
  }
  auto RMid = FMgr.readU32();
  if (!RMid) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CoreInstance));
    return Unexpect(RMid);
  } else if (*RMid != 0x12) {
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(), ASTNodeAttr::CoreInstance);
  }
  auto RIdx = FMgr.readU32();
  if (!RIdx) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CoreInstance));
    return Unexpect(RIdx);
  }
  Arg = AST::InstantiateArg(*RName, *RIdx);
  return {};
}

Expect<void> Loader::loadCoreInstance(AST::CoreInstanceExpr &InstanceExpr) {
  if (auto Tag = FMgr.readU32()) {
    switch (*Tag) {
    case 0x00: {
      uint32_t Idx = 0;
      if (auto Res = FMgr.readU32()) {
        Idx = *Res;
      } else {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CoreInstance));
        return Unexpect(Res);
      }
      std::vector<AST::InstantiateArg> Args{};
      if (auto Res = loadVec(Args,
                             [this](AST::InstantiateArg &Arg) -> Expect<void> {
                               return loadInstantiateArg(Arg);
                             });
          !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CoreInstance));
        return Unexpect(Tag);
      }

      InstanceExpr = AST::CoreInstanceExpr::Instantiate(Idx, Args);
      break;
    }
    case 0x01:
      break;
    default:
      return logLoadError(ErrCode::Value::MalformedCoreInstance,
                          FMgr.getLastOffset(), ASTNodeAttr::CoreInstance);
    }

    return {};
  } else {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CoreInstance));
    return Unexpect(Tag);
  }
}

} // namespace Loader
} // namespace WasmEdge
