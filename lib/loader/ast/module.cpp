// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/aot_section.h"
#include "loader/loader.h"
#include "loader/shared_library.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadModuleInBound(AST::Module &Mod,
                                       std::optional<uint64_t> Bound) {
  uint64_t StartOffset = FMgr.getOffset();

  // Variables to record the loaded section types.
  HasDataSection = false;
  std::bitset<0x0EU> Secs;

  uint64_t Offset = FMgr.getOffset();

  // Read Section index and create Section nodes.
  while (!Bound.has_value() || Bound.value() > Offset - StartOffset) {
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
      Mod.getCustomSections().emplace_back();
      if (auto Res = loadSection(Mod.getCustomSections().back()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      break;
    case 0x01:
      if (auto Res = loadSection(Mod.getTypeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x02:
      if (auto Res = loadSection(Mod.getImportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x03:
      if (auto Res = loadSection(Mod.getFunctionSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x04:
      if (auto Res = loadSection(Mod.getTableSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x05:
      if (auto Res = loadSection(Mod.getMemorySection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x06:
      if (auto Res = loadSection(Mod.getGlobalSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x07:
      if (auto Res = loadSection(Mod.getExportSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x08:
      if (auto Res = loadSection(Mod.getStartSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x09:
      if (auto Res = loadSection(Mod.getElementSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0A:
      if (auto Res = loadSection(Mod.getCodeSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    case 0x0B:
      if (auto Res = loadSection(Mod.getDataSection()); !Res) {
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
      if (auto Res = loadSection(Mod.getDataCountSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      HasDataSection = true;
      Secs.set(NewSectionId);
      break;
    case 0x0D:
      // This section is for ExceptionHandling proposal.
      if (!Conf.hasProposal(Proposal::ExceptionHandling)) {
        return logNeedProposal(ErrCode::Value::MalformedSection,
                               Proposal::ExceptionHandling,
                               FMgr.getLastOffset(), ASTNodeAttr::Module);
      }
      if (auto Res = loadSection(Mod.getTagSection()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
        return Unexpect(Res);
      }
      Secs.set(NewSectionId);
      break;
    default:
      return logLoadError(ErrCode::Value::MalformedSection,
                          FMgr.getLastOffset(), ASTNodeAttr::Module);
    }

    Offset = FMgr.getOffset();
  }

  setTagFunctionType(Mod.getTagSection(), Mod.getImportSection(),
                     Mod.getTypeSection());

  // Verify the function section and code section are matched.
  if (Mod.getFunctionSection().getContent().size() !=
      Mod.getCodeSection().getContent().size()) {
    spdlog::error(ErrCode::Value::IncompatibleFuncCode);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::IncompatibleFuncCode);
  }

  // Verify the data count section and data segments are matched.
  if (Mod.getDataCountSection().getContent()) {
    if (Mod.getDataSection().getContent().size() !=
        *(Mod.getDataCountSection().getContent())) {
      spdlog::error(ErrCode::Value::IncompatibleDataCount);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(ErrCode::Value::IncompatibleDataCount);
    }
  }

  return {};
}

// Load binary to construct Module node. See "include/loader/loader.h".
Expect<void> Loader::loadModule(AST::Module &Mod) {
  return loadModuleInBound(Mod, std::nullopt);
}

// Setup symbols from loaded binary. See "include/loader/loader.h".
Expect<void> Loader::loadExecutable(AST::Module &Mod,
                                    std::shared_ptr<Executable> Exec) {
  auto &SubTypes = Mod.getTypeSection().getContent();
  for (auto &SubType : SubTypes) {
    if (unlikely(!SubType.getCompositeType().isFunc())) {
      // TODO: GC - AOT: implement other composite types.
      spdlog::error(ErrCode::Value::MalformedSection);
      spdlog::error("    Currently AOT not support GC proposal yet.");
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(ErrCode::Value::MalformedSection);
    }
  }

  size_t Offset = 0;
  for (const auto &ImpDesc : Mod.getImportSection().getContent()) {
    if (ImpDesc.getExternalType() == ExternalType::Function) {
      ++Offset;
    }
  }
  auto &CodeSegs = Mod.getCodeSection().getContent();

  // Check the symbols.
  auto FuncTypeSymbols = Exec->getTypes(SubTypes.size());
  auto CodeSymbols = Exec->getCodes(Offset, CodeSegs.size());
  auto IntrinsicsSymbol = Exec->getIntrinsics();
  if (unlikely(FuncTypeSymbols.size() != SubTypes.size())) {
    spdlog::error("    AOT section -- number of types not matching:{} {}, "
                  "use interpreter mode instead.",
                  FuncTypeSymbols.size(), SubTypes.size());
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }
  if (unlikely(CodeSymbols.size() != CodeSegs.size())) {
    spdlog::error("    AOT section -- number of codes not matching:{} {}, "
                  "use interpreter mode instead.",
                  CodeSymbols.size(), CodeSegs.size());
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }
  if (unlikely(!IntrinsicsSymbol)) {
    spdlog::error("    AOT section -- intrinsics table symbol not found, use "
                  "interpreter mode instead.");
    return Unexpect(ErrCode::Value::IllegalGrammar);
  }

  // Set the symbols into the module.
  for (size_t I = 0; I < SubTypes.size(); ++I) {
    SubTypes[I].getCompositeType().getFuncType().setSymbol(
        std::move(FuncTypeSymbols[I]));
  }
  for (size_t I = 0; I < CodeSegs.size(); ++I) {
    CodeSegs[I].setSymbol(std::move(CodeSymbols[I]));
  }
  Mod.setSymbol(std::move(IntrinsicsSymbol));
  if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
    // If the configure is set to force interpreter mode, not to set the
    // symbol.
    if (auto &Symbol = Mod.getSymbol()) {
      *Symbol = IntrinsicsTable;
    }
  }

  return {};
}

Expect<void> Loader::loadUniversalWASM(AST::Module &Mod) {
  if (!Conf.getRuntimeConfigure().isForceInterpreter()) {
    auto Exec = std::make_shared<AOTSection>();
    if (auto Res = Exec->load(Mod.getAOTSection()); unlikely(!Res)) {
      spdlog::error("    AOT section -- library load failed:{} , use "
                    "interpreter mode instead.",
                    Res.error());
    } else {
      if (loadExecutable(Mod, Exec)) {
        return {};
      }
    }
  }

  // Fallback to the interpreter mode case: Re-read the code section.
  WASMType = InputType::WASM;
  FMgr.seek(Mod.getCodeSection().getStartOffset());
  if (auto Res = loadSection(Mod.getCodeSection()); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  return {};
}

Expect<void> Loader::loadModuleAOT(AST::AOTSection &AOTSection) {
  // Find and Read the AOT custom section first. Jump the others.
  // This loop is for checking the input is an universal WASM or not.
  // Therefore, if the configure is set as force interpreter mode, skip this.
  while (WASMType != InputType::SharedLibrary) {
    // This loop only overview the custom sections and read the AOT section.
    // For the other general errors, break and handle in the sequentially
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
          AOTSection = std::move(NewAOTSection);
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
  return {};
}

} // namespace Loader
} // namespace WasmEdge
