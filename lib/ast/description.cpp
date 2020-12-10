// SPDX-License-Identifier: Apache-2.0
#include "ast/description.h"
#include "common/log.h"

namespace SSVM {
namespace AST {

/// Load binary of Import description. See "include/ast/description.h".
Expect<void> ImportDesc::loadBinary(FileMgr &Mgr) {
  /// Read the module name.
  if (auto Res = Mgr.readName()) {
    ModName = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the external name.
  if (auto Res = Mgr.readName()) {
    ExtName = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the external type.
  if (auto Res = Mgr.readByte()) {
    ExtType = static_cast<ExternalType>(*Res);
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Make content node according to external type.
  switch (ExtType) {
  case ExternalType::Function: {
    /// Read the function type index.
    if (auto Res = Mgr.readU32()) {
      FuncTypeIdx = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    break;
  }
  case ExternalType::Table: {
    /// Read the table type node.
    return TabType.loadBinary(Mgr);
  }
  case ExternalType::Memory: {
    /// Read the memory type node.
    return MemType.loadBinary(Mgr);
  }
  case ExternalType::Global: {
    /// Read the global type node.
    return GlobType.loadBinary(Mgr);
  }
  default:
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(ErrCode::InvalidGrammar);
  }
  return {};
}

/// Load binary of Export description. See "include/ast/description.h".
Expect<void> ExportDesc::loadBinary(FileMgr &Mgr) {
  /// Read external name to export.
  if (auto Res = Mgr.readName()) {
    ExtName = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read external type.
  if (auto Res = Mgr.readByte()) {
    ExtType = static_cast<ExternalType>(*Res);
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  switch (ExtType) {
  case ExternalType::Function:
  case ExternalType::Table:
  case ExternalType::Memory:
  case ExternalType::Global:
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(ErrCode::InvalidGrammar);
  }

  /// Read external index to export.
  if (auto Res = Mgr.readU32()) {
    ExtIdx = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  return {};
}

} // namespace AST
} // namespace SSVM
