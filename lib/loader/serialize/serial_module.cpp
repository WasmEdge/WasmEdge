// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  using SecVariant =
      std::variant<const AST::CustomSection *, const AST::TypeSection *,
                   const AST::ImportSection *, const AST::FunctionSection *,
                   const AST::TableSection *, const AST::MemorySection *,
                   const AST::GlobalSection *, const AST::ExportSection *,
                   const AST::StartSection *, const AST::ElementSection *,
                   const AST::CodeSection *, const AST::DataSection *,
                   const AST::DataCountSection *>;
  std::vector<SecVariant> Sections;
  Sections.reserve(Mod.getCustomSections().size() + 12);
  for (auto &CustomSec : Mod.getCustomSections()) {
    Sections.push_back(&CustomSec);
  }
  Sections.push_back(&Mod.getTypeSection());
  Sections.push_back(&Mod.getImportSection());
  Sections.push_back(&Mod.getFunctionSection());
  Sections.push_back(&Mod.getTableSection());
  Sections.push_back(&Mod.getMemorySection());
  Sections.push_back(&Mod.getGlobalSection());
  Sections.push_back(&Mod.getExportSection());
  Sections.push_back(&Mod.getStartSection());
  Sections.push_back(&Mod.getElementSection());
  Sections.push_back(&Mod.getCodeSection());
  Sections.push_back(&Mod.getDataSection());
  Sections.push_back(&Mod.getDataCountSection());
  std::sort(Sections.begin(), Sections.end(), [&](auto &A, auto &B) {
    auto Getter = [](auto &Sec) { return Sec->getStartOffset(); };
    return std::visit(Getter, A) < std::visit(Getter, B);
  });

  // Serialize sections.
  for (auto &Sec : Sections) {
    auto SerVisit = [&OutVec, this](auto &A) -> Expect<void> {
      return serializeSection(*A, OutVec);
    };
    if (auto Res = std::visit(SerVisit, Sec); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    }
  }
  return OutVec;
}

} // namespace Loader
} // namespace WasmEdge
