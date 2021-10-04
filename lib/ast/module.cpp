// SPDX-License-Identifier: Apache-2.0
#include <bitset>

#include "ast/module.h"
#include "common/log.h"

namespace WasmEdge {
namespace AST {

/// Load binary to construct Module node. See "include/ast/module.h".
Expect<void> Module::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read Magic and Version sequences.
  if (auto Res = Mgr.readBytes(4)) {
    Magic = *Res;
    std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
    if (Magic != WasmMagic) {
      return logLoadError(ErrCode::MalformedMagic, Mgr.getLastOffset(),
                          NodeAttr);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  if (auto Res = Mgr.readBytes(4)) {
    Version = *Res;
    std::vector<Byte> WasmVersion = {0x01, 0x00, 0x00, 0x00};
    if (Version != WasmVersion) {
      return logLoadError(ErrCode::MalformedVersion, Mgr.getLastOffset(),
                          NodeAttr);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }

  /// Copy the configure to set `HasDataCountSection` flag if read the datacount
  /// section.
  Configure CopyConf = Conf;
  std::bitset<0x0DU> Secs;

  /// Read Section index and create Section nodes.
  while (true) {
    uint8_t NewSectionId = 0x00;
    /// If not read section ID, seems the end of file and break.
    if (auto Res = Mgr.readByte()) {
      NewSectionId = *Res;
    } else {
      if (Res.error() == ErrCode::UnexpectedEnd) {
        break;
      } else {
        return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
      }
    }

    /// Sections except the custom section should be unique.
    if (NewSectionId > 0x00U && NewSectionId < 0x0DU &&
        Secs.test(NewSectionId)) {
      return logLoadError(ErrCode::JunkSection, Mgr.getLastOffset(), NodeAttr);
    }

    switch (NewSectionId) {
    case 0x00:
      CustomSecs.emplace_back();
      if (auto Res = CustomSecs.back().loadBinary(Mgr, CopyConf); !Res) {
        CustomSecs.pop_back();
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      break;
    case 0x01:
      if (auto Res = TypeSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x02:
      if (auto Res = ImportSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x03:
      if (auto Res = FunctionSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x04:
      if (auto Res = TableSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x05:
      if (auto Res = MemorySec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x06:
      if (auto Res = GlobalSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x07:
      if (auto Res = ExportSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x08:
      if (auto Res = StartSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x09:
      if (auto Res = ElementSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0A:
      if (auto Res = CodeSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0B:
      if (auto Res = DataSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0C:
      /// This section is for BulkMemoryOperations or ReferenceTypes proposal.
      if (!CopyConf.hasProposal(Proposal::BulkMemoryOperations) &&
          !CopyConf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(ErrCode::MalformedSection,
                               Proposal::BulkMemoryOperations,
                               Mgr.getLastOffset(), NodeAttr);
      }
      if (auto Res = DataCountSec.loadBinary(Mgr, CopyConf); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttr));
        return Unexpect(Res);
      }
      CopyConf.getRuntimeConfigure().addDataCountSection();
      Secs.set(NewSectionId);
      break;
    default:
      return logLoadError(ErrCode::MalformedSection, Mgr.getLastOffset(),
                          NodeAttr);
    }
  }

  /// Verify the function section and code section are matched.
  if (FunctionSec.getContent().size() != CodeSec.getContent().size()) {
    spdlog::error(ErrCode::IncompatibleFuncCode);
    spdlog::error(ErrInfo::InfoAST(NodeAttr));
    return Unexpect(ErrCode::IncompatibleFuncCode);
  }

  /// Verify the data count section and data segments are matched.
  if (DataCountSec.getContent()) {
    if (DataSec.getContent().size() != *DataCountSec.getContent()) {
      spdlog::error(ErrCode::IncompatibleDataCount);
      spdlog::error(ErrInfo::InfoAST(NodeAttr));
      return Unexpect(ErrCode::IncompatibleDataCount);
    }
  }

  /// Load Custom Sections
  for (const auto &CustomSec : CustomSecs) {
    const auto &Name = CustomSec.getName();
    if (Name == "wasmedge") {
      {
        FileMgr VecMgr;
        VecMgr.setCode(CustomSec.getContent());
        if (auto Res = AOTSec.loadBinary(VecMgr, Conf); unlikely(!Res)) {
          spdlog::error("load failed:{}", Res.error());
          continue;
        }
      }

      auto Library = std::make_shared<Loader::SharedLibrary>();
      if (auto Res = Library->load(AOTSec); unlikely(!Res)) {
        spdlog::error("library load failed:{}", Res.error());
        continue;
      }

      auto &FuncTypes = TypeSec.getContent();
      if (auto Symbols = Library->getTypes<FunctionType::Wrapper>();
          unlikely(Symbols.size() != FuncTypes.size())) {
        spdlog::error("number of types not matching:{} {}", Symbols.size(),
                      FuncTypes.size());
        continue;
      } else {
        for (size_t I = 0; I < FuncTypes.size(); ++I) {
          FuncTypes[I].setSymbol(std::move(Symbols[I]));
        }
      }
      auto &CodeSegs = CodeSec.getContent();
      if (auto Symbols = Library->getCodes<void>();
          unlikely(Symbols.size() != CodeSegs.size())) {
        spdlog::error("number of codes not matching:{} {}", Symbols.size(),
                      CodeSegs.size());
        continue;
      } else {
        for (size_t I = 0; I < CodeSegs.size(); ++I) {
          CodeSegs[I].setSymbol(std::move(Symbols[I]));
        }
      }
      if (auto Symbol =
              Library->getIntrinsics<const AST::Module::IntrinsicsTable *>()) {
        setSymbol(std::move(Symbol));
      }
      break;
    }
  }

  return {};
}

/// Load compiled function from loadable manager. See "include/ast/module.h".
Expect<void> Module::loadCompiled(LDMgr &Mgr) {
  auto &FuncTypes = TypeSec.getContent();
  for (size_t I = 0; I < FuncTypes.size(); ++I) {
    const std::string Name = "t" + std::to_string(I);
    if (auto Symbol = Mgr.getSymbol<FunctionType::Wrapper>(Name.c_str())) {
      FuncTypes[I].setSymbol(Symbol);
    }
  }
  size_t Offset = 0;
  for (const auto &ImpDesc : ImportSec.getContent()) {
    if (ImpDesc.getExternalType() == ExternalType::Function) {
      ++Offset;
    }
  }
  auto &CodeSegs = CodeSec.getContent();
  for (size_t I = 0; I < CodeSegs.size(); ++I) {
    const std::string Name = "f" + std::to_string(I + Offset);
    if (auto Symbol = Mgr.getSymbol<void>(Name.c_str())) {
      CodeSegs[I].setSymbol(Symbol);
    }
  }
  return {};
}

} // namespace AST
} // namespace WasmEdge
