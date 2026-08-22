// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "linker/link_graph.h"

#include <llvm/BinaryFormat/Dwarf.h>

#include <cstdint>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace Internal {

// CIE/FDE records follow the Linux Standard Base .eh_frame extension to DWARF;
// pointer encoding values are defined by the DWARF EH encoding convention.
inline constexpr uint64_t EHFrameLengthFieldSize = 4;
inline constexpr uint64_t EHFrameRecordHeaderSize = 8;
inline constexpr uint8_t EHFramePointerWidth = 8;
inline constexpr uint8_t EHFrameCIEVersion = 1;
// "zR" gives each FDE augmentation data and declares its initial location
// encoding.
inline constexpr std::string_view EHFrameCIEAugmentation = "zR";
inline constexpr uint64_t EHFrameCIECodeAlignment = 1;
inline constexpr uint64_t EHFrameAArch64CIECodeAlignment = 4;
inline constexpr int64_t EHFrameCIEDataAlignment = -8;
inline constexpr uint64_t EHFrameCIEAugmentationLength = 1;
inline constexpr uint8_t EHFrameFDEPointerEncoding =
    llvm::dwarf::DW_EH_PE_absptr | llvm::dwarf::DW_EH_PE_pcrel;

std::optional<size_t> ehFrameTerminatorOffset(Span<const Byte> Bytes);
bool readULEB128(Span<const Byte> Bytes, size_t &Offset,
                 uint64_t &Value) noexcept;
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
