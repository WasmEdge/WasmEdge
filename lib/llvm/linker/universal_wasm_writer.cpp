// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/universal_wasm_writer.h"

#include "aot/version.h"
#include "common/defines.h"
#include "common/spdlog.h"
#include "linker/architecture.h"
#include "linker/pe_writer.h"
#include "linker/writer.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <optional>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

bool fitsSize(uint64_t Value) noexcept {
#if SIZE_MAX < UINT64_MAX
  return Value <= SIZE_MAX;
#else
  (void)Value;
  return true;
#endif
}

std::optional<uint64_t> add(uint64_t Left, uint64_t Right) noexcept {
  if (Right > std::numeric_limits<uint64_t>::max() - Left)
    return std::nullopt;
  return Left + Right;
}

std::optional<uint64_t> subtract(uint64_t Left, uint64_t Right) noexcept {
  if (Left < Right)
    return std::nullopt;
  return Left - Right;
}

bool overlaps(uint64_t LeftAddress, uint64_t LeftSize, uint64_t RightAddress,
              uint64_t RightSize) noexcept {
  if (LeftSize == 0 || RightSize == 0)
    return false;
  return LeftAddress <= RightAddress ? RightAddress - LeftAddress < LeftSize
                                     : LeftAddress - RightAddress < RightSize;
}

enum class AOTSectionKind : uint8_t {
  Text = 1,
  Data = 2,
  BSS = 3,
  Unwind = 4,
};

enum class AOTOSType : uint8_t { Linux = 1, MacOS = 2, Windows = 3 };
constexpr uint8_t CustomSectionId = 0;

Expect<void> writeError() noexcept {
  return Unexpect(ErrCode::Value::IllegalPath);
}

std::optional<uint8_t> hostOS() noexcept {
#if WASMEDGE_OS_LINUX
  return static_cast<uint8_t>(AOTOSType::Linux);
#elif WASMEDGE_OS_MACOS
  return static_cast<uint8_t>(AOTOSType::MacOS);
#elif WASMEDGE_OS_WINDOWS
  return static_cast<uint8_t>(AOTOSType::Windows);
#else
  return std::nullopt;
#endif
}

std::optional<AOTSectionKind> sectionKind(ObjectFormat Format,
                                          const Section &Value) noexcept {
  if (Value.Purpose == SectionPurpose::PData)
    return AOTSectionKind::Unwind;
  if (Value.Purpose == SectionPurpose::EHFrame)
    return Format == ObjectFormat::COFF ? AOTSectionKind::Data
                                        : AOTSectionKind::Unwind;
  if (Value.Purpose == SectionPurpose::ARMExidx)
    return AOTSectionKind::Data;
  switch (Value.Kind) {
  case SectionKind::Text:
    return AOTSectionKind::Text;
  case SectionKind::ReadOnly:
  case SectionKind::Data:
    return AOTSectionKind::Data;
  case SectionKind::BSS:
    return AOTSectionKind::BSS;
  case SectionKind::Unwind:
    return AOTSectionKind::Data;
  }
  return std::nullopt;
}

struct OutputSection {
  AOTSectionKind Kind;
  uint64_t Address;
  uint64_t Size;
  std::vector<Byte> Content;
};

uint64_t runtimeImageBase(const LinkGraph &Graph) noexcept {
  if (Graph.format() != ObjectFormat::COFF)
    return 0;
  return std::any_of(
             Graph.sections().begin(), Graph.sections().end(),
             [](const auto &Value) { return Value.Address >= PEImageBase; })
             ? PEImageBase
             : 0;
}

