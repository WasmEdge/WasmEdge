// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "linker/link_graph.h"

#include <cstdint>
#include <set>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {

Expect<int64_t> decodeSLEB128(Span<const Byte> Bytes);
Expect<uint64_t> resolveMachOFDEAddress(uint64_t LoadBase,
                                        uint64_t SectionAddress, uint64_t Field,
                                        int64_t Delta);

} // namespace Internal

Expect<void> normalizeMachOEHFrame(LinkGraph &Graph);
Expect<std::set<size_t>> machOEHFrameFields(Span<const Byte> Bytes,
                                            Target Architecture);
Expect<void> validateMachOEHFrameCoverage(const LinkGraph &Graph);
Expect<std::vector<uint64_t>> machOEHFrameStarts(const LinkGraph &Graph,
                                                 uint64_t LoadBase);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
