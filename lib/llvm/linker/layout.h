// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/link_graph.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {

struct Placement {
  uint64_t Address = 0;
  uint64_t FileOffset = 0;
};

std::vector<SectionId> sectionsOfKind(const LinkGraph &Graph, SectionKind Kind);
LinkExpect<void> applyPlacements(LinkGraph &Graph,
                                 Span<const Placement> Placements);

} // namespace Internal

LinkExpect<void> layout(LinkGraph &Graph, uint64_t ImageBase = 0,
                        uint64_t SegmentAlignment = 1);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
