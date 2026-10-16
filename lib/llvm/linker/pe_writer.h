// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/link_graph.h"
#include "linker/writer.h"

#include <string_view>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

inline constexpr uint64_t PEImageBase = UINT64_C(0x180000000);

LinkExpect<std::vector<Byte>>
normalizePERuntimeFunctions(const LinkGraph &Graph,
                            uint64_t RuntimeImageBase = PEImageBase);

class PEWriter {
public:
  static Expect<void> layout(LinkGraph &Graph) noexcept;
  static LinkExpect<void> write(const LinkGraph &Graph,
                                std::string_view DLLName,
                                Writer &Output) noexcept;
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
