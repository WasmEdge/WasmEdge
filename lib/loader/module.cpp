#include "loader/module.h"

namespace AST {

/// Load binary to construct Module node. See "include/loader/module.h".
Loader::ErrCode Module::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read Magic and Version sequences.
  if ((Status = Mgr.readBytes(Magic, 4)) != Loader::ErrCode::Success)
    return Status;
  if ((Status = Mgr.readBytes(Version, 4)) != Loader::ErrCode::Success)
    return Status;

  /// Read Section index and create Section nodes.
  unsigned char NewSectionId = 0x00;
  while (Status == Loader::ErrCode::Success) {
    /// If not read section ID, seems the end of file and break.
    Status = Mgr.readByte(NewSectionId);
    if (Status == Loader::ErrCode::EndOfFile) {
      Status = Loader::ErrCode::Success;
      break;
    } else if (Status != Loader::ErrCode::Success) {
      break;
    }

    switch (NewSectionId) {
    case 0x00:
      CustomSec = std::make_unique<CustomSection>();
      Status = CustomSec->loadBinary(Mgr);
      break;
    case 0x01:
      TypeSec = std::make_unique<TypeSection>();
      Status = TypeSec->loadBinary(Mgr);
      break;
    case 0x02:
      ImportSec = std::make_unique<ImportSection>();
      Status = ImportSec->loadBinary(Mgr);
      break;
    case 0x03:
      FunctionSec = std::make_unique<FunctionSection>();
      Status = FunctionSec->loadBinary(Mgr);
      break;
    case 0x04:
      TableSec = std::make_unique<TableSection>();
      Status = TableSec->loadBinary(Mgr);
      break;
    case 0x05:
      MemorySec = std::make_unique<MemorySection>();
      Status = MemorySec->loadBinary(Mgr);
      break;
    case 0x06:
      GlobalSec = std::make_unique<GlobalSection>();
      Status = GlobalSec->loadBinary(Mgr);
      break;
    case 0x07:
      ExportSec = std::make_unique<ExportSection>();
      Status = ExportSec->loadBinary(Mgr);
      break;
    case 0x08:
      StartSec = std::make_unique<StartSection>();
      Status = StartSec->loadBinary(Mgr);
      break;
    case 0x09:
      ElementSec = std::make_unique<ElementSection>();
      Status = ElementSec->loadBinary(Mgr);
      break;
    case 0x0A:
      CodeSec = std::make_unique<CodeSection>();
      Status = CodeSec->loadBinary(Mgr);
      break;
    case 0x0B:
      DataSec = std::make_unique<DataSection>();
      Status = DataSec->loadBinary(Mgr);
      break;
    default:
      Status = Loader::ErrCode::InvalidGrammar;
      break;
    }
  }
  return Status;
}

} // namespace AST