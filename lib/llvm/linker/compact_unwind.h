// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "linker/link_graph.h"

#include <cstdint>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {

// Compact unwind encodings follow Apple's compact_unwind_encoding.h.
inline constexpr uint32_t CompactUnwindModeMask = 0x0F000000;
inline constexpr uint32_t CompactUnwindArm64DwarfMode = 0x03000000;
inline constexpr uint32_t CompactUnwindX86_64DwarfMode = 0x04000000;
inline constexpr uint32_t CompactUnwindDwarfOffsetMask = 0x00FFFFFF;

inline constexpr uint32_t compactUnwindDwarfMode(Target Value) noexcept {
  return Value == Target::AArch64 ? CompactUnwindArm64DwarfMode
                                  : CompactUnwindX86_64DwarfMode;
}

inline constexpr bool isDwarfCompactUnwind(Target Value,
                                           uint32_t Encoding) noexcept {
  const uint32_t Mode = Encoding & CompactUnwindModeMask;
  return (Value == Target::AArch64 && Mode == CompactUnwindArm64DwarfMode) ||
         (Value == Target::X86_64 && Mode == CompactUnwindX86_64DwarfMode);
}

} // namespace Internal

Expect<void> compactUnwindToEHFrame(LinkGraph &Graph);
Expect<void> validateCompactUnwind(const LinkGraph &Graph);
Expect<uint64_t> machOUnwindInfoSize(const LinkGraph &Graph);
Expect<void> reserveMachOUnwindInfo(LinkGraph &Graph);
Expect<void> populateMachOUnwindInfo(LinkGraph &Graph, uint64_t ImageBase = 0);

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