Expect<std::vector<OutputSection>> outputSections(const LinkGraph &Graph,
                                                  uint64_t ImageBase) {
  std::vector<const Section *> Ordered;
  for (const auto &Value : Graph.sections()) {
    if (Value.VirtualSize != 0 &&
        Value.Purpose != SectionPurpose::CompactUnwind &&
        Value.Purpose != SectionPurpose::PData) {
      Ordered.push_back(&Value);
    }
  }
  std::sort(Ordered.begin(), Ordered.end(),
            [&Graph](const auto *Left, const auto *Right) {
              return std::tuple(sectionKind(Graph.format(), *Left),
                                Left->Address, Left->Name) <
                     std::tuple(sectionKind(Graph.format(), *Right),
                                Right->Address, Right->Name);
            });
  std::vector<OutputSection> Result;
  for (const auto *Value : Ordered) {
    const auto Kind = sectionKind(Graph.format(), *Value);
    if (!Kind)
      return Unexpect(ErrCode::Value::IllegalPath);
    if (Value->Content.size() > Value->VirtualSize)
      return Unexpect(ErrCode::Value::IllegalPath);
    const auto Address = subtract(Value->Address, ImageBase);
    const auto End = add(Value->Address, Value->VirtualSize);
    if (!Address || !End)
      return Unexpect(ErrCode::Value::IllegalPath);
    const auto RelativeEnd = subtract(*End, ImageBase);
    if (!RelativeEnd)
      return Unexpect(ErrCode::Value::IllegalPath);
    if (Result.empty() || Result.back().Kind != *Kind) {
      Result.push_back(
          OutputSection{*Kind, *Address, Value->VirtualSize, Value->Content});
      continue;
    }
    auto &Output = Result.back();
    if (*Address < Output.Address || Output.Size > *Address - Output.Address) {
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    const uint64_t Gap = *Address - Output.Address;
    if (*RelativeEnd < Output.Address)
      return Unexpect(ErrCode::Value::IllegalPath);
    const uint64_t Size = *RelativeEnd - Output.Address;
    if (!fitsSize(Gap) || !fitsSize(Size)) {
      return Unexpect(ErrCode::Value::IllegalPath);
    }
    if (Output.Kind != AOTSectionKind::BSS) {
      if (Gap > Output.Content.max_size() ||
          Value->Content.size() >
              Output.Content.max_size() - static_cast<size_t>(Gap))
        return Unexpect(ErrCode::Value::IllegalPath);
      Output.Content.resize(static_cast<size_t>(Gap));
      Output.Content.insert(Output.Content.end(), Value->Content.begin(),
                            Value->Content.end());
    }
    Output.Size = Size;
  }
  const auto PDataSection = std::find_if(
      Graph.sections().begin(), Graph.sections().end(),
      [](const auto &Value) { return Value.Purpose == SectionPurpose::PData; });
  if (PDataSection != Graph.sections().end()) {
    if (Graph.format() != ObjectFormat::COFF)
      return Unexpect(ErrCode::Value::IllegalPath);
    uint64_t Address = UINT64_MAX;
    uint64_t ReservedEnd = 0;
    for (const auto &Value : Graph.sections()) {
      if (Value.Purpose != SectionPurpose::PData)
        continue;
      const auto RelativeAddress = subtract(Value.Address, ImageBase);
      const auto AbsoluteEnd = add(Value.Address, Value.VirtualSize);
      if (!RelativeAddress || !AbsoluteEnd)
        return Unexpect(ErrCode::Value::IllegalPath);
      const auto RelativeEnd = subtract(*AbsoluteEnd, ImageBase);
      if (!RelativeEnd)
        return Unexpect(ErrCode::Value::IllegalPath);
      Address = std::min(Address, *RelativeAddress);
      ReservedEnd = std::max(ReservedEnd, *RelativeEnd);
    }
    auto PData = normalizePERuntimeFunctions(Graph, ImageBase);
    if (!PData)
      return Unexpect(ErrCode::Value::IllegalPath);
    const auto End = add(Address, static_cast<uint64_t>(PData->size()));
    if (!End || *End > ReservedEnd)
      return Unexpect(ErrCode::Value::IllegalPath);
    if (std::any_of(Result.begin(), Result.end(), [&](const auto &Value) {
          return overlaps(Address, PData->size(), Value.Address, Value.Size);
        }))
      return Unexpect(ErrCode::Value::IllegalPath);
    Result.push_back(OutputSection{AOTSectionKind::Unwind, Address,
                                   PData->size(), std::move(*PData)});
    std::sort(Result.begin(), Result.end(),
              [](const auto &Left, const auto &Right) {
                return std::tie(Left.Kind, Left.Address) <
                       std::tie(Right.Kind, Right.Address);
              });
  }
  return Result;
}

std::optional<uint64_t> symbolAddress(const LinkGraph &Graph,
                                      const Symbol &Value,
                                      uint64_t ImageBase) noexcept {
  if (Value.Section >= Graph.sections().size()) {
    return std::nullopt;
  }
  const auto Base = Graph.sections()[Value.Section].Address;
  const auto Address = add(Base, Value.Offset);
  if (!Address || *Address < ImageBase)
    return std::nullopt;
  return (*Address - ImageBase) | static_cast<uint64_t>(Value.Thumb);
}

bool indexedSymbol(std::string_view Name, char Prefix,
                   uint64_t &Index) noexcept {
  if (Name.size() < 2 || Name.front() != Prefix) {
    return false;
  }
  const auto Result =
      std::from_chars(Name.data() + 1, Name.data() + Name.size(), Index);
  return Result.ec == std::errc{} && Result.ptr == Name.data() + Name.size();
}

std::optional<std::string_view> semanticName(const LinkGraph &Graph,
                                             const Symbol &Value) {
  if (!Value.Exported && !Value.Global)
    return std::nullopt;
  std::string_view Name = Value.ExportName ? *Value.ExportName : Value.Name;
  if (Graph.format() == ObjectFormat::MachO && !Name.empty() && Name[0] == '_')
    Name.remove_prefix(1);
  return Name;
}

Expect<void> writeAddresses(Writer &Output, const LinkGraph &Graph,
                            uint64_t ImageBase) {
  uint64_t Version = 0;
  uint64_t Intrinsics = 0;
  std::vector<uint64_t> Types;
  std::vector<uint64_t> Codes;
  std::vector<bool> TypePresent;
  std::vector<bool> CodePresent;
  uint64_t FirstCode = std::numeric_limits<uint64_t>::max();
  bool HasVersion = false;
  bool HasIntrinsics = false;
  for (const auto &SymbolValue : Graph.symbols()) {
    const auto Address = symbolAddress(Graph, SymbolValue, ImageBase);
    if (!Address) {
      return writeError();
    }
    const auto Name = semanticName(Graph, SymbolValue);
    if (!Name)
      continue;
    if (*Name == "version") {
      if (HasVersion)
        return writeError();
      HasVersion = true;
      Version = *Address;
    } else if (*Name == "intrinsics") {
      if (HasIntrinsics)
        return writeError();
      HasIntrinsics = true;
      Intrinsics = *Address;
    } else {
      uint64_t Index = 0;
      if (indexedSymbol(*Name, 't', Index) &&
          std::to_string(Index) == Name->substr(1)) {
        if (Index == std::numeric_limits<uint64_t>::max()) {
          return writeError();
        }
        const uint64_t NextSize = Index + 1;
        if (!fitsSize(NextSize) || NextSize > Types.max_size() ||
            NextSize > TypePresent.max_size())
          return writeError();
        if (TypePresent.size() > Index && TypePresent[Index])
          return writeError();
        Types.resize(std::max(Types.size(), static_cast<size_t>(NextSize)));
        TypePresent.resize(Types.size());
        Types[Index] = *Address;
        TypePresent[Index] = true;
      } else if (indexedSymbol(*Name, 'f', Index) &&
                 std::to_string(Index) == Name->substr(1)) {
        if (Index == std::numeric_limits<uint64_t>::max()) {
          return writeError();
        }
        const uint64_t NextSize = Index + 1;
        if (!fitsSize(NextSize) || NextSize > Codes.max_size() ||
            NextSize > CodePresent.max_size())
          return writeError();
        if (CodePresent.size() > Index && CodePresent[Index])
          return writeError();
        Codes.resize(std::max(Codes.size(), static_cast<size_t>(NextSize)));
        CodePresent.resize(Codes.size());
        Codes[Index] = *Address;
        CodePresent[Index] = true;
        FirstCode = std::min(FirstCode, Index);
      }
    }
  }
  if (!HasVersion || !HasIntrinsics ||
      std::find(TypePresent.begin(), TypePresent.end(), false) !=
          TypePresent.end()) {
    spdlog::error(
        "universal writer: invalid semantic symbols version={} intrinsics={} types={} codes={}"sv,
        HasVersion, HasIntrinsics, Types.size(), Codes.size());
    return writeError();
  }
  if (FirstCode != std::numeric_limits<uint64_t>::max()) {
    Codes.erase(
        Codes.begin(),
        Codes.begin() +
            static_cast<std::vector<uint64_t>::difference_type>(FirstCode));
    CodePresent.erase(
        CodePresent.begin(),
        CodePresent.begin() +
            static_cast<std::vector<bool>::difference_type>(FirstCode));
  }
  if (std::find(CodePresent.begin(), CodePresent.end(), false) !=
      CodePresent.end())
    return writeError();
  EXPECTED_TRY(Output.writeU64(Version));
  EXPECTED_TRY(Output.writeU64(Intrinsics));
  EXPECTED_TRY(Output.writeU64(Types.size()));
  for (const auto Address : Types) {
    EXPECTED_TRY(Output.writeU64(Address));
  }
  EXPECTED_TRY(Output.writeU64(Codes.size()));
  for (const auto Address : Codes) {
    EXPECTED_TRY(Output.writeU64(Address));
  }
  return {};
}

std::string_view objectFormatName(ObjectFormat Format) noexcept {
  switch (Format) {
  case ObjectFormat::ELF:
    return "ELF"sv;
  case ObjectFormat::MachO:
    return "MachO"sv;
  case ObjectFormat::COFF:
    return "COFF"sv;
  }
  return "unknown"sv;
}

void logRebase(const LinkGraph &Graph, const Rebase &Value) noexcept {
  if (Value.Section < Graph.sections().size()) {
    const auto &Section = Graph.sections()[Value.Section];
    spdlog::error(
        "universal writer: rebase unsupported section='{}' id={} offset={} width={} format={} type={}"sv,
        Section.Name, Value.Section, Value.Offset,
        static_cast<unsigned>(Value.Width), objectFormatName(Value.Format),
        Value.Type);
    return;
  }
  spdlog::error(
      "universal writer: rebase unsupported section='<invalid>' id={} offset={} width={} format={} type={}"sv,
      Value.Section, Value.Offset, static_cast<unsigned>(Value.Width),
      objectFormatName(Value.Format), Value.Type);
}

} // namespace

