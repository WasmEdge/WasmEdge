// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC
#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadInstantiateArg(
    AST::InstantiateArg<AST::SortIndex<AST::Sort>> &Arg) {
  // syntax `(with n si)`
  //
  // instantiatearg ::= n:<string>  si:<sortidx>
  if (auto Res = FMgr.readName(); !Res) {
    return Unexpect(Res);
  } else {
    Arg.getName() = *Res;
  }
  return loadSortIndex(Arg.getIndex());
}

Expect<void> Loader::loadInlineExport(AST::InlineExport<AST::Sort> &Exp) {
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
  if (auto Res = FMgr.readName()) {
    Arg.getName() = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = FMgr.readByte(); !Res) {
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(), ASTNodeAttr::CoreInstance);
  } else if (*Res != 0x12U) {
    return logLoadError(ErrCode::Value::IntegerTooLong, FMgr.getLastOffset(),
                        ASTNodeAttr::CoreInstance);
  }
  if (auto Res = FMgr.readU32()) {
    Arg.getIndex() = *Res;
  } else {
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Loader::loadInlineExport(AST::InlineExport<AST::CoreSort> &Exp) {
  if (auto Res = FMgr.readName()) {
    Exp.getName() = *Res;
  } else {
    return Unexpect(Res);
  }
  return loadCoreSortIndex(Exp.getSortIdx());
}

Expect<void> Loader::loadInstance(AST::InstanceExpr &InstanceExpr) {
  if (auto Tag = FMgr.readByte()) {
    switch (*Tag) {
    case 0x00: {
      uint32_t Idx = 0;
      if (auto Res = FMgr.readU32()) {
        Idx = *Res;
      } else {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Instance));
        return Unexpect(Res);
      }
      std::vector<AST::InstantiateArg<AST::SortIndex<AST::Sort>>> Args{};
      if (auto Res = loadVec<AST::InstanceSection>(
              Args,
              [this](AST::InstantiateArg<AST::SortIndex<AST::Sort>> &Arg)
                  -> Expect<void> { return loadInstantiateArg(Arg); });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<AST::Instantiate>(AST::Instantiate(Idx, Args));
      break;
    }
    case 0x01: {
      std::vector<AST::InlineExport<AST::Sort>> Exports{};
      if (auto Res = loadVec<AST::InstanceSection>(
              Exports,
              [this](AST::InlineExport<AST::Sort> &Arg) -> Expect<void> {
                return loadInlineExport(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<AST::CompInlineExports>(AST::InlineExports(Exports));
      break;
    }
    default:
      return logLoadError(ErrCode::Value::MalformedInstance,
                          FMgr.getLastOffset(), ASTNodeAttr::Instance);
    }

    return {};
  } else {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Instance));
    return Unexpect(Tag);
  }
}

Expect<void> Loader::loadCoreInstance(AST::CoreInstanceExpr &InstanceExpr) {
  if (auto Tag = FMgr.readByte()) {
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
      std::vector<AST::InlineExport<AST::CoreSort>> Exports{};
      if (auto Res = loadVec<AST::CoreInstanceSection>(
              Exports,
              [this](AST::InlineExport<AST::CoreSort> &Arg) -> Expect<void> {
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
