// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/byte_io.h"
#include "linker/link_graph.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

LinkExpect<void> applyRelocations(LinkGraph &Graph);

namespace Internal {

class RebaseIntervalIndex {
public:
  explicit RebaseIntervalIndex(Span<const Rebase> Rebases);
  bool insert(SectionId Section, uint64_t Offset, uint8_t Width);

private:
  std::map<std::pair<SectionId, uint64_t>, uint8_t> Intervals;
};

struct RelocationResult {
  std::vector<std::vector<Byte>> Content;
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
  std::vector<Byte> &content(SectionId Section) noexcept {
    return Result.Content[Section];
  }
  LinkExpect<void> writeAbsolute(const Relocation &Rel, uint8_t Width,
                                 Endianness Endian, uint64_t Value,
                                 int64_t Addend);
  RelocationResult finish() && noexcept { return std::move(Result); }

private:
  const LinkGraph &Graph;
  RelocationResult Result;
  RebaseIntervalIndex RebaseIntervals;
};

std::optional<unsigned> aarch64LoadStoreScale(uint32_t Instruction) noexcept;

LinkExpect<RelocationResult> applyX86_64(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyARM(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyAArch64(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyRISCV(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyS390X(const LinkGraph &Graph);

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
