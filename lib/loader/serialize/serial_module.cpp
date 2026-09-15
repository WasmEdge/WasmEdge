// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/serialize.h"

#include <algorithm>
#include <variant>

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

  using SecVariant =
      std::variant<const AST::TypeSection *, const AST::ImportSection *,
                   const AST::FunctionSection *, const AST::TableSection *,
                   const AST::MemorySection *, const AST::TagSection *,
                   const AST::GlobalSection *, const AST::ExportSection *,
                   const AST::StartSection *, const AST::ElementSection *,
                   const AST::DataCountSection *, const AST::CodeSection *,
                   const AST::DataSection *>;

  // The known sections, in the order the binary format requires, which is the
  // order Loader::loadModule enforces.
  const SecVariant Ordered[] = {
      &Mod.getTypeSection(),      &Mod.getImportSection(),
      &Mod.getFunctionSection(),  &Mod.getTableSection(),
      &Mod.getMemorySection(),    &Mod.getTagSection(),
      &Mod.getGlobalSection(),    &Mod.getExportSection(),
      &Mod.getStartSection(),     &Mod.getElementSection(),
      &Mod.getDataCountSection(), &Mod.getCodeSection(),
      &Mod.getDataSection()};

  // Custom sections may appear anywhere, so they are merged into that skeleton
  // by offset, which keeps their position for a module read by the loader.
  std::vector<const AST::CustomSection *> Customs;
  Customs.reserve(Mod.getCustomSections().size());
  for (const auto &Custom : Mod.getCustomSections()) {
    Customs.push_back(&Custom);
  }
  std::stable_sort(Customs.begin(), Customs.end(),
                   [](const auto *A, const auto *B) {
                     return A->getStartOffset() < B->getStartOffset();
                   });

  auto It = Customs.begin();
  const auto End = Customs.end();
  for (const auto &Sec : Ordered) {
    const uint64_t Offset =
        std::visit([](const auto *S) { return S->getStartOffset(); }, Sec);
    for (; It != End && (*It)->getStartOffset() < Offset; ++It) {
      serializeSection(**It, OutVec);
    }
    std::visit([&](const auto *S) { serializeSection(*S, OutVec); }, Sec);
  }
  for (; It != End; ++It) {
    serializeSection(**It, OutVec);
  }

  return OutVec;
}

} // namespace Loader
} // namespace WasmEdge
