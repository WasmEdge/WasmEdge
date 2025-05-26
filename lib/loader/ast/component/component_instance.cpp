// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadInstantiateArg(InstantiateArg<SortIndex<Sort>> &Arg) {
  // syntax `(with n si)`
  //
  // instantiatearg ::= n:<string>  si:<sortidx>
  EXPECTED_TRY(Arg.getName(), FMgr.readName());
  return loadSortIndex(Arg.getIndex());
}

Expect<void> Loader::loadInlineExport(InlineExport<Sort> &Exp) {
  EXPECTED_TRY(Exp.getName(), FMgr.readName());
  return loadSortIndex(Exp.getSortIdx());
}

Expect<void> Loader::loadInstantiateArg(CoreInstantiateArg &Arg) {
  // syntax `(with n (instance i))`
  //
  // core:instantiatearg ::= n:<core:name> 0x12 i:<instanceidx>
  EXPECTED_TRY(Arg.getName(), FMgr.readName());
  EXPECTED_TRY(auto B, FMgr.readByte().map_error([this](auto) {
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(), ASTNodeAttr::CoreInstance)
        .value();
  }));
  if (B != 0x12U) {
    return logLoadError(ErrCode::Value::IllegalGrammar, FMgr.getLastOffset(),
                        ASTNodeAttr::CoreInstance);
  }
  EXPECTED_TRY(Arg.getIndex(), FMgr.readU32());

  return {};
}

Expect<void> Loader::loadInlineExport(InlineExport<CoreSort> &Exp) {
  EXPECTED_TRY(Exp.getName(), FMgr.readName());
  return loadCoreSortIndex(Exp.getSortIdx());
}

Expect<void> Loader::loadInstance(InstanceExpr &InstanceExpr) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Instance));
    return E;
  };
  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error(ReportError));
  switch (Tag) {
  case 0x00: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<InstantiateArg<SortIndex<Sort>>> Args{};
    EXPECTED_TRY(loadVec<InstanceSection>(
        Args, [this](InstantiateArg<SortIndex<Sort>> &Arg) -> Expect<void> {
          return loadInstantiateArg(Arg);
        }));

    InstanceExpr.emplace<Instantiate>(Instantiate(Idx, std::move(Args)));
    break;
  }
  case 0x01: {
    std::vector<InlineExport<Sort>> Exports{};
    EXPECTED_TRY(
        loadVec<InstanceSection>(Exports, [this](InlineExport<Sort> &Arg) {
          return loadInlineExport(Arg);
        }));

    InstanceExpr.emplace<CompInlineExports>(InlineExports(Exports));
    break;
  }
  default:
    return logLoadError(ErrCode::Value::MalformedInstance, FMgr.getLastOffset(),
                        ASTNodeAttr::Instance);
  }

  return {};
}

Expect<void> Loader::loadCoreInstance(CoreInstanceExpr &InstanceExpr) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::CoreInstance));
    return E;
  };
  EXPECTED_TRY(auto Tag, FMgr.readByte().map_error(ReportError));
  switch (Tag) {
  case 0x00: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<CoreInstantiateArg> Args{};
    EXPECTED_TRY(
        loadVec<CoreInstanceSection>(Args, [this](CoreInstantiateArg &Arg) {
          return loadInstantiateArg(Arg);
        }));

    InstanceExpr.emplace<CoreInstantiate>(CoreInstantiate(Idx, Args));

    break;
  }
  case 0x01: {
    std::vector<InlineExport<CoreSort>> Exports{};
    EXPECTED_TRY(loadVec<CoreInstanceSection>(
        Exports,
        [this](InlineExport<CoreSort> &Arg) { return loadInlineExport(Arg); }));

    InstanceExpr.emplace<CoreInlineExports>(CoreInlineExports(Exports));

    break;
  }
  default:
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(), ASTNodeAttr::CoreInstance);
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
