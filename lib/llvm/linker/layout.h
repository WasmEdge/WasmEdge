// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/link_graph.h"

#include <cstdint>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

LinkExpect<void> layout(LinkGraph &Graph, uint64_t ImageBase = 0,
                        uint64_t SegmentAlignment = 1);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
