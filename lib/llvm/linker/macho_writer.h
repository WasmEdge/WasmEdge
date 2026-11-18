// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/link_graph.h"
#include "linker/writer.h"

#include <cstdint>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {
bool machOBuildVersionCommandSize(uint64_t ToolCount,
                                  uint32_t &Result) noexcept;
} // namespace Internal

class MachOWriter {
public:
  static Expect<void> layout(LinkGraph &Graph) noexcept;
  static LinkExpect<void> write(const LinkGraph &Graph,
                                Writer &Output) noexcept;
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
