#include "loader/description.h"

namespace AST {

/// Load binary of Import description. See "include/loader/description.h".
bool ImportDesc::loadBinary(FileMgr &Mgr) {
  /// Read the module name.
  if (!Mgr.readName(ModName))
    return false;

  /// Read the external name.
  if (!Mgr.readName(ExtName))
    return false;

  /// Read the external type.
  unsigned char Byte = 0;
  if (!Mgr.readByte(Byte))
    return false;
  ExtType = static_cast<ExternalType>(Byte);

  /// Make content node according to external type.
  switch (ExtType) {
  case ExternalType::Function: {
    /// Read the function type index.
    unsigned int TypeIdx = 0;
    if (!Mgr.readU32(TypeIdx))
      return false;
    ExtContent = std::make_unique<unsigned int>(TypeIdx);
    break;
  }
  case ExternalType::Table: {
    /// Read and make table type node.
    auto NewTable = std::make_unique<TableType>();
    if (!NewTable->loadBinary(Mgr))
      return false;
    ExtContent = std::move(NewTable);
    break;
  }
  case ExternalType::Memory: {
    /// Read and make memory type node.
    auto NewMemory = std::make_unique<MemoryType>();
    if (!NewMemory->loadBinary(Mgr))
      return false;
    ExtContent = std::move(NewMemory);
    break;
  }
  case ExternalType::Global: {
    /// Read and make global type node.
    auto NewGlobal = std::make_unique<GlobalType>();
    if (!NewGlobal->loadBinary(Mgr))
      return false;
    ExtContent = std::move(NewGlobal);
    break;
  }
  default:
    break;
  }
  return true;
}

/// Load binary of Export description. See "include/loader/description.h".
bool ExportDesc::loadBinary(FileMgr &Mgr) {
  /// Read external name to export.
  if (!Mgr.readName(ExtName))
    return false;

  /// Read external type.
  unsigned char Byte = 0;
  if (!Mgr.readByte(Byte))
    return false;
  ExtType = static_cast<ExternalType>(Byte);

  /// Read external index to export.
  return Mgr.readU32(ExtIdx);
}

} // namespace AST