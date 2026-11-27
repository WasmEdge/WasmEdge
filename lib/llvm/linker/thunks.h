// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "linker/link_graph.h"

namespace WasmEdge {
namespace LLVM {
namespace Linker {

inline constexpr uint64_t AArch64TextPieceSize = UINT64_C(64) << 20;

/// Split each AArch64 text section larger than \p PieceSize at function
/// starts, so that branch thunk islands can sit between the pieces. Branches
/// that the assembler resolved across a cut become relocations.
LinkExpect<bool> splitTextSections(LinkGraph &Graph, uint64_t PieceSize);
LinkExpect<bool> insertBranchThunks(LinkGraph &Graph);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
