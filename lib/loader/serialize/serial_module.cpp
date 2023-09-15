// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/serialize.h"

#include <functional>

namespace WasmEdge {
namespace Loader {

// Serialize module. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeModule(const AST::Module &Mod) const noexcept {
  std::vector<uint8_t> OutVec;
  OutVec.reserve(Mod.getMagic().size() + Mod.getVersion().size());
  // Serialize Magic and Version.
  OutVec.insert(OutVec.end(), Mod.getMagic().begin(), Mod.getMagic().end());
  OutVec.insert(OutVec.end(), Mod.getVersion().begin(), Mod.getVersion().end());

  // Sort sections according to start offset.
  using SerialWrap = Expect<std::vector<uint8_t>>(const AST::Section *);
  std::vector<std::pair<const AST::Section *, std::function<SerialWrap>>>
      Sections;
  Sections.reserve(Mod.getCustomSections().size() + 12);
  for (auto &CustomSec : Mod.getCustomSections()) {
    Sections.push_back(std::pair(&CustomSec, [this](const AST::Section *Sec) {
      return serializeSection(*static_cast<const AST::CustomSection *>(Sec));
    }));
  }
  if (Mod.getTypeSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getTypeSection(), [this](const AST::Section *Sec) {
          return serializeSection(*static_cast<const AST::TypeSection *>(Sec));
        }));
  }
  if (Mod.getImportSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getImportSection(), [this](const AST::Section *Sec) {
          return serializeSection(
              *static_cast<const AST::ImportSection *>(Sec));
        }));
  }
  if (Mod.getFunctionSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getFunctionSection(), [this](const AST::Section *Sec) {
          return serializeSection(
              *static_cast<const AST::FunctionSection *>(Sec));
        }));
  }
  if (Mod.getTableSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getTableSection(), [this](const AST::Section *Sec) {
          return serializeSection(*static_cast<const AST::TableSection *>(Sec));
        }));
  }
  if (Mod.getMemorySection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getMemorySection(), [this](const AST::Section *Sec) {
          return serializeSection(
              *static_cast<const AST::MemorySection *>(Sec));
        }));
  }
  if (Mod.getGlobalSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getGlobalSection(), [this](const AST::Section *Sec) {
          return serializeSection(
              *static_cast<const AST::GlobalSection *>(Sec));
        }));
  }
  if (Mod.getExportSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getExportSection(), [this](const AST::Section *Sec) {
          return serializeSection(
              *static_cast<const AST::ExportSection *>(Sec));
        }));
  }
  if (Mod.getStartSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getStartSection(), [this](const AST::Section *Sec) {
          return serializeSection(*static_cast<const AST::StartSection *>(Sec));
        }));
  }
  if (Mod.getElementSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getElementSection(), [this](const AST::Section *Sec) {
          return serializeSection(
              *static_cast<const AST::ElementSection *>(Sec));
        }));
  }
  if (Mod.getCodeSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getCodeSection(), [this](const AST::Section *Sec) {
          return serializeSection(*static_cast<const AST::CodeSection *>(Sec));
        }));
  }
  if (Mod.getDataSection().getContentSize() > 0) {
    Sections.push_back(
        std::pair(&Mod.getDataSection(), [this](const AST::Section *Sec) {
          return serializeSection(*static_cast<const AST::DataSection *>(Sec));
        }));
  }
  if (Mod.getDataCountSection().getContentSize() > 0) {
    // This section is for BulkMemoryOperations or ReferenceTypes proposal.
    if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
        !Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::MalformedSection,
                             Proposal::BulkMemoryOperations,
                             ASTNodeAttr::Module);
    }
    Sections.push_back(
        std::pair(&Mod.getDataCountSection(), [this](const AST::Section *Sec) {
          return serializeSection(
              *static_cast<const AST::DataCountSection *>(Sec));
        }));
  }
  std::sort(Sections.begin(), Sections.end(), [](auto &A, auto &B) {
    return (A.first)->getStartOffset() < (B.first)->getStartOffset();
  });

  // Serialize sections.
  for (auto &Sec : Sections) {
    if (auto Res = Sec.second(Sec.first)) {
      OutVec.insert(OutVec.end(), Res->begin(), Res->end());
    } else {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    }
  }
  return OutVec;
}

} // namespace Loader
} // namespace WasmEdge
