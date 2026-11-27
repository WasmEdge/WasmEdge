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
  static LinkExpect<void> layout(LinkGraph &Graph) noexcept;
  static LinkExpect<void> write(const LinkGraph &Graph,
                                Writer &Output) noexcept;
};

/// Gives each ARM text section without an exception index entry an
/// EXIDX_CANTUNWIND entry, so that it does not inherit the rules of the
/// preceding function.
LinkExpect<void> addARMExidxBoundaries(LinkGraph &Graph);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
