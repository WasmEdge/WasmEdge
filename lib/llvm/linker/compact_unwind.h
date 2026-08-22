// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"

#include <cstdint>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

class LinkGraph;

Expect<void> compactUnwindToEHFrame(LinkGraph &Graph);
Expect<void> validateCompactUnwind(const LinkGraph &Graph);
Expect<uint64_t> machOUnwindInfoSize(const LinkGraph &Graph);
Expect<void> reserveMachOUnwindInfo(LinkGraph &Graph);
Expect<void> populateMachOUnwindInfo(LinkGraph &Graph, uint64_t ImageBase = 0);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
