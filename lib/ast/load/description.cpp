#include "ast/description.h"

namespace SSVM {
namespace AST {

/// Load binary of Import description. See "include/ast/description.h".
Loader::ErrCode ImportDesc::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the module name.
  if ((Status = Mgr.readName(ModName)) != Loader::ErrCode::Success)
    return Status;

  /// Read the external name.
  if ((Status = Mgr.readName(ExtName)) != Loader::ErrCode::Success)
    return Status;

  /// Read the external type.
  unsigned char Byte = 0;
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  ExtType = static_cast<ExternalType>(Byte);

  /// Make content node according to external type.
  switch (ExtType) {
  case ExternalType::Function: {
    /// Read the function type index.
    unsigned int TypeIdx = 0;
    Status = Mgr.readU32(TypeIdx);
    ExtContent = std::make_unique<unsigned int>(TypeIdx);
    break;
  }
  case ExternalType::Table: {
    /// Read and make table type node.
    auto NewTable = std::make_unique<TableType>();
    Status = NewTable->loadBinary(Mgr);
    ExtContent = std::move(NewTable);
    break;
  }
  case ExternalType::Memory: {
    /// Read and make memory type node.
    auto NewMemory = std::make_unique<MemoryType>();
    Status = NewMemory->loadBinary(Mgr);
    ExtContent = std::move(NewMemory);
    break;
  }
  case ExternalType::Global: {
    /// Read and make global type node.
    auto NewGlobal = std::make_unique<GlobalType>();
    Status = NewGlobal->loadBinary(Mgr);
    ExtContent = std::move(NewGlobal);
    break;
  }
  default:
    Status = Loader::ErrCode::InvalidGrammar;
    break;
  }
  return Status;
}

/// Load binary of Export description. See "include/ast/description.h".
Loader::ErrCode ExportDesc::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read external name to export.
  if ((Status = Mgr.readName(ExtName)) != Loader::ErrCode::Success)
    return Status;

  /// Read external type.
  unsigned char Byte = 0;
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  ExtType = static_cast<ExternalType>(Byte);
  switch (ExtType) {
  case ExternalType::Function:
  case ExternalType::Table:
  case ExternalType::Memory:
  case ExternalType::Global:
    break;
  default:
    return Loader::ErrCode::InvalidGrammar;
  }

  /// Read external index to export.
  return Mgr.readU32(ExtIdx);
}

} // namespace AST
} // namespace SSVM