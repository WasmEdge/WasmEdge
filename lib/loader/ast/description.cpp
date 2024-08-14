// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

// Load binary of Import description. See "include/loader/loader.h".
Expect<void> Loader::loadDesc(AST::ImportDesc &ImpDesc) {
  // Read the module name.
  if (auto Res = FMgr.readName()) {
    ImpDesc.setModuleName(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Desc_Import);
  }

  // Read the external name.
  if (auto Res = FMgr.readName()) {
    ImpDesc.setExternalName(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Desc_Import);
  }

  // Read the external type.
  if (auto Res = FMgr.readByte()) {
    ImpDesc.setExternalType(static_cast<ExternalType>(*Res));
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Desc_Import);
  }

  // Make content node according to external type.
  switch (ImpDesc.getExternalType()) {
  case ExternalType::Function: {
    // Read the function type index.
    if (auto Res = FMgr.readU32()) {
      ImpDesc.setExternalFuncTypeIdx(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Desc_Import);
    }
    break;
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
    if (auto Res = loadType(ImpDesc.getExternalGlobalType()); !Res) {
      return Unexpect(Res.error());
    }
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
  return {};
}

// Load binary of Export description. See "include/loader/loader.h".
Expect<void> Loader::loadDesc(AST::ExportDesc &ExpDesc) {
  // Read external name to export.
  if (auto Res = FMgr.readName()) {
    ExpDesc.setExternalName(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Desc_Export);
  }

  // Read external type.
  if (auto Res = FMgr.readByte()) {
    ExpDesc.setExternalType(static_cast<ExternalType>(*Res));
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Desc_Export);
  }
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
  if (auto Res = FMgr.readU32()) {
    ExpDesc.setExternalIndex(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Desc_Export);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
