// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

#include <cstdint>

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadInstantiateArg(InstantiateArg<SortIndex<Sort>> &Arg) {
  // syntax `(with n si)`
  //
  // instantiatearg ::= n:<string>  si:<sortidx>
  if (auto Res = FMgr.readName()) {
    Arg.getName() = *Res;
  } else {
    return Unexpect(Res);
  }
  return loadSortIndex(Arg.getIndex());
}

Expect<void> Loader::loadInlineExport(InlineExport<Sort> &Exp) {
  if (auto Res = FMgr.readName()) {
    Exp.getName() = *Res;
  } else {
    return Unexpect(Res);
  }
  return loadSortIndex(Exp.getSortIdx());
}

Expect<void> Loader::loadInstantiateArg(CoreInstantiateArg &Arg) {
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

Expect<void> Loader::loadInlineExport(InlineExport<CoreSort> &Exp) {
  if (auto Res = FMgr.readName()) {
    Exp.getName() = *Res;
  } else {
    return Unexpect(Res);
  }
  return loadCoreSortIndex(Exp.getSortIdx());
}

Expect<void> Loader::loadInstance(InstanceExpr &InstanceExpr) {
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
      std::vector<InstantiateArg<SortIndex<Sort>>> Args{};
      if (auto Res = loadVec<InstanceSection>(
              Args,
              [this](InstantiateArg<SortIndex<Sort>> &Arg) -> Expect<void> {
                return loadInstantiateArg(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<Instantiate>(Instantiate(Idx, Args));
      break;
    }
    case 0x01: {
      std::vector<InlineExport<Sort>> Exports{};
      if (auto Res = loadVec<InstanceSection>(
              Exports,
              [this](InlineExport<Sort> &Arg) -> Expect<void> {
                return loadInlineExport(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<CompInlineExports>(InlineExports(Exports));
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

Expect<void> Loader::loadCoreInstance(CoreInstanceExpr &InstanceExpr) {
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
      std::vector<CoreInstantiateArg> Args{};
      if (auto Res = loadVec<CoreInstanceSection>(
              Args,
              [this](CoreInstantiateArg &Arg) -> Expect<void> {
                return loadInstantiateArg(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<CoreInstantiate>(CoreInstantiate(Idx, Args));

      break;
    }
    case 0x01: {
      std::vector<InlineExport<CoreSort>> Exports{};
      if (auto Res = loadVec<CoreInstanceSection>(
              Exports,
              [this](InlineExport<CoreSort> &Arg) -> Expect<void> {
                return loadInlineExport(Arg);
              });
          !Res) {
        return Unexpect(Res);
      }

      InstanceExpr.emplace<CoreInlineExports>(CoreInlineExports(Exports));

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
