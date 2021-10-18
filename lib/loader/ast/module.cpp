// SPDX-License-Identifier: Apache-2.0

#include "loader/loader.h"

#include <bitset>

namespace WasmEdge {
namespace Loader {

/// Load binary to construct Module node. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>> Loader::loadModule() {
  auto Mod = std::make_unique<AST::Module>();
  /// Read Magic and Version sequences.
  if (auto Res = FMgr.readBytes(4)) {
    std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
    if (*Res != WasmMagic) {
      return logLoadError(ErrCode::MalformedMagic, FMgr.getLastOffset(),
                          ASTNodeAttr::Module);
    }
    Mod->getMagic() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(), ASTNodeAttr::Module);
  }
  if (auto Res = FMgr.readBytes(4)) {
    std::vector<Byte> WasmVersion = {0x01, 0x00, 0x00, 0x00};
    if (*Res != WasmVersion) {
      return logLoadError(ErrCode::MalformedVersion, FMgr.getLastOffset(),
                          ASTNodeAttr::Module);
    }
    Mod->getVersion() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(), ASTNodeAttr::Module);
  }

  /// Variables to record the loaded section types.
  HasDataSection = false;
  std::bitset<0x0DU> Secs;

  /// Read Section index and create Section nodes.
  while (true) {
    uint8_t NewSectionId = 0x00;
    /// If not read section ID, seems the end of file and break.
    if (auto Res = FMgr.readByte()) {
      NewSectionId = *Res;
    } else {
      if (Res.error() == ErrCode::UnexpectedEnd) {
        break;
      } else {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Module);
      }
    }

    /// Sections except the custom section should be unique.
    if (NewSectionId > 0x00U && NewSectionId < 0x0DU &&
        Secs.test(NewSectionId)) {
      return logLoadError(ErrCode::JunkSection, FMgr.getLastOffset(),
                          ASTNodeAttr::Module);
    }

    switch (NewSectionId) {
    case 0x00:
      Mod->getCustomSections().emplace_back();
      if (auto Res = loadSection(Mod->getCustomSections().back()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      break;
    case 0x01:
      if (auto Res = loadSection(Mod->getTypeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x02:
      if (auto Res = loadSection(Mod->getImportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x03:
      if (auto Res = loadSection(Mod->getFunctionSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x04:
      if (auto Res = loadSection(Mod->getTableSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x05:
      if (auto Res = loadSection(Mod->getMemorySection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x06:
      if (auto Res = loadSection(Mod->getGlobalSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x07:
      if (auto Res = loadSection(Mod->getExportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x08:
      if (auto Res = loadSection(Mod->getStartSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x09:
      if (auto Res = loadSection(Mod->getElementSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0A:
      if (auto Res = loadSection(Mod->getCodeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0B:
      if (auto Res = loadSection(Mod->getDataSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0C:
      /// This section is for BulkMemoryOperations or ReferenceTypes proposal.
      if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
          !Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(ErrCode::MalformedSection,
                               Proposal::BulkMemoryOperations,
                               FMgr.getLastOffset(), ASTNodeAttr::Module);
      }
      if (auto Res = loadSection(Mod->getDataCountSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      HasDataSection = true;
      Secs.set(NewSectionId);
      break;
    default:
      return logLoadError(ErrCode::MalformedSection, FMgr.getLastOffset(),
                          ASTNodeAttr::Module);
    }
  }

  /// Verify the function section and code section are matched.
  if (Mod->getFunctionSection().getContent().size() !=
      Mod->getCodeSection().getContent().size()) {
    spdlog::error(ErrCode::IncompatibleFuncCode);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::IncompatibleFuncCode);
  }

  /// Verify the data count section and data segments are matched.
  if (Mod->getDataCountSection().getContent()) {
    if (Mod->getDataSection().getContent().size() !=
        *(Mod->getDataCountSection().getContent())) {
      spdlog::error(ErrCode::IncompatibleDataCount);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(ErrCode::IncompatibleDataCount);
    }
  }

  /// Load Custom Sections
  for (const auto &CustomSec : Mod->getCustomSections()) {
    const auto &Name = CustomSec.getName();
    if (Name == "wasmedge") {
      {
        FileMgr VecMgr;
        VecMgr.setCode(CustomSec.getContent());
        if (auto Res = loadSection(Mod->getAOTSection()); unlikely(!Res)) {
          spdlog::error("load failed:{}", Res.error());
          continue;
        }
      }

      auto Library = std::make_shared<SharedLibrary>();
      if (auto Res = loadSection(Mod->getAOTSection()); unlikely(!Res)) {
        spdlog::error("library load failed:{}", Res.error());
        continue;
      }

      auto &FuncTypes = Mod->getTypeSection().getContent();
      if (auto Symbols = Library->getTypes<AST::FunctionType::Wrapper>();
          unlikely(Symbols.size() != FuncTypes.size())) {
        spdlog::error("number of types not matching:{} {}", Symbols.size(),
                      FuncTypes.size());
        continue;
      } else {
        for (size_t I = 0; I < FuncTypes.size(); ++I) {
          FuncTypes[I].setSymbol(std::move(Symbols[I]));
        }
      }
      auto &CodeSegs = Mod->getCodeSection().getContent();
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
        Mod->setSymbol(std::move(Symbol));
      }
      break;
    }
  }

  return Mod;
}

/// Load compiled function from loadable manager. See "include/loader/loader.h".
Expect<void> Loader::loadCompiled(AST::Module &Mod) {
  auto &FuncTypes = Mod.getTypeSection().getContent();
  for (size_t I = 0; I < FuncTypes.size(); ++I) {
    const std::string Name = "t" + std::to_string(I);
    if (auto Symbol =
            LMgr.getSymbol<AST::FunctionType::Wrapper>(Name.c_str())) {
      FuncTypes[I].setSymbol(std::move(Symbol));
    }
  }
  size_t Offset = 0;
  for (const auto &ImpDesc : Mod.getImportSection().getContent()) {
    if (ImpDesc.getExternalType() == ExternalType::Function) {
      ++Offset;
    }
  }
  auto &CodeSegs = Mod.getCodeSection().getContent();
  for (size_t I = 0; I < CodeSegs.size(); ++I) {
    const std::string Name = "f" + std::to_string(I + Offset);
    if (auto Symbol = LMgr.getSymbol<void>(Name.c_str())) {
      CodeSegs[I].setSymbol(std::move(Symbol));
    }
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
