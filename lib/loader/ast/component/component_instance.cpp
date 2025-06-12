// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadCoreInstance(AST::Component::CoreInstance &Instance) {
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

  auto LoadInstArg =
      [this](AST::Component::InstantiateArg<uint32_t> &Arg) -> Expect<void> {
    auto ReportArgError = [this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_CoreInstanceArg);
    };
    // core:instantiatearg ::= n:<core:name> 0x12 i:<instanceidx>
    //                       => (with n (instance i))
    EXPECTED_TRY(Arg.getName(), FMgr.readName().map_error(ReportArgError));
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportArgError));
    if (B != 0x12U) {
      return ReportArgError(ErrCode::Value::MalformedCoreInstance);
    }
    EXPECTED_TRY(Arg.getIndex(), FMgr.readU32().map_error(ReportArgError));
    return {};
  };

  auto LoadInlineExp =
      [this](AST::Component::InlineExport &Exp) -> Expect<void> {
    // core:inlineexport ::= n:<core:name> si:<core:sortidx> => (export n si)
    EXPECTED_TRY(Exp.getName(), FMgr.readName().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_CoreInlineExport);
    }));
    return loadSortIndex(Exp.getSortIdx(), true).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInlineExport));
      return E;
    });
  };

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<AST::Component::InstantiateArg<uint32_t>> Args;
    EXPECTED_TRY(loadVec<AST::Component::CoreInstance>(Args, LoadInstArg));
    Instance.setInstantiateArgs(Idx, std::move(Args));
    return {};
  }
  case 0x01: {
    std::vector<AST::Component::InlineExport> Exports;
    EXPECTED_TRY(loadVec<AST::Component::CoreInstance>(Exports, LoadInlineExp));
    Instance.setInlineExports(std::move(Exports));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedCoreInstance,
                        FMgr.getLastOffset(), ASTNodeAttr::Comp_CoreInstance);
  }
}

Expect<void> Loader::loadInstance(AST::Component::Instance &Instance) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Instance);
  };
  // instance     ::= ie:<instanceexpr>
  //                => (instance ie)
  // instanceexpr ::= 0x00 c:<componentidx> arg*:vec(<instantiatearg>)
  //                => (instantiate c arg*)
  //                | 0x01 e*:vec(<inlineexport>)
  //                => e*

  auto LoadInstArg =
      [this](AST::Component::InstantiateArg<AST::Component::SortIndex> &Arg)
      -> Expect<void> {
    // instantiatearg ::= n:<string> si:<sortidx> => (with n si)
    EXPECTED_TRY(Arg.getName(), FMgr.readName().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_InstanceArg);
    }));
    return loadSortIndex(Arg.getIndex()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_InstanceArg));
      return E;
    });
  };

  auto LoadInlineExp =
      [this](AST::Component::InlineExport &Exp) -> Expect<void> {
    // inlineexport ::= n:<exportname> si:<sortidx> => (export n si)
    EXPECTED_TRY(Exp.getName(), FMgr.readName().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(),
                          ASTNodeAttr::Comp_InlineExport);
    }));
    return loadSortIndex(Exp.getSortIdx()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_InlineExport));
      return E;
    });
  };

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    std::vector<AST::Component::InstantiateArg<AST::Component::SortIndex>> Args;
    EXPECTED_TRY(loadVec<AST::Component::Instance>(Args, LoadInstArg));
    Instance.setInstantiateArgs(Idx, std::move(Args));
    return {};
  }
  case 0x01: {
    std::vector<AST::Component::InlineExport> Exports;
    EXPECTED_TRY(loadVec<AST::Component::Instance>(Exports, LoadInlineExp));
    Instance.setInlineExports(std::move(Exports));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedInstance, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Instance);
  }
}

} // namespace Loader
} // namespace WasmEdge
