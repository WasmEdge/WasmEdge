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
      return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 4,
                          NodeAttr);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  if (auto Res = Mgr.readBytes(4)) {
    Version = *Res;
    std::vector<Byte> WasmVersion = {0x01, 0x00, 0x00, 0x00};
    if (Version != WasmVersion) {
      return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 4,
                          NodeAttr);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
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
        return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
      }
    }

    switch (NewSectionId) {
    case 0x00:
      /// TODO: Handle the messages in custom section.
      if (auto Res = CustomSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x01:
      if (auto Res = TypeSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x02:
      if (auto Res = ImportSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x03:
      if (auto Res = FunctionSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x04:
      if (auto Res = TableSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x05:
      if (auto Res = MemorySec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x06:
      if (auto Res = GlobalSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x07:
      if (auto Res = ExportSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x08:
      if (StartSec.getContent()) {
        /// Start section should be unique.
        logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1, NodeAttr);
      }
      if (auto Res = StartSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x09:
      if (auto Res = ElementSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x0A:
      if (auto Res = CodeSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x0B:
      if (auto Res = DataSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    case 0x0C:
      /// This section is for BulkMemoryOperations or ReferenceTypes proposal.
      if (!PConf.hasProposal(Proposal::BulkMemoryOperations) &&
          !PConf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(ErrCode::InvalidGrammar,
                               Proposal::BulkMemoryOperations,
                               Mgr.getOffset() - 1, NodeAttr);
      }
      if (DataCountSec.getContent()) {
        /// Data count section should be unique.
        logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1, NodeAttr);
      }
      if (auto Res = DataCountSec.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      break;
    default:
      return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1,
                          NodeAttr);
    }
  }

  /// Verify the data count section and data segments are matched.
  if (DataCountSec.getContent()) {
    if (DataSec.getContent().size() != *DataCountSec.getContent()) {
      LOG(ERROR) << ErrCode::InvalidGrammar;
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(ErrCode::InvalidGrammar);
    }
  }
  return {};
}

/// Load compiled function from loadable manager. See "include/ast/module.h".
Expect<void> Module::loadCompiled(LDMgr &Mgr) {
  if (auto Symbol = Mgr.getSymbol<FunctionType::Wrapper *[]>("types")) {
    auto &FuncTypes = TypeSec.getContent();
    for (size_t I = 0; I < FuncTypes.size(); ++I) {
      FuncTypes[I].setSymbol(Symbol.index(I).deref());
    }
  }
  if (auto Symbol = Mgr.getSymbol<void *[]>("codes")) {
    auto &CodeSegs = CodeSec.getContent();
    for (size_t I = 0; I < CodeSegs.size(); ++I) {
      CodeSegs[I].setSymbol(Symbol.index(I).deref());
    }
  }
  return {};
}

} // namespace AST
} // namespace SSVM
