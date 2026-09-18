// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "linker/link_graph.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace LLVM {
namespace Linker {

enum class ObjectReaderInputPolicy : uint8_t {
  Strict,
  AllowUnreferencedMSVCFltused,
};

namespace Internal {

ObjectReaderInputPolicy nativeObjectInputPolicy(ObjectFormat Format) noexcept;
uint64_t normalizeSectionAlignment(uint64_t Alignment) noexcept;
std::optional<std::map<std::string, std::string>>
parseCOFFExports(std::string_view Directives);
bool supportsMachORelocationMetadata(Target TargetValue,
                                     bool Scattered) noexcept;
LinkExpect<MachOBuildVersion> parseMachOBuildVersion(Span<const Byte> Command,
                                                     Endianness Endian);

struct CompactUnwindRelocation {
  uint64_t Offset;
  uint32_t Type;
  uint8_t PatchSize;
  bool PCRelative;
  bool External;
  bool Scattered;
};

struct DecodedCompactUnwindRecord {
  uint64_t Function;
  uint32_t Length;
  uint32_t Encoding;
  uint64_t Personality;
  uint64_t LSDA;
};

LinkExpect<std::vector<DecodedCompactUnwindRecord>>
parseCompactUnwindSection(Target TargetValue, Span<const Byte> Content,
                          Span<const CompactUnwindRelocation> Relocations);
LinkExpect<uint64_t> resolveCompactUnwindTargetOffset(bool External,
                                                      uint64_t SectionAddress,
                                                      uint64_t SymbolOffset,
                                                      uint64_t RawAddend);

} // namespace Internal

class ObjectReader {
public:
  static LinkExpect<LinkGraph>
  read(Span<const Byte> Buffer, Target ExpectedTarget,
       ObjectReaderInputPolicy InputPolicy = ObjectReaderInputPolicy::Strict);
};

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
