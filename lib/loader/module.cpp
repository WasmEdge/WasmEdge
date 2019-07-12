#include "loader/module.h"

namespace AST {

/// Load binary to construct Module node. See "include/loader/module.h".
bool Module::loadBinary(FileMgr &Mgr) {
  /// Read Magic and Version sequences.
  std::cout << " - Module \n";
  if (!Mgr.readBytes(Magic, 4))
    return false;
  std::cout << "   | Magic: ";
  for (auto it = Magic.begin(); it != Magic.end(); it++)
    printf("0x%02X ", *it);
  std::cout << std::endl;
  if (!Mgr.readBytes(Version, 4))
    return false;
  std::cout << "   | Version: ";
  for (auto it = Version.begin(); it != Version.end(); it++)
    printf("0x%02X ", *it);
  std::cout << std::endl;

  /// Read Section index and create Section nodes.
  unsigned char NewSectionId = 0x00;
  while (Mgr.readByte(NewSectionId)) {
    switch (NewSectionId) {
    case 0x00:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Custom section\n";
      CustomSec = std::make_unique<CustomSection>();
      if (!CustomSec->loadBinary(Mgr))
        return false;
      break;
    case 0x01:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Type section\n";
      TypeSec = std::make_unique<TypeSection>();
      if (!TypeSec->loadBinary(Mgr))
        return false;
      break;
    case 0x02:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Import section\n";
      ImportSec = std::make_unique<ImportSection>();
      if (!ImportSec->loadBinary(Mgr))
        return false;
      break;
    case 0x03:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Function section\n";
      FunctionSec = std::make_unique<FunctionSection>();
      if (!FunctionSec->loadBinary(Mgr))
        return false;
      break;
    case 0x04:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Table section\n";
      TableSec = std::make_unique<TableSection>();
      if (!TableSec->loadBinary(Mgr))
        return false;
      break;
    case 0x05:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Memory section\n";
      MemorySec = std::make_unique<MemorySection>();
      if (!MemorySec->loadBinary(Mgr))
        return false;
      break;
    case 0x06:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Custom section\n";
      GlobalSec = std::make_unique<GlobalSection>();
      if (!GlobalSec->loadBinary(Mgr))
        return false;
      break;
    case 0x07:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Export section\n";
      ExportSec = std::make_unique<ExportSection>();
      if (!ExportSec->loadBinary(Mgr))
        return false;
      break;
    case 0x08:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Start section\n";
      StartSec = std::make_unique<StartSection>();
      if (!StartSec->loadBinary(Mgr))
        return false;
      break;
    case 0x09:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Element section\n";
      ElementSec = std::make_unique<ElementSection>();
      if (!ElementSec->loadBinary(Mgr))
        return false;
      break;
    case 0x0A:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Code section\n";
      CodeSec = std::make_unique<CodeSection>();
      if (!CodeSec->loadBinary(Mgr))
        return false;
      break;
    case 0x0B:
      std::cout << "   - Section: " << static_cast<int>(NewSectionId)
                << " Data section\n";
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