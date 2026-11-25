// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "aot/version.h"
#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "linker/link_graph.h"
#include "linker/writer.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

struct UniversalWasmChunk {
  uint64_t Offset;
  Span<const Byte> Bytes;
};

struct UniversalWasmSection {
  AOT::SectionKind Kind;
  uint64_t Address;
  uint64_t Size;
  std::vector<UniversalWasmChunk> Chunks;

  uint64_t contentSize() const noexcept {
    return Chunks.empty() ? 0
                          : Chunks.back().Offset + Chunks.back().Bytes.size();
  }
};

struct UniversalWasmTables {
  uint64_t Version = 0;
  uint64_t Intrinsics = 0;
  std::vector<uint64_t> Types;
  std::vector<uint64_t> Codes;
};

struct SemanticAddress {
  std::string_view Name;
  uint64_t Address;
};

class UniversalWasmWriter {
public:
  /// Builds the address tables from symbol names without the platform
  /// prefix. Names that the AOT loader does not read are ignored.
  static LinkExpect<UniversalWasmTables>
  tables(Span<const SemanticAddress> Symbols);
  static Expect<void> encode(Writer &Output, Span<const Byte> Wasm,
                             const UniversalWasmTables &Tables,
                             Span<const UniversalWasmSection> Sections);
  static LinkExpect<void> write(const LinkGraph &Graph, Span<const Byte> Wasm,
                                Writer &Output);
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
