// SPDX-License-Identifier: Apache-2.0
#include "common/ast/description.h"

namespace SSVM {
namespace AST {

/// Load binary of Import description. See "include/common/ast/description.h".
Expect<void> ImportDesc::loadBinary(FileMgr &Mgr) {
  /// Read the module name.
  if (auto Res = Mgr.readName()) {
    ModName = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Read the external name.
  if (auto Res = Mgr.readName()) {
    ExtName = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Read the external type.
  if (auto Res = Mgr.readByte()) {
    ExtType = static_cast<ExternalType>(*Res);
  } else {
    return Unexpect(Res);
  }

  /// Make content node according to external type.
  switch (ExtType) {
  case ExternalType::Function: {
    /// Read the function type index.
    if (auto Res = Mgr.readU32()) {
      ExtContent = std::make_unique<unsigned int>(*Res);
    } else {
      return Unexpect(Res);
    }
    break;
  }
  case ExternalType::Table: {
    /// Read and make table type node.
    ExtContent = std::make_unique<TableType>();
    return std::get<1>(ExtContent)->loadBinary(Mgr);
  }
  case ExternalType::Memory: {
    /// Read and make memory type node.
    ExtContent = std::make_unique<MemoryType>();
    return std::get<2>(ExtContent)->loadBinary(Mgr);
  }
  case ExternalType::Global: {
    /// Read and make global type node.
    ExtContent = std::make_unique<GlobalType>();
    return std::get<3>(ExtContent)->loadBinary(Mgr);
  }
  default:
    return Unexpect(ErrCode::InvalidGrammar);
  }
  return {};
}

/// Load binary of Export description. See "include/common/ast/description.h".
Expect<void> ExportDesc::loadBinary(FileMgr &Mgr) {
  /// Read external name to export.
  if (auto Res = Mgr.readName()) {
    ExtName = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Read external type.
  if (auto Res = Mgr.readByte()) {
    ExtType = static_cast<ExternalType>(*Res);
  } else {
    return Unexpect(Res);
  }
  switch (ExtType) {
  case ExternalType::Function:
  case ExternalType::Table:
  case ExternalType::Memory:
  case ExternalType::Global:
    break;
  default:
    return Unexpect(ErrCode::InvalidGrammar);
  }

  /// Read external index to export.
  if (auto Res = Mgr.readU32()) {
    ExtIdx = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

} // namespace AST
} // namespace SSVM