Expect<void>
UniversalWasmWriter::write(const LinkGraph &Graph, Span<const Byte> Wasm,
                           const std::filesystem::path &Output) noexcept {
  try {
    std::vector<Byte> Bytes;
    Writer Buffer(Bytes);
    EXPECTED_TRY(write(Graph, Wasm, Buffer));
    EXPECTED_TRY(auto Result, Writer::open(Output));
    EXPECTED_TRY(Result.write(Bytes));
    return Result.close();
  } catch (...) {
    return writeError();
  }
}

Expect<void> UniversalWasmWriter::write(const LinkGraph &Graph,
                                        Span<const Byte> Wasm, Writer &Result) {
  constexpr std::array<Byte, 8> WasmHeader{0x00, 0x61, 0x73, 0x6D,
                                           0x01, 0x00, 0x00, 0x00};
  if (Wasm.size() < WasmHeader.size() ||
      !std::equal(WasmHeader.begin(), WasmHeader.end(), Wasm.begin()))
    return writeError();
  size_t Offset = WasmHeader.size();
  while (Offset < Wasm.size()) {
    const Byte SectionId = Wasm[Offset++];
    constexpr Byte LastCoreSectionId = 13;
    if (SectionId > LastCoreSectionId)
      return writeError();
    uint32_t Size = 0;
    unsigned Shift = 0;
    Byte Encoded = 0;
    do {
      if (Offset >= Wasm.size() || Shift >= 35)
        return writeError();
      Encoded = Wasm[Offset++];
      if (Shift == 28 && (Encoded & 0xF0) != 0)
        return writeError();
      Size |= static_cast<uint32_t>(Encoded & 0x7F) << Shift;
      Shift += 7;
    } while ((Encoded & 0x80) != 0);
    if (Offset > Wasm.size() || Size > Wasm.size() - Offset)
      return writeError();
    Offset += Size;
  }
  const auto OS = hostOS();
  const auto Arch = Internal::architecture(Graph.target());
  if (!OS || !Arch || *Arch != AOT::kHostArchitecture ||
      !Graph.relocationsApplied()) {
    return writeError();
  }
  if (!Graph.rebases().empty()) {
    logRebase(Graph, Graph.rebases().front());
    return writeError();
  }
  const uint64_t ImageBase = runtimeImageBase(Graph);

  std::vector<Byte> Payload;
  Writer Section(Payload);
  EXPECTED_TRY(Section.writeName("wasmedge"sv));
  EXPECTED_TRY(Section.writeU32(AOT::kBinaryVersion));
  EXPECTED_TRY(Section.writeByte(*OS));
  const auto HostArchitecture = static_cast<uint8_t>(AOT::kHostArchitecture);
  EXPECTED_TRY(Section.writeByte(HostArchitecture));
  EXPECTED_TRY(writeAddresses(Section, Graph, ImageBase));
  EXPECTED_TRY(auto Sections, outputSections(Graph, ImageBase));
  if (Sections.size() > std::numeric_limits<uint32_t>::max()) {
    return writeError();
  }
  EXPECTED_TRY(Section.writeU32(static_cast<uint32_t>(Sections.size())));
  for (const auto &Value : Sections) {
    EXPECTED_TRY(Section.writeByte(static_cast<uint8_t>(Value.Kind)));
    EXPECTED_TRY(Section.writeU64(Value.Address));
    EXPECTED_TRY(Section.writeU64(Value.Size));
    EXPECTED_TRY(Section.writeName(
        std::string_view(reinterpret_cast<const char *>(Value.Content.data()),
                         Value.Content.size())));
  }
  EXPECTED_TRY(Section.close());
  if (Payload.size() > UINT32_MAX) {
    return writeError();
  }

  EXPECTED_TRY(Result.write(Wasm));
  EXPECTED_TRY(Result.writeByte(CustomSectionId));
  EXPECTED_TRY(Result.writeU32(static_cast<uint32_t>(Payload.size())));
  EXPECTED_TRY(Result.write(Payload));
  return Result.close();
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
