// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

// Load binary of Import description. See "include/loader/loader.h".
Expect<void> Loader::loadDesc(AST::ImportDesc &ImpDesc) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Desc_Import);
  };

  // Read the module name.
  EXPECTED_TRY(FMgr.readName().map_error(ReportError).map([&](std::string S) {
    ImpDesc.setModuleName(S);
  }));

  // Read the external name.
  EXPECTED_TRY(FMgr.readName().map_error(ReportError).map([&](std::string S) {
    ImpDesc.setExternalName(S);
  }));

  // Read the external type.
  EXPECTED_TRY(FMgr.readByte().map_error(ReportError).map([&](uint8_t B) {
    ImpDesc.setExternalType(static_cast<ExternalType>(B));
  }));

  // Make content node according to external type.
  switch (ImpDesc.getExternalType()) {
  case ExternalType::Function: {
    // Read the function type index.
    EXPECTED_TRY(FMgr.readU32().map_error(ReportError).map([&](uint32_t Idx) {
      ImpDesc.setExternalFuncTypeIdx(Idx);
    }));
    return {};
  }
  case ExternalType::Table: {
    // Read the table type node.
    return loadType(ImpDesc.getExternalTableType());
  }
  case ExternalType::Memory: {
    // Read the memory type node.
    return loadType(ImpDesc.getExternalMemoryType());
  }
  case ExternalType::Global: {
    // Read the global type node.
    EXPECTED_TRY(loadType(ImpDesc.getExternalGlobalType()));
    // Import the mutable globals are for ImportExportMutGlobals proposal.
    if (ImpDesc.getExternalGlobalType().getValMut() == ValMut::Var &&
        unlikely(!Conf.hasProposal(Proposal::ImportExportMutGlobals))) {
      return logNeedProposal(ErrCode::Value::InvalidMut,
                             Proposal::ImportExportMutGlobals,
                             FMgr.getLastOffset(), ASTNodeAttr::Desc_Import);
    }
    return {};
  }
  case ExternalType::Tag: {
    if (!Conf.hasProposal(Proposal::ExceptionHandling)) {
      return logNeedProposal(ErrCode::Value::MalformedImportKind,
                             Proposal::ExceptionHandling, FMgr.getLastOffset(),
                             ASTNodeAttr::Module);
    }
    // Read the Tag type node.
    return loadType(ImpDesc.getExternalTagType());
  }
  default:
    return logLoadError(ErrCode::Value::MalformedImportKind,
                        FMgr.getLastOffset(), ASTNodeAttr::Desc_Import);
  }
}

// Load binary of Export description. See "include/loader/loader.h".
Expect<void> Loader::loadDesc(AST::ExportDesc &ExpDesc) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Desc_Export);
  };

  // Read external name to export.
  EXPECTED_TRY(FMgr.readName().map_error(ReportError).map([&](std::string S) {
    ExpDesc.setExternalName(S);
  }));

  // Read external type.
  EXPECTED_TRY(FMgr.readByte().map_error(ReportError).map([&](uint8_t B) {
    ExpDesc.setExternalType(static_cast<ExternalType>(B));
  }));
  switch (ExpDesc.getExternalType()) {
  case ExternalType::Function:
  case ExternalType::Table:
  case ExternalType::Memory:
  case ExternalType::Global:
    break;
  case ExternalType::Tag:
    if (!Conf.hasProposal(Proposal::ExceptionHandling)) {
      return logNeedProposal(ErrCode::Value::MalformedImportKind,
                             Proposal::ExceptionHandling, FMgr.getLastOffset(),
                             ASTNodeAttr::Module);
    }
    break;
  default:
    return logLoadError(ErrCode::Value::MalformedExportKind,
                        FMgr.getLastOffset(), ASTNodeAttr::Desc_Export);
  }

  // Read external index to export.
  EXPECTED_TRY(FMgr.readU32().map_error(ReportError).map([&](uint32_t Idx) {
    ExpDesc.setExternalIndex(Idx);
  }));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
