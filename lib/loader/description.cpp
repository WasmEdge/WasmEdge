#include "loader/description.h"

namespace AST {

/// Load binary of Import description. See "include/loader/description.h".
Base::ErrCode ImportDesc::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the module name.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readName(ModName))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read the external name.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readName(ExtName))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read the external type.
  unsigned char Byte = 0;
  if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte))) !=
      Base::ErrCode::Success)
    return Status;
  ExtType = static_cast<ExternalType>(Byte);

  /// Make content node according to external type.
  switch (ExtType) {
  case ExternalType::Function: {
    /// Read the function type index.
    unsigned int TypeIdx = 0;
    Status = static_cast<Base::ErrCode>(Mgr.readU32(TypeIdx));
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
    Status = Base::ErrCode::InvalidGrammar;
    break;
  }
  return Status;
}

/// Load binary of Export description. See "include/loader/description.h".
Base::ErrCode ExportDesc::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read external name to export.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readName(ExtName))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read external type.
  unsigned char Byte = 0;
  if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(Byte))) !=
      Base::ErrCode::Success)
    return Status;
  ExtType = static_cast<ExternalType>(Byte);
  switch (ExtType) {
  case ExternalType::Function:
  case ExternalType::Table:
  case ExternalType::Memory:
  case ExternalType::Global:
    break;
  default:
    return Base::ErrCode::InvalidGrammar;
  }

  /// Read external index to export.
  return static_cast<Base::ErrCode>(Mgr.readU32(ExtIdx));
}

} // namespace AST