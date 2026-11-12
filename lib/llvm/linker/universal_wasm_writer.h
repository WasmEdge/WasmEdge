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
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

struct UniversalWasmSection {
  AOT::SectionKind Kind;
  uint64_t Address;
  uint64_t Size;
  std::vector<Byte> Content;
};

struct UniversalWasmTables {
  uint64_t Version = 0;
  uint64_t Intrinsics = 0;
  std::vector<uint64_t> Types;
  std::vector<uint64_t> Codes;
};

class UniversalWasmWriter {
public:
  static Expect<void> encode(Writer &Output, Span<const Byte> Wasm,
                             const UniversalWasmTables &Tables,
                             Span<const UniversalWasmSection> Sections);
  static LinkExpect<void> write(const LinkGraph &Graph, Span<const Byte> Wasm,
                                Writer &Output);
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
