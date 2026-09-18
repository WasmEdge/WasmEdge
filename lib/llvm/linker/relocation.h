// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/link_graph.h"

#include <cstdint>
#include <map>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

LinkExpect<void> applyRelocations(LinkGraph &Graph);

namespace Internal {

Expect<uint64_t> readUnsigned(Span<const Byte> Bytes, uint64_t Offset,
                              uint8_t Width, Endianness Endian) noexcept;
Expect<int64_t> readSigned(Span<const Byte> Bytes, uint64_t Offset,
                           uint8_t Width, Endianness Endian) noexcept;
Expect<void> writeUnsigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                           Endianness Endian, uint64_t Value) noexcept;
Expect<void> writeSigned(Span<Byte> Bytes, uint64_t Offset, uint8_t Width,
                         Endianness Endian, int64_t Value) noexcept;
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

LinkExpect<RelocationResult> applyX86_64(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyARM(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyAArch64(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyRISCV(const LinkGraph &Graph);
LinkExpect<RelocationResult> applyS390X(const LinkGraph &Graph);

} // namespace Internal
} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
