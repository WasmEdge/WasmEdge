// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/link_graph.h"
#include "linker/writer.h"

namespace WasmEdge {
namespace LLVM {
namespace Linker {

class ELFWriter {
public:
  static Expect<void> layout(LinkGraph &Graph) noexcept;
  static LinkExpect<void> write(const LinkGraph &Graph,
                                Writer &Output) noexcept;
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
