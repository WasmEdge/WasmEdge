// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Loader {

// Load binary to construct Module node. See "include/loader/loader.h".
Expect<std::unique_ptr<AST::Module>> Loader::loadModule() {
  // When entering this function, the `Loader::WASMType` should be
  // `Loader::InputType::SharedLibrary` or `Loader::InputType::WASM`.

  auto Mod = std::make_unique<AST::Module>();
  // Read Magic and Version sequences.
  if (auto Res = FMgr.readBytes(4)) {
    std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
    if (*Res != WasmMagic) {
      return logLoadError(ErrCode::Value::MalformedMagic, FMgr.getLastOffset(),
                          ASTNodeAttr::Module);
    }
    Mod->getMagic() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(), ASTNodeAttr::Module);
  }
  if (auto Res = FMgr.readBytes(4)) {
    std::vector<Byte> WasmVersion = {0x01, 0x00, 0x00, 0x00};
    if (*Res != WasmVersion) {
      return logLoadError(ErrCode::Value::MalformedVersion,
                          FMgr.getLastOffset(), ASTNodeAttr::Module);
    }
    Mod->getVersion() = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(), ASTNodeAttr::Module);
  }

  // Find and Read the AOT custom section first. Jump the others.
  // This loop is for checking the input is an universal WASM or not.
  // Therefore, if the configure is set as force interpreter mode, skip this.
  while (!Conf.getRuntimeConfigure().isForceInterpreter() &&
         WASMType != InputType::SharedLibrary) {
    // This loop only overview the custom sections and read the AOT section.
    // For the other general errors, break and handle in the sequencially
    // parsing below.
    uint8_t NewSectionId = 0x00;
    if (auto Res = FMgr.readByte()) {
      NewSectionId = *Res;
    } else {
      break;
    }

    if (NewSectionId == 0x00U) {
      // Load the section size.
      uint32_t ContentSize = 0;
      if (auto Res = FMgr.readU32()) {
        ContentSize = *Res;
      } else {
        break;
      }
      if (ContentSize > FMgr.getRemainSize()) {
        break;
      }

      // Load the section name.
      auto StartOffset = FMgr.getOffset();
      std::string Name;
      if (auto Res = FMgr.readName()) {
        // The UTF-8 failed case will be ignored here.
        Name = std::move(*Res);
      }

      auto ReadSize = FMgr.getOffset() - StartOffset;
      if (ContentSize < ReadSize) {
        // Syntax error of overread. Jump to the next section.
        FMgr.seek(StartOffset + ContentSize);
        continue;
      }

      if (Name == "wasmedge") {
        // Found the AOT section in universal WASM. Load the AOT code.
        // Read the content.
        std::vector<uint8_t> Content;
        if (auto Res = FMgr.readBytes(ContentSize - ReadSize)) {
          Content = std::move(*Res);
        } else {
          break;
        }

        // Load the AOT section.
        FileMgr VecMgr;
        AST::AOTSection NewAOTSection;
        VecMgr.setCode(Content);
        if (auto Res = loadSection(VecMgr, NewAOTSection)) {
          // Also handle the duplicated AOT sections case.
          // If the new AOT section discovered, use the new one.
          WASMType = InputType::UniversalWASM;
          Mod->getAOTSection() = std::move(NewAOTSection);
        } else {
          // If the new AOT section load failed, use the old one or the
          // interpreter mode.
          if (WASMType == InputType::UniversalWASM) {
            spdlog::info(
                "    Load AOT section failed. Use the previous succeeded one.");
          } else {
            spdlog::info(
                "    Load AOT section failed. Use interpreter mode instead.");
          }
        }
      } else {
        // Found other custom sections. Jump to the next section.
        FMgr.seek(StartOffset + ContentSize);
        continue;
      }
    } else {
      if (auto Res = FMgr.jumpContent(); unlikely(!Res)) {
        break;
      }
    }
  }

  // Seek to the position after the binary header.
  FMgr.seek(8);

  // Variables to record the loaded section types.
  HasDataSection = false;
  std::bitset<0x0DU> Secs;

  // Read Section index and create Section nodes.
  while (true) {
    uint8_t NewSectionId = 0x00;
    // If not read section ID, seems the end of file and break.
    if (auto Res = FMgr.readByte()) {
      NewSectionId = *Res;
    } else {
      if (Res.error() == ErrCode::Value::UnexpectedEnd) {
        break;
      } else {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Module);
      }
    }

    // Sections except the custom section should be unique.
    if (NewSectionId > 0x00U && NewSectionId < 0x0DU &&
        Secs.test(NewSectionId)) {
      return logLoadError(ErrCode::Value::JunkSection, FMgr.getLastOffset(),
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
      // This section is for BulkMemoryOperations or ReferenceTypes proposal.
      if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
          !Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(ErrCode::Value::MalformedSection,
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
      return logLoadError(ErrCode::Value::MalformedSection,
                          FMgr.getLastOffset(), ASTNodeAttr::Module);
    }
  }

  // Verify the function section and code section are matched.
  if (Mod->getFunctionSection().getContent().size() !=
      Mod->getCodeSection().getContent().size()) {
    spdlog::error(ErrCode::Value::IncompatibleFuncCode);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::IncompatibleFuncCode);
  }

  // Verify the data count section and data segments are matched.
  if (Mod->getDataCountSection().getContent()) {
    if (Mod->getDataSection().getContent().size() !=
        *(Mod->getDataCountSection().getContent())) {
      spdlog::error(ErrCode::Value::IncompatibleDataCount);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(ErrCode::Value::IncompatibleDataCount);
    }
  }

  // Load library from AOT Section for the universal WASM case.
  // For the force interpreter mode, skip this.
  if (WASMType == InputType::UniversalWASM &&
      !Conf.getRuntimeConfigure().isForceInterpreter()) {
    bool FallBackInterpreter = false;
    auto Library = std::make_shared<SharedLibrary>();
    if (auto Res = Library->load(Mod->getAOTSection()); unlikely(!Res)) {
      spdlog::error("    AOT section -- library load failed:{} , use "
                    "interpreter mode instead.",
                    Res.error());
      FallBackInterpreter = true;
    }

    // Check the symbols.
    auto FuncTypeSymbols = Library->getTypes<AST::FunctionType::Wrapper>();
    auto CodeSymbols = Library->getCodes<void>();
    auto IntrinsicsSymbol =
        Library->getIntrinsics<const AST::Module::IntrinsicsTable *>();
    auto &FuncTypes = Mod->getTypeSection().getContent();
    auto &CodeSegs = Mod->getCodeSection().getContent();
    if (!FallBackInterpreter &&
        unlikely(FuncTypeSymbols.size() != FuncTypes.size())) {
      spdlog::error("    AOT section -- number of types not matching:{} {}, "
                    "use interpreter mode instead.",
                    FuncTypeSymbols.size(), FuncTypes.size());
      FallBackInterpreter = true;
    }
    if (!FallBackInterpreter &&
        unlikely(CodeSymbols.size() != CodeSegs.size())) {
      spdlog::error("    AOT section -- number of codes not matching:{} {}, "
                    "use interpreter mode instead.",
                    CodeSymbols.size(), CodeSegs.size());
      FallBackInterpreter = true;
    }
    if (!FallBackInterpreter && unlikely(!IntrinsicsSymbol)) {
      spdlog::error("    AOT section -- intrinsics table symbol not found, use "
                    "interpreter mode instead.");
      FallBackInterpreter = true;
    }

    // Set the symbols into the module.
    if (!FallBackInterpreter) {
      for (size_t I = 0; I < FuncTypes.size(); ++I) {
        FuncTypes[I].setSymbol(std::move(FuncTypeSymbols[I]));
      }
      for (size_t I = 0; I < CodeSegs.size(); ++I) {
        CodeSegs[I].setSymbol(std::move(CodeSymbols[I]));
      }
      Mod->setSymbol(std::move(IntrinsicsSymbol));
    } else {
      // Fallback to the interpreter mode case: Re-read the code section.
      WASMType = InputType::WASM;
      FMgr.seek(Mod->getCodeSection().getStartOffset());
      if (auto Res = loadSection(Mod->getCodeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
    }
  }

  return Mod;
}

// Load compiled function from loadable manager. See "include/loader/loader.h".
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
