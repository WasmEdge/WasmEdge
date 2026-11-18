// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/relocation.h"

#include <optional>
#include <string>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {

Unexpected<Diagnostic> relocationError(const LinkGraph &Graph,
                                       const Relocation &Rel,
                                       std::string_view Message,
                                       DiagnosticKind Kind) {
  Diagnostic Diag{std::string(Message)};
  Diag.Section = Rel.Section;
  Diag.Symbol = Rel.Symbol;
  Diag.RelocationType = Rel.Type;
  Diag.Offset = Rel.Offset;
  if (Rel.Section < Graph.sections().size())
    Diag.SectionName = Graph.sections()[Rel.Section].Name;
  if (Rel.Symbol < Graph.symbols().size())
    Diag.SymbolName = Graph.symbols()[Rel.Symbol].Name;
  Diag.Kind = Kind;
  return Unexpected<Diagnostic>(std::move(Diag));
}

LinkExpect<uint64_t> symbolAddress(const LinkGraph &Graph,
                                   const Relocation &Rel) {
  const auto Address = symbolAddress(Graph, Graph.symbols()[Rel.Symbol]);
  if (!Address)
    return relocationError(Graph, Rel, "symbol address overflows");
  return *Address;
}

LinkExpect<uint64_t> placeAddress(const LinkGraph &Graph,
                                  const Relocation &Rel) {
  uint64_t Address = 0;
  if (!addUnsigned(Graph.sections()[Rel.Section].Address, Rel.Offset, Address))
    return relocationError(Graph, Rel, "relocation place overflows");
  return Address;
}

RelocationWriter::RelocationWriter(const LinkGraph &Graph)
    : Graph(Graph), Result{{}, Graph.rebases()} {
  Result.Content.reserve(Graph.sections().size());
  for (const auto &Section : Graph.sections()) {
    Result.Content.push_back(Section.Content);
  }
}

LinkExpect<void> RelocationWriter::writeAbsolute(const Relocation &Rel,
                                                 uint8_t Width,
                                                 Endianness Endian,
                                                 uint64_t Value,
                                                 int64_t Addend) {
  if (!writeUnsigned(Result.Content[Rel.Section], Rel.Offset, Width, Endian,
                     Value)) {
    return relocationError(Graph, Rel, "absolute relocation overflows");
  }
  Result.Rebases.push_back(
      Rebase{Rel.Section, Rel.Offset, Rel.Type, Addend, Width, Rel.Format});
  return {};
}

std::optional<unsigned> aarch64LoadStoreScale(uint32_t Instruction) noexcept {
  constexpr uint32_t LoadStoreUnsignedImmediateMask = UINT32_C(0x3B000000);
  constexpr uint32_t LoadStoreUnsignedImmediate = UINT32_C(0x39000000);
  constexpr uint32_t VectorQuadWordBits = UINT32_C(0x04800000);
  constexpr unsigned SizeShift = 30;
  constexpr uint32_t SizeMask = UINT32_C(3);
  constexpr unsigned QuadWordScale = 4;
  if ((Instruction & LoadStoreUnsignedImmediateMask) !=
      LoadStoreUnsignedImmediate)
    return std::nullopt;
  const unsigned Size = (Instruction >> SizeShift) & SizeMask;
  if (Size == 0 && (Instruction & VectorQuadWordBits) == VectorQuadWordBits)
    return QuadWordScale;
  return Size;
}

} // namespace Internal

LinkExpect<void> applyRelocations(LinkGraph &Graph) {
  if (Graph.RelocationsApplied ||
      Graph.UnwindInfoState == MachOUnwindInfoState::Populated) {
    return Unexpected<Diagnostic>(
        Diagnostic{"link graph relocations already applied"});
  }
  if (auto Valid = Graph.validate(); !Valid)
    return Unexpected<Diagnostic>(std::move(Valid.error()));
  if (Graph.endianness() != expectedEndianness(Graph.target())) {
    return Unexpected<Diagnostic>(
        Diagnostic{"target input has wrong endianness"});
  }
  Diagnostic Unsupported{"unsupported relocation target"};
  Unsupported.Kind = DiagnosticKind::Unsupported;
  LinkExpect<Internal::RelocationResult> Result =
      Unexpected<Diagnostic>(std::move(Unsupported));
  switch (Graph.target()) {
  case Target::X86_64:
    Result = Internal::applyX86_64(Graph);
    break;
  case Target::ARM:
    Result = Internal::applyARM(Graph);
    break;
  case Target::AArch64:
    Result = Internal::applyAArch64(Graph);
    break;
  case Target::RISCV64:
    Result = Internal::applyRISCV(Graph);
    break;
  case Target::S390X:
    Result = Internal::applyS390X(Graph);
    break;
  }
  if (!Result)
    return Unexpected<Diagnostic>(std::move(Result.error()));
  if (Result->Content.size() != Graph.Sections.size())
    return Unexpected<Diagnostic>(
        Diagnostic{"relocation backend returned wrong section count"});
  EXPECTED_TRY(LinkGraph::checkRebaseOverlaps(Result->Rebases));
  for (size_t I = 0; I < Result->Content.size(); ++I) {
    Graph.Sections[I].Content.swap(Result->Content[I]);
  }
  Graph.Rebases.swap(Result->Rebases);
  Graph.RelocationsApplied = true;
  return {};
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
