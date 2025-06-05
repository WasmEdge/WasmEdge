// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void>
Loader::loadCoreInstance(AST::Component::CoreInstanceExpr &InstanceExpr) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreInstance);
  };
  // core:instance       ::= ie:<core:instanceexpr>
  //                       => (instance ie)
  // core:instanceexpr   ::= 0x00 m:<moduleidx> arg*:vec(<core:instantiatearg>)
  //                       => (instantiate m arg*)
  //                       | 0x01 e*:vec(<core:inlineexport>)
  //                       => e*

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<AST::Component::CoreInstantiateArg> Args;
    EXPECTED_TRY(loadVec<AST::Component::CoreInstanceExpr>(
        Args, [this](AST::Component::CoreInstantiateArg &Arg) {
          return loadCoreInstantiateArg(Arg);
        }));
    InstanceExpr.emplace<AST::Component::CoreInstantiate>(Idx, std::move(Args));
    return {};
  }
  case 0x01: {
    std::vector<AST::Component::InlineExportImpl<AST::Component::CoreSort>>
        Exports;
    EXPECTED_TRY(loadVec<AST::Component::CoreInstanceExpr>(
        Exports,
        [this](
            AST::Component::InlineExportImpl<AST::Component::CoreSort> &Arg) {
          return loadCoreInlineExport(Arg);
        }));
    InstanceExpr.emplace<AST::Component::CoreInlineExports>(std::move(Exports));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_CoreInstance);
  }
}

Expect<void>
Loader::loadCoreInstantiateArg(AST::Component::CoreInstantiateArg &Arg) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreInstanceArg);
  };
  // core:instantiatearg ::= n:<core:name> 0x12 i:<instanceidx>
  //                       => (with n (instance i))

  EXPECTED_TRY(Arg.getName(), FMgr.readName().map_error(ReportError));
  EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
  if (B != 0x12U) {
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreInstanceArg);
  }
  EXPECTED_TRY(Arg.getIndex(), FMgr.readU32().map_error(ReportError));
  return {};
}

Expect<void> Loader::loadCoreInlineExport(
    AST::Component::InlineExportImpl<AST::Component::CoreSort> &Exp) {
  // core:inlineexport ::= n:<core:name> si:<core:sortidx> => (export n si)
  EXPECTED_TRY(Exp.getName(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_CoreInlineExport);
  }));
  return loadCoreSortIndex(Exp.getSortIdx()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInlineExport));
    return E;
  });
}

Expect<void> Loader::loadInstance(AST::Component::InstanceExpr &InstanceExpr) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Instance);
  };
  // instance     ::= ie:<instanceexpr>
  //                => (instance ie)
  // instanceexpr ::= 0x00 c:<componentidx> arg*:vec(<instantiatearg>)
  //                => (instantiate c arg*)
  //                | 0x01 e*:vec(<inlineexport>)
  //                => e*

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<AST::Component::InstantiateArg> Args;
    EXPECTED_TRY(loadVec<AST::Component::InstanceExpr>(
        Args, [this](AST::Component::InstantiateArg &Arg) -> Expect<void> {
          return loadInstantiateArg(Arg);
        }));
    InstanceExpr.emplace<AST::Component::Instantiate>(
        AST::Component::Instantiate(Idx, std::move(Args)));
    return {};
  }
  case 0x01: {
    std::vector<AST::Component::InlineExportImpl<AST::Component::Sort>> Exports;
    EXPECTED_TRY(loadVec<AST::Component::InstanceExpr>(
        Exports,
        [this](AST::Component::InlineExportImpl<AST::Component::Sort> &Arg) {
          return loadInlineExport(Arg);
        }));
    InstanceExpr.emplace<AST::Component::InlineExports>(std::move(Exports));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedInstance, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Instance);
  }
}

Expect<void> Loader::loadInstantiateArg(AST::Component::InstantiateArg &Arg) {
  // instantiatearg ::= n:<string> si:<sortidx> => (with n si)
  EXPECTED_TRY(Arg.getName(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_InlineExport);
  }));
  return loadSortIndex(Arg.getIndex()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_InlineExport));
    return E;
  });
}

Expect<void> Loader::loadInlineExport(
    AST::Component::InlineExportImpl<AST::Component::Sort> &Exp) {
  // inlineexport ::= n:<exportname> si:<sortidx> => (export n si)
  EXPECTED_TRY(Exp.getName(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_InlineExport);
  }));
  return loadSortIndex(Exp.getSortIdx()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_InlineExport));
    return E;
  });
}

} // namespace Loader
} // namespace WasmEdge
