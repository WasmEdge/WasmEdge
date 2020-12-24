// SPDX-License-Identifier: Apache-2.0
#include "ast/module.h"
#include "common/log.h"

namespace SSVM {
namespace AST {

/// Load binary to construct Module node. See "include/ast/module.h".
Expect<void> Module::loadBinary(FileMgr &Mgr, const ProposalConfigure &PConf) {
  /// Read Magic and Version sequences.
  if (auto Res = Mgr.readBytes(4)) {
    Magic = *Res;
    std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
    if (Magic != WasmMagic) {
      LOG(ERROR) << ErrCode::InvalidGrammar;
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 4);
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(ErrCode::InvalidGrammar);
    }
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  if (auto Res = Mgr.readBytes(4)) {
    Version = *Res;
    std::vector<Byte> WasmVersion = {0x01, 0x00, 0x00, 0x00};
    if (Version != WasmVersion) {
      LOG(ERROR) << ErrCode::InvalidGrammar;
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 4);
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(ErrCode::InvalidGrammar);
    }
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
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
        LOG(ERROR) << Res.error();
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
    }

    switch (NewSectionId) {
    case 0x00:
      if (CustomSec == nullptr) {
        CustomSec = std::make_unique<CustomSection>();
      }
      if (auto Res = CustomSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x01:
      if (TypeSec == nullptr) {
        TypeSec = std::make_unique<TypeSection>();
      }
      if (auto Res = TypeSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x02:
      if (ImportSec == nullptr) {
        ImportSec = std::make_unique<ImportSection>();
      }
      if (auto Res = ImportSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x03:
      if (FunctionSec == nullptr) {
        FunctionSec = std::make_unique<FunctionSection>();
      }
      if (auto Res = FunctionSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x04:
      if (TableSec == nullptr) {
        TableSec = std::make_unique<TableSection>();
      }
      if (auto Res = TableSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x05:
      if (MemorySec == nullptr) {
        MemorySec = std::make_unique<MemorySection>();
      }
      if (auto Res = MemorySec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x06:
      if (GlobalSec == nullptr) {
        GlobalSec = std::make_unique<GlobalSection>();
      }
      if (auto Res = GlobalSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x07:
      if (ExportSec == nullptr) {
        ExportSec = std::make_unique<ExportSection>();
      }
      if (auto Res = ExportSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x08:
      if (StartSec == nullptr) {
        StartSec = std::make_unique<StartSection>();
      }
      if (auto Res = StartSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x09:
      if (ElementSec == nullptr) {
        ElementSec = std::make_unique<ElementSection>();
      }
      if (auto Res = ElementSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x0A:
      if (CodeSec == nullptr) {
        CodeSec = std::make_unique<CodeSection>();
      }
      if (auto Res = CodeSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x0B:
      if (DataSec == nullptr) {
        DataSec = std::make_unique<DataSection>();
      }
      if (auto Res = DataSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x0C:
      /// This section is for BulkMemoryOperations or ReferenceTypes proposal.
      if (!PConf.hasProposal(Proposal::BulkMemoryOperations) &&
          !PConf.hasProposal(Proposal::ReferenceTypes)) {
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoProposal(Proposal::BulkMemoryOperations);
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(ErrCode::InvalidGrammar);
      }
      if (DataCountSec == nullptr) {
        DataCountSec = std::make_unique<DataCountSection>();
      }
      if (auto Res = DataCountSec->loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    default:
      LOG(ERROR) << ErrCode::InvalidGrammar;
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(ErrCode::InvalidGrammar);
    }
  }

  /// Verify the data count section and data segments are matched.
  if (DataCountSec != nullptr) {
    if (DataSec != nullptr) {
      if (DataSec->getContent().size() != DataCountSec->getContent()) {
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      if (DataCountSec->getContent() != 0) {
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    }
  }
  return {};
}

/// Load compiled function from loadable manager. See "include/ast/module.h".
Expect<void> Module::loadCompiled(LDMgr &Mgr) {
  if (TypeSec) {
    if (auto Symbol = Mgr.getSymbol<FunctionType::Wrapper *[]>("types")) {
      auto &TypeSecs = TypeSec->getContent();
      for (size_t I = 0; I < TypeSecs.size(); ++I) {
        TypeSecs[I].setSymbol(Symbol.index(I).deref());
      }
    }
  }
  if (CodeSec) {
    if (auto Symbol = Mgr.getSymbol<void *[]>("codes")) {
      auto &CodeSecs = CodeSec->getContent();
      for (size_t I = 0; I < CodeSecs.size(); ++I) {
        CodeSecs[I].setSymbol(Symbol.index(I).deref());
      }
    }
  }
  return {};
}

} // namespace AST
} // namespace SSVM
