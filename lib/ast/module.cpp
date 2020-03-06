// SPDX-License-Identifier: Apache-2.0
#include "common/ast/module.h"

namespace SSVM {
namespace AST {

/// Load binary to construct Module node. See "include/ast/module.h".
Expect<void> Module::loadBinary(FileMgr &Mgr) {
  /// Read Magic and Version sequences.
  if (auto Res = Mgr.readBytes(4)) {
    Magic = *Res;
  } else {
    return Unexpect(Res);
  }
  if (auto Res = Mgr.readBytes(4)) {
    Version = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Read Section index and create Section nodes.
  while (true) {
    uint8_t NewSectionId = 0x00;
    /// If not read section ID, seems the end of file and break.
    if (auto Res = Mgr.readByte()) {
      NewSectionId = *Res;
    } else {
      if (Res.error() == ErrCode::EndOfFile) {
        break;
      } else {
        return Unexpect(Res);
      }
    }

    switch (NewSectionId) {
    case 0x00:
      CustomSec = std::make_unique<CustomSection>();
      if (auto Res = CustomSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x01:
      TypeSec = std::make_unique<TypeSection>();
      if (auto Res = TypeSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x02:
      ImportSec = std::make_unique<ImportSection>();
      if (auto Res = ImportSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x03:
      FunctionSec = std::make_unique<FunctionSection>();
      if (auto Res = FunctionSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x04:
      TableSec = std::make_unique<TableSection>();
      if (auto Res = TableSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x05:
      MemorySec = std::make_unique<MemorySection>();
      if (auto Res = MemorySec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x06:
      GlobalSec = std::make_unique<GlobalSection>();
      if (auto Res = GlobalSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x07:
      ExportSec = std::make_unique<ExportSection>();
      if (auto Res = ExportSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x08:
      StartSec = std::make_unique<StartSection>();
      if (auto Res = StartSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x09:
      ElementSec = std::make_unique<ElementSection>();
      if (auto Res = ElementSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x0A:
      CodeSec = std::make_unique<CodeSection>();
      if (auto Res = CodeSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    case 0x0B:
      DataSec = std::make_unique<DataSection>();
      if (auto Res = DataSec->loadBinary(Mgr); !Res) {
        return Unexpect(Res);
      }
      break;
    default:
      return Unexpect(ErrCode::InvalidGrammar);
    }
  }
  return {};
}

} // namespace AST
} // namespace SSVM
