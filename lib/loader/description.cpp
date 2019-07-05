#include "loader/description.h"

namespace AST {

bool ImportDesc::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;

  if (!Mgr.readName(ModName))
    return false;
  if (!Mgr.readName(ExtName))
    return false;

  if (!Mgr.readByte(Byte))
    return false;
  ExtType = static_cast<ExternalType>(Byte);

  switch (ExtType) {
  case ExternalType::Function: {
    unsigned int TypeIdx = 0;
    if (!Mgr.readU32(TypeIdx))
      return false;
    ExtContent = std::make_unique<unsigned int>(TypeIdx);
    break;
  }
  case ExternalType::Table: {
    auto NewTable = std::make_unique<TableType>();
    if (!NewTable->loadBinary(Mgr))
      return false;
    ExtContent = std::move(NewTable);
    break;
  }
  case ExternalType::Memory: {
    auto NewMemory = std::make_unique<MemoryType>();
    if (!NewMemory->loadBinary(Mgr))
      return false;
    ExtContent = std::move(NewMemory);
    break;
  }
  case ExternalType::Global: {
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

bool ExportDesc::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;

  if (!Mgr.readName(ExtName))
    return false;

  if (!Mgr.readByte(Byte))
    return false;
  ExtType = static_cast<ExternalType>(Byte);
  return Mgr.readU32(ExtIdx);
}

} // namespace AST