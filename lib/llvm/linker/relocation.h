// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/byte_io.h"
#include "linker/link_graph.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

LinkExpect<void> applyRelocations(LinkGraph &Graph);

namespace Internal {

struct RelocationResult {
  std::vector<std::vector<Byte>> Content;
  std::vector<bool> Staged;
  std::vector<Rebase> Rebases;
};

Unexpected<Diagnostic>
relocationError(const LinkGraph &Graph, const Relocation &Rel,
                std::string_view Message,
                DiagnosticKind Kind = DiagnosticKind::Malformed);
LinkExpect<uint64_t> symbolAddress(const LinkGraph &Graph,
                                   const Relocation &Rel);
LinkExpect<uint64_t> placeAddress(const LinkGraph &Graph,
                                  const Relocation &Rel);

class RelocationWriter {
public:
  explicit RelocationWriter(const LinkGraph &Graph);
  std::vector<Byte> &content(SectionId Section) {
    if (!Result.Staged[Section]) {
      Result.Content[Section] = Graph.sections()[Section].Content;
      Result.Staged[Section] = true;
    }
    return Result.Content[Section];
  }
  LinkExpect<void> writeAbsolute(const Relocation &Rel, uint8_t Width,
                                 Endianness Endian, uint64_t Value,
                                 int64_t Addend);
  RelocationResult finish() && noexcept { return std::move(Result); }

private:
  const LinkGraph &Graph;
  RelocationResult Result;
};

std::optional<unsigned> aarch64LoadStoreScale(uint32_t Instruction) noexcept;
/// Byte displacement in the imm26 field of an AArch64 B or BL.
int64_t aarch64Branch26Displacement(uint32_t Instruction) noexcept;
/// Sign-extended immlo:immhi field of an AArch64 ADR or ADRP.
int64_t aarch64AdrImmediate(uint32_t Instruction) noexcept;
/// imm12 field of an AArch64 ADD or unsigned-offset load or store.
uint32_t aarch64Low12Immediate(uint32_t Instruction) noexcept;

LinkExpect<RelocationResult> applyX86_64(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyARM(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyAArch64(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyRISCV(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyS390X(const LinkGraph &Graph);

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
