// SPDX-License-Identifier: Apache-2.0
#include "ast/description.h"
#include "common/log.h"

namespace WasmEdge {
namespace AST {

/// Load binary of Import description. See "include/ast/description.h".
Expect<void> ImportDesc::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read the module name.
  if (auto Res = Mgr.readName()) {
    ModName = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }

  /// Read the external name.
  if (auto Res = Mgr.readName()) {
    ExtName = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }

  /// Read the external type.
  if (auto Res = Mgr.readByte()) {
    ExtType = static_cast<ExternalType>(*Res);
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }

  /// Make content node according to external type.
  switch (ExtType) {
  case ExternalType::Function: {
    /// Read the function type index.
    if (auto Res = Mgr.readU32()) {
      FuncTypeIdx = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    break;
  }
  case ExternalType::Table: {
    /// Read the table type node.
    return TabType.loadBinary(Mgr, Conf);
  }
  case ExternalType::Memory: {
    /// Read the memory type node.
    return MemType.loadBinary(Mgr, Conf);
  }
  case ExternalType::Global: {
    /// Read the global type node.
    return GlobType.loadBinary(Mgr, Conf);
  }
  default:
    return logLoadError(ErrCode::InvalidImportKind, Mgr.getOffset() - 1,
                        NodeAttr);
  }
  return {};
}

/// Load binary of Export description. See "include/ast/description.h".
Expect<void> ExportDesc::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read external name to export.
  if (auto Res = Mgr.readName()) {
    ExtName = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }

  /// Read external type.
  if (auto Res = Mgr.readByte()) {
    ExtType = static_cast<ExternalType>(*Res);
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  switch (ExtType) {
  case ExternalType::Function:
  case ExternalType::Table:
  case ExternalType::Memory:
  case ExternalType::Global:
    break;
  default:
    return logLoadError(ErrCode::InvalidExportKind, Mgr.getOffset() - 1,
                        NodeAttr);
  }

  /// Read external index to export.
  if (auto Res = Mgr.readU32()) {
    ExtIdx = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  return {};
}

} // namespace AST
} // namespace WasmEdge
