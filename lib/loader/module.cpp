#include "loader/module.h"

namespace AST {

/// Load binary to construct Module node. See "include/loader/module.h".
bool Module::loadBinary(FileMgr &Mgr) {
  /// Read Magic and Version sequences.
  if (!Mgr.readSize(Magic, 4))
    return false;
  if (!Mgr.readSize(Version, 4))
    return false;

  /// Read Section index and create Section nodes.
  unsigned char NewSectionId = 0x00;
  while (Mgr.readByte(NewSectionId)) {
    switch (NewSectionId) {
    case 0x00:
      CustomSec = std::make_unique<CustomSection>();
      if (!CustomSec->loadBinary(Mgr))
        return false;
      break;
    case 0x01:
      TypeSec = std::make_unique<TypeSection>();
      if (!TypeSec->loadBinary(Mgr))
        return false;
      break;
    case 0x02:
      ImportSec = std::make_unique<ImportSection>();
      if (!ImportSec->loadBinary(Mgr))
        return false;
      break;
    case 0x03:
      FunctionSec = std::make_unique<FunctionSection>();
      if (!FunctionSec->loadBinary(Mgr))
        return false;
      break;
    case 0x04:
      TableSec = std::make_unique<TableSection>();
      if (!TableSec->loadBinary(Mgr))
        return false;
      break;
    case 0x05:
      MemorySec = std::make_unique<MemorySection>();
      if (!MemorySec->loadBinary(Mgr))
        return false;
      break;
    case 0x06:
      GlobalSec = std::make_unique<GlobalSection>();
      if (!GlobalSec->loadBinary(Mgr))
        return false;
      break;
    case 0x07:
      ExportSec = std::make_unique<ExportSection>();
      if (!ExportSec->loadBinary(Mgr))
        return false;
      break;
    case 0x08:
      StartSec = std::make_unique<StartSection>();
      if (!StartSec->loadBinary(Mgr))
        return false;
      break;
    case 0x09:
      ElementSec = std::make_unique<ElementSection>();
      if (!ElementSec->loadBinary(Mgr))
        return false;
      break;
    case 0x0A:
      CodeSec = std::make_unique<CodeSection>();
      if (!CodeSec->loadBinary(Mgr))
        return false;
      break;
    case 0x0B:
      DataSec = std::make_unique<DataSection>();
      if (!DataSec->loadBinary(Mgr))
        return false;
      break;
    default:
      break;
    }
  }
  return true;
}

} // namespace AST