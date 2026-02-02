// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/aot_section.h"
#include "loader/loader.h"
#include "loader/shared_library.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Loader {

// Load binary to construct Module node. See "include/loader/loader.h".
Expect<void> Loader::loadModule(AST::Module &Mod,
                                std::optional<uint64_t> Bound) {
  uint64_t StartOffset = FMgr.getOffset();
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  };

  // Variables to record the loaded section types.
  HasDataSection = false;
  std::vector<uint8_t> Secs = {0x0BU, 0x0AU, 0x0CU, 0x09U, 0x08U, 0x07U, 0x06U,
                               0x0DU, 0x05U, 0x04U, 0x03U, 0x02U, 0x01U};
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

    // Sections except the custom section should be unique and in order.
    if (NewSectionId > 0x00U && NewSectionId < 0x0EU) {
      while (Secs.size() > 0 && Secs.back() != NewSectionId) {
        Secs.pop_back();
      }
      if (Secs.empty()) {
        return logLoadError(ErrCode::Value::JunkSection, FMgr.getLastOffset(),
                            ASTNodeAttr::Module);
      }
      Secs.pop_back();
    }

    switch (NewSectionId) {
    case 0x00:
      Mod.getCustomSections().emplace_back();
      EXPECTED_TRY(
          loadSection(Mod.getCustomSections().back()).map_error(ReportError));
      break;
    case 0x01:
      EXPECTED_TRY(loadSection(Mod.getTypeSection()).map_error(ReportError));
      break;
    case 0x02:
      EXPECTED_TRY(loadSection(Mod.getImportSection()).map_error(ReportError));
      break;
    case 0x03:
      EXPECTED_TRY(
          loadSection(Mod.getFunctionSection()).map_error(ReportError));
      break;
    case 0x04:
      EXPECTED_TRY(loadSection(Mod.getTableSection()).map_error(ReportError));
      break;
    case 0x05:
      EXPECTED_TRY(loadSection(Mod.getMemorySection()).map_error(ReportError));
      break;
    case 0x06:
      EXPECTED_TRY(loadSection(Mod.getGlobalSection()).map_error(ReportError));
      break;
    case 0x07:
      EXPECTED_TRY(loadSection(Mod.getExportSection()).map_error(ReportError));
      break;
    case 0x08:
      EXPECTED_TRY(loadSection(Mod.getStartSection()).map_error(ReportError));
      break;
    case 0x09:
      EXPECTED_TRY(loadSection(Mod.getElementSection()).map_error(ReportError));
      break;
    case 0x0A:
      EXPECTED_TRY(loadSection(Mod.getCodeSection()).map_error(ReportError));
      break;
    case 0x0B:
      EXPECTED_TRY(loadSection(Mod.getDataSection()).map_error(ReportError));
      break;
    case 0x0C:
      // This section is for BulkMemoryOperations or ReferenceTypes proposal.
      if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
          !Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(ErrCode::Value::MalformedSection,
                               Proposal::BulkMemoryOperations,
                               FMgr.getLastOffset(), ASTNodeAttr::Module);
      }
      EXPECTED_TRY(
          loadSection(Mod.getDataCountSection()).map_error(ReportError));
      HasDataSection = true;
      break;
    case 0x0D:
      // This section is for ExceptionHandling proposal.
      if (!Conf.hasProposal(Proposal::ExceptionHandling)) {
        return logNeedProposal(ErrCode::Value::MalformedSection,
                               Proposal::ExceptionHandling,
                               FMgr.getLastOffset(), ASTNodeAttr::Module);
      }
      EXPECTED_TRY(loadSection(Mod.getTagSection()).map_error(ReportError));
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

// Setup symbols from loaded binary. See "include/loader/loader.h".
Expect<void> Loader::loadExecutable(AST::Module &Mod,
                                    std::shared_ptr<Executable> Exec) {
  auto &SubTypes = Mod.getTypeSection().getContent();
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

  // Validate and set the symbols into the module.
  uint32_t FuncTypeIdx = 0;
  for (auto &SubType : SubTypes) {
    if (SubType.getCompositeType().isFunc()) {
      if (unlikely(!FuncTypeSymbols[FuncTypeIdx])) {
        spdlog::error(
            "    AOT section -- invalid type symbol at index {}, use "
            "interpreter mode instead.",
            FuncTypeIdx);
        return Unexpect(ErrCode::Value::IllegalGrammar);
      }
      SubType.getCompositeType().getFuncType().setSymbol(
          std::move(FuncTypeSymbols[FuncTypeIdx]));
    }
    FuncTypeIdx++;
  }
  for (size_t I = 0; I < CodeSegs.size(); ++I) {
    if (unlikely(!CodeSymbols[I])) {
      spdlog::error(
          "    AOT section -- invalid code symbol at index {}, use "
          "interpreter mode instead.",
          I);
      return Unexpect(ErrCode::Value::IllegalGrammar);
    }
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
  return loadSection(Mod.getCodeSection()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return E;
  });
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
