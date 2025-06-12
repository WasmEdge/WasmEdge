// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadDecl(AST::Component::CoreImportDecl &Decl) {
  // core:importdecl ::= m:<core:name> n:<core:name> d:<core:importdesc>
  //                   => (import m n d)

  EXPECTED_TRY(Decl.getModuleName(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_CoreImport);
  }));
  EXPECTED_TRY(Decl.getName(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_CoreImport);
  }));
  return loadDesc(Decl.getImportDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_CoreImport));
    return E;
  });
}

Expect<void> Loader::loadDecl(AST::Component::CoreExportDecl &Decl) {
  // core:exportdecl ::= n:<core:name> d:<core:importdesc> => (export n d)

  EXPECTED_TRY(Decl.getName(), FMgr.readName().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_CoreExport);
  }));
  return loadDesc(Decl.getImportDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_CoreExport));
    return E;
  });
}

Expect<void> Loader::loadDecl(AST::Component::CoreModuleDecl &Decl) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_CoreModule));
    return E;
  };
  // core:moduledecl ::= 0x00 i:<core:importdecl> => i
  //                   | 0x01 t:<core:type>       => t
  //                   | 0x02 a:<core:alias>      => a
  //                   | 0x03 e:<core:exportdecl> => e

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_CoreModule);
  }));
  switch (Flag) {
  case 0x00: {
    AST::Component::CoreImportDecl ImpDecl;
    EXPECTED_TRY(loadDecl(ImpDecl).map_error(ReportError));
    Decl.setImport(std::move(ImpDecl));
    return {};
  }
  case 0x01: {
    auto DefType = std::make_unique<AST::Component::CoreDefType>();
    EXPECTED_TRY(loadType(*DefType).map_error(ReportError));
    Decl.setType(std::move(DefType));
    return {};
  }
  case 0x02: {
    AST::Component::CoreAlias Alias;
    EXPECTED_TRY(loadCoreAlias(Alias).map_error(ReportError));
    Decl.setAlias(std::move(Alias));
    return {};
  }
  case 0x03: {
    AST::Component::CoreExportDecl ExpDecl;
    EXPECTED_TRY(loadDecl(ExpDecl).map_error(ReportError));
    Decl.setExport(std::move(ExpDecl));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::IllegalGrammar, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_CoreModule);
  }
}

Expect<void> Loader::loadDecl(AST::Component::ImportDecl &Decl) {
  // importdecl  ::= in:<importname'> ed:<externdesc> => (import in ed)
  // importname' ::= 0x00 len:<u32> in:<importname>   => in (if len = |in|)

  EXPECTED_TRY(loadExternName(Decl.getName()).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Decl_Import);
  }));
  return loadDesc(Decl.getExternDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Import));
    return E;
  });
}

Expect<void> Loader::loadDecl(AST::Component::ExportDecl &Decl) {
  // exportdecl  ::= en:<exportname'> ed:<externdesc> => (export en ed)
  // exportname' ::= 0x00 len:<u32> en:<exportname>   => en (if len = |en|)

  EXPECTED_TRY(loadExternName(Decl.getName()).map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Decl_Export);
  }));
  return loadDesc(Decl.getExternDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Export));
    return E;
  });
}

Expect<void> Loader::loadDecl(AST::Component::InstanceDecl &Decl) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Instance));
    return E;
  };
  // instancedecl ::= 0x00 t:<core:type>   => t
  //                | 0x01 t:<type>        => t
  //                | 0x02 a:<alias>       => a
  //                | 0x04 ed:<exportdecl> => ed

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_Instance);
  }));
  switch (Flag) {
  case 0x00: {
    auto DefType = std::make_unique<AST::Component::CoreDefType>();
    EXPECTED_TRY(loadType(*DefType).map_error(ReportError));
    Decl.setCoreType(std::move(DefType));
    return {};
  }
  case 0x01: {
    auto DefType = std::make_unique<AST::Component::DefType>();
    EXPECTED_TRY(loadType(*DefType).map_error(ReportError));
    Decl.setType(std::move(DefType));
    return {};
  }
  case 0x02: {
    AST::Component::Alias Alias;
    EXPECTED_TRY(loadAlias(Alias).map_error(ReportError));
    Decl.setAlias(std::move(Alias));
    return {};
  }
  case 0x04: {
    AST::Component::ExportDecl ExpDecl;
    EXPECTED_TRY(loadDecl(ExpDecl).map_error(ReportError));
    Decl.setExport(std::move(ExpDecl));
    return {};
  }
  default:
    return logLoadError(ErrCode::Value::MalformedDefType, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_Instance);
  }
}

Expect<void> Loader::loadDecl(AST::Component::ComponentDecl &Decl) {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Component));
    return E;
  };
  // componentdecl ::= 0x03 id:<importdecl> => id
  //                 | id:<instancedecl>    => id

  EXPECTED_TRY(auto B, FMgr.peekByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Decl_Component);
  }));
  if (B == 0x03U) {
    FMgr.readByte();
    AST::Component::ImportDecl ImpDecl;
    EXPECTED_TRY(loadDecl(ImpDecl).map_error(ReportError));
    Decl.setImport(std::move(ImpDecl));
  } else {
    AST::Component::InstanceDecl InstDecl;
    EXPECTED_TRY(loadDecl(InstDecl).map_error(ReportError));
    Decl.setInstance(std::move(InstDecl));
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
