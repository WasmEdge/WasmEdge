// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC
#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadInstantiateArg(AST::InstantiateArg &Arg) {
  // syntax `(with n si)`
  //
  // instantiatearg ::= n:<string>  si:<sortidx>
  if (auto Res = FMgr.readName(); !Res) {
    return Unexpect(Res);
  } else {
    Arg.getName() = *Res;
  }
  return loadSortIndex(Arg.getSortIndex());
}

Expect<void> Loader::loadInlineExport(AST::InlineExport &Exp) {
  if (auto Res = FMgr.readName(); !Res) {
    return Unexpect(Res);
  } else {
    Exp.getName() = *Res;
  }
  return loadSortIndex(Exp.getSortIdx());
}

Expect<void> Loader::loadInstantiateArg(AST::CoreInstantiateArg &Arg) {
  // syntax `(with n (instance i))`
  //
  // core:instantiatearg ::= n:<core:name> 0x12 i:<instanceidx>
  if (auto Res = FMgr.readName(); !Res) {
    return Unexpect(Res);
  } else {
    Arg.getName() = *Res;
  }
  if (auto Res = FMgr.readU32(); !Res) {
    return Unexpect(Res);
  } else if (*Res != 0x12) {
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(), ASTNodeAttr::CoreInstance);
  }
  if (auto Res = FMgr.readU32(); !Res) {
    return Unexpect(Res);
  } else {
    Arg.getInstanceIdx() = *Res;
  }

  return {};
}

Expect<void> Loader::loadInlineExport(AST::CoreInlineExport &Exp) {
  if (auto Res = FMgr.readName(); !Res) {
    return Unexpect(Res);
  } else {
    Exp.getName() = *Res;
  }
  return loadCoreSortIndex(Exp.getSortIdx());
}

Expect<void> Loader::loadInstance(AST::InstanceExpr &InstanceExpr) {
  if (auto Tag = FMgr.readU32()) {
    switch (*Tag) {
    case 0x00: {
      uint32_t Idx = 0;
      if (auto Res = FMgr.readU32()) {
        Idx = *Res;
      } else {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Instance));
        return Unexpect(Res);
      }
      std::vector<AST::InstantiateArg> Args{};
      if (auto Res = loadVec<AST::InstanceSection>(
              Args,
              [this](AST::InstantiateArg &Arg) -> Expect<void> {
                return loadInstantiateArg(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<AST::Instantiate>(AST::Instantiate(Idx, Args));
      break;
    }
    case 0x01: {
      std::vector<AST::InlineExport> Exports{};
      if (auto Res = loadVec<AST::InstanceSection>(
              Exports,
              [this](AST::InlineExport &Arg) -> Expect<void> {
                return loadInlineExport(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<AST::InlineExports>(AST::InlineExports(Exports));
      break;
    }
    default:
      return logLoadError(ErrCode::Value::MalformedCoreInstance,
                          FMgr.getLastOffset(), ASTNodeAttr::Instance);
    }

    return {};
  } else {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Instance));
    return Unexpect(Tag);
  }
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
      std::vector<AST::CoreInstantiateArg> Args{};
      if (auto Res = loadVec<AST::CoreInstanceSection>(
              Args,
              [this](AST::CoreInstantiateArg &Arg) -> Expect<void> {
                return loadInstantiateArg(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<AST::CoreInstantiate>(
          AST::CoreInstantiate(Idx, Args));

      break;
    }
    case 0x01: {
      std::vector<AST::CoreInlineExport> Exports{};
      if (auto Res = loadVec<AST::CoreInstanceSection>(
              Exports,
              [this](AST::CoreInlineExport &Arg) -> Expect<void> {
                return loadInlineExport(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<AST::CoreInlineExports>(
          AST::CoreInlineExports(Exports));

      break;
    }
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
