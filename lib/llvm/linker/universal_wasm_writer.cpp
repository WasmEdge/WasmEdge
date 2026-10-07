// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "linker/universal_wasm_writer.h"

#include "aot/version.h"
#include "linker/architecture.h"
#include "linker/byte_io.h"
#include "linker/pe_writer.h"
#include "linker/writer.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace LLVM {
namespace Linker {

namespace {

using Internal::fitsSizeT;

constexpr uint8_t CustomSectionId = 0;
constexpr Byte LastCoreSectionId = 13;
constexpr unsigned ULEB32ShiftLimit = 35;
constexpr unsigned ULEB32FinalShift = 28;
constexpr Byte ULEB32FinalUnusedBits = 0xF0;

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

Unexpected<Diagnostic>
writeError(std::string_view Reason,
           DiagnosticKind Kind = DiagnosticKind::Malformed) {
  return diagnosticError("universal writer: " + std::string(Reason), Kind);
}

LinkExpect<void> encoded(Expect<void> Result, std::string_view Action) {
  if (!Result)
    return writeError("failed to " + std::string(Action), DiagnosticKind::IO);
  return {};
}

std::optional<AOT::SectionKind> sectionKind(ObjectFormat Format,
                                            const Section &Value) noexcept {
  if (Value.Purpose == SectionPurpose::PData)
    return AOT::SectionKind::Unwind;
  if (Value.Purpose == SectionPurpose::EHFrame)
    return Format == ObjectFormat::COFF ? AOT::SectionKind::Data
                                        : AOT::SectionKind::Unwind;
  if (Value.Purpose == SectionPurpose::ARMExidx)
    return AOT::SectionKind::Data;
  switch (Value.Kind) {
  case SectionKind::Text:
    return AOT::SectionKind::Text;
  case SectionKind::ReadOnly:
  case SectionKind::Data:
  case SectionKind::Unwind:
    return AOT::SectionKind::Data;
  case SectionKind::BSS:
    return AOT::SectionKind::BSS;
  }
  return std::nullopt;
}

struct OutputSection {
  AOT::SectionKind Kind;
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

struct PDataReservation {
  std::vector<std::pair<uint64_t, uint64_t>> Ranges;
  uint64_t Address = UINT64_MAX;
  uint64_t End = 0;
  bool Present = false;
};

LinkExpect<PDataReservation> reservePData(const LinkGraph &Graph,
                                          uint64_t ImageBase) {
  PDataReservation Result;
  for (const auto &Value : Graph.sections()) {
    if (Value.Purpose != SectionPurpose::PData)
      continue;
    const auto RelativeAddress = subtract(Value.Address, ImageBase);
    const auto AbsoluteEnd = add(Value.Address, Value.VirtualSize);
    if (!RelativeAddress || !AbsoluteEnd)
      return writeError("PE runtime function table address overflow");
    const auto RelativeEnd = subtract(*AbsoluteEnd, ImageBase);
    if (!RelativeEnd)
      return writeError(
          "PE runtime function table end precedes the image base");
    if (Value.VirtualSize != 0)
      Result.Ranges.emplace_back(*RelativeAddress, Value.VirtualSize);
    Result.Address = std::min(Result.Address, *RelativeAddress);
    Result.End = std::max(Result.End, *RelativeEnd);
    Result.Present = true;
  }
  return Result;
}

LinkExpect<std::vector<OutputSection>> outputSections(const LinkGraph &Graph,
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
  EXPECTED_TRY(auto Reserved, reservePData(Graph, ImageBase));
  const auto CrossesReserved = [&Reserved](const OutputSection &Output,
                                           uint64_t Address,
                                           uint64_t RelativeEnd) {
    if (Address < Output.Address || RelativeEnd < Output.Address)
      return false;
    const uint64_t Extent = RelativeEnd - Output.Address;
    return std::any_of(
        Reserved.Ranges.begin(), Reserved.Ranges.end(), [&](const auto &Value) {
          return overlaps(Value.first, Value.second, Output.Address, Extent);
        });
  };
  std::vector<OutputSection> Result;
  for (const auto *Value : Ordered) {
    const auto Kind = sectionKind(Graph.format(), *Value);
    if (!Kind)
      return writeError("unsupported section kind");
    if (Value->Content.size() > Value->VirtualSize)
      return writeError("section content exceeds its virtual size");
    const auto Address = subtract(Value->Address, ImageBase);
    const auto End = add(Value->Address, Value->VirtualSize);
    if (!Address || !End)
      return writeError("section address overflow");
    const auto RelativeEnd = subtract(*End, ImageBase);
    if (!RelativeEnd)
      return writeError("section end precedes the image base");
    if (Result.empty() || Result.back().Kind != *Kind ||
        CrossesReserved(Result.back(), *Address, *RelativeEnd)) {
      Result.push_back(
          OutputSection{*Kind, *Address, Value->VirtualSize, Value->Content});
      continue;
    }
    auto &Output = Result.back();
    if (*Address < Output.Address || Output.Size > *Address - Output.Address)
      return writeError("section overlaps the previous output section");
    const uint64_t Gap = *Address - Output.Address;
    if (*RelativeEnd < Output.Address)
      return writeError("section end precedes the output section");
    const uint64_t Size = *RelativeEnd - Output.Address;
    if (!fitsSizeT(Gap) || !fitsSizeT(Size))
      return writeError("section gap or size is too large");
    if (Output.Kind != AOT::SectionKind::BSS) {
      if (Gap > Output.Content.max_size() ||
          Value->Content.size() >
              Output.Content.max_size() - static_cast<size_t>(Gap))
        return writeError("merged section content is too large");
      Output.Content.resize(static_cast<size_t>(Gap));
      Output.Content.insert(Output.Content.end(), Value->Content.begin(),
                            Value->Content.end());
    }
    Output.Size = Size;
  }
  if (!Reserved.Present)
    return Result;
  if (Graph.format() != ObjectFormat::COFF)
    return writeError("PE runtime function table outside a COFF object");
  auto PData = normalizePERuntimeFunctions(Graph, ImageBase);
  if (!PData)
    return writeError("PE runtime function table: " + PData.error().Message);
  const auto End = add(Reserved.Address, static_cast<uint64_t>(PData->size()));
  if (!End || *End > Reserved.End)
    return writeError("PE runtime function table exceeds its reservation");
  if (std::any_of(Result.begin(), Result.end(), [&](const auto &Value) {
        return overlaps(Reserved.Address, PData->size(), Value.Address,
                        Value.Size);
      }))
    return writeError("PE runtime function table overlaps an output section");
  Result.push_back(OutputSection{AOT::SectionKind::Unwind, Reserved.Address,
                                 PData->size(), std::move(*PData)});
  std::sort(Result.begin(), Result.end(),
            [](const auto &Left, const auto &Right) {
              return std::tie(Left.Kind, Left.Address) <
                     std::tie(Right.Kind, Right.Address);
            });
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
  std::string_view Name = Value.exportedName();
  if (Graph.format() == ObjectFormat::MachO && !Name.empty() && Name[0] == '_')
    Name.remove_prefix(1);
  return Name;
}

struct IndexedTable {
  std::vector<uint64_t> Addresses;
  std::vector<bool> Present;
};

struct SemanticTables {
  uint64_t Version = 0;
  uint64_t Intrinsics = 0;
  IndexedTable Types;
  IndexedTable Codes;
};

LinkExpect<SemanticTables> semanticTables(const LinkGraph &Graph,
                                          uint64_t ImageBase) {
  SemanticTables Result;
  uint64_t FirstCode = std::numeric_limits<uint64_t>::max();
  bool HasVersion = false;
  bool HasIntrinsics = false;
  auto Insert = [](IndexedTable &Table, uint64_t Index, uint64_t Address,
                   std::string_view Kind) -> LinkExpect<void> {
    if (Index == std::numeric_limits<uint64_t>::max())
      return writeError(std::string(Kind) + " index out of range");
    const uint64_t NextSize = Index + 1;
    if (!fitsSizeT(NextSize) || NextSize > Table.Addresses.max_size() ||
        NextSize > Table.Present.max_size())
      return writeError(std::string(Kind) + " table is too large");
    if (Table.Present.size() > Index && Table.Present[Index])
      return writeError("duplicate " + std::string(Kind) + " index");
    Table.Addresses.resize(
        std::max(Table.Addresses.size(), static_cast<size_t>(NextSize)));
    Table.Present.resize(Table.Addresses.size());
    Table.Addresses[Index] = Address;
    Table.Present[Index] = true;
    return {};
  };
  for (const auto &SymbolValue : Graph.symbols()) {
    const auto Address = symbolAddress(Graph, SymbolValue, ImageBase);
    if (!Address)
      return writeError("symbol address precedes the image base");
    const auto Name = semanticName(Graph, SymbolValue);
    if (!Name)
      continue;
    uint64_t Index = 0;
    if (*Name == "version") {
      if (HasVersion)
        return writeError("duplicate version symbol");
      HasVersion = true;
      Result.Version = *Address;
    } else if (*Name == "intrinsics") {
      if (HasIntrinsics)
        return writeError("duplicate intrinsics symbol");
      HasIntrinsics = true;
      Result.Intrinsics = *Address;
    } else if (indexedSymbol(*Name, 't', Index) &&
               std::to_string(Index) == Name->substr(1)) {
      EXPECTED_TRY(Insert(Result.Types, Index, *Address, "type"));
    } else if (indexedSymbol(*Name, 'f', Index) &&
               std::to_string(Index) == Name->substr(1)) {
      EXPECTED_TRY(Insert(Result.Codes, Index, *Address, "code"));
      FirstCode = std::min(FirstCode, Index);
    }
  }
  auto &Types = Result.Types;
  auto &Codes = Result.Codes;
  if (!HasVersion || !HasIntrinsics ||
      std::find(Types.Present.begin(), Types.Present.end(), false) !=
          Types.Present.end())
    return writeError("invalid semantic symbol tables version="s +
                      (HasVersion ? "true" : "false") +
                      " intrinsics=" + (HasIntrinsics ? "true" : "false") +
                      " types=" + std::to_string(Types.Addresses.size()) +
                      " codes=" + std::to_string(Codes.Addresses.size()));
  if (FirstCode != std::numeric_limits<uint64_t>::max()) {
    Codes.Addresses.erase(
        Codes.Addresses.begin(),
        Codes.Addresses.begin() +
            static_cast<std::vector<uint64_t>::difference_type>(FirstCode));
    Codes.Present.erase(
        Codes.Present.begin(),
        Codes.Present.begin() +
            static_cast<std::vector<bool>::difference_type>(FirstCode));
  }
  if (std::find(Codes.Present.begin(), Codes.Present.end(), false) !=
      Codes.Present.end())
    return writeError("missing code table entry");
  return Result;
}

Expect<void> writeAddresses(Writer &Output, const SemanticTables &Tables) {
  EXPECTED_TRY(Output.writeU64(Tables.Version));
  EXPECTED_TRY(Output.writeU64(Tables.Intrinsics));
  for (const auto *Table : {&Tables.Types, &Tables.Codes}) {
    EXPECTED_TRY(Output.writeU64(Table->Addresses.size()));
    for (const auto Address : Table->Addresses)
      EXPECTED_TRY(Output.writeU64(Address));
  }
  return {};
}

Expect<void> writeHeader(Writer &Output) {
  EXPECTED_TRY(Output.writeName("wasmedge"sv));
  EXPECTED_TRY(Output.writeU32(AOT::kBinaryVersion));
  EXPECTED_TRY(Output.writeByte(static_cast<uint8_t>(AOT::kHostOSType)));
  return Output.writeByte(static_cast<uint8_t>(AOT::kHostArchitecture));
}

Expect<void> writeSections(Writer &Output, Span<const OutputSection> Sections) {
  EXPECTED_TRY(Output.writeU32(static_cast<uint32_t>(Sections.size())));
  for (const auto &Value : Sections) {
    EXPECTED_TRY(Output.writeByte(static_cast<uint8_t>(Value.Kind)));
    EXPECTED_TRY(Output.writeU64(Value.Address));
    EXPECTED_TRY(Output.writeU64(Value.Size));
    EXPECTED_TRY(Output.writeLengthPrefixed(Value.Content));
  }
  return Output.close();
}

Expect<void> writeContainer(Writer &Output, Span<const Byte> Wasm,
                            Span<const Byte> Payload) {
  EXPECTED_TRY(Output.write(Wasm));
  EXPECTED_TRY(Output.writeByte(CustomSectionId));
  EXPECTED_TRY(Output.writeU32(static_cast<uint32_t>(Payload.size())));
  EXPECTED_TRY(Output.write(Payload));
  return Output.close();
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

Unexpected<Diagnostic> unsupportedRebase(const LinkGraph &Graph,
                                         const Rebase &Value) {
  Diagnostic Diag("universal writer: rebase unsupported width=" +
                  std::to_string(Value.Width) +
                  " format=" + std::string(objectFormatName(Value.Format)));
  Diag.Section = Value.Section;
  Diag.Offset = Value.Offset;
  Diag.RelocationType = Value.Type;
  Diag.SectionName = Value.Section < Graph.sections().size()
                         ? Graph.sections()[Value.Section].Name
                         : "<invalid>";
  return diagnosticError(std::move(Diag));
}

LinkExpect<void> validateWasmSections(Span<const Byte> Wasm) {
  constexpr std::array<Byte, 8> WasmHeader{0x00, 0x61, 0x73, 0x6D,
                                           0x01, 0x00, 0x00, 0x00};
  if (Wasm.size() < WasmHeader.size() ||
      !std::equal(WasmHeader.begin(), WasmHeader.end(), Wasm.begin()))
    return writeError("invalid wasm header");
  size_t Offset = WasmHeader.size();
  while (Offset < Wasm.size()) {
    const Byte SectionId = Wasm[Offset++];
    if (SectionId > LastCoreSectionId)
      return writeError("invalid wasm section id");
    uint32_t Size = 0;
    unsigned Shift = 0;
    Byte Encoded = 0;
    do {
      if (Offset >= Wasm.size() || Shift >= ULEB32ShiftLimit)
        return writeError("malformed wasm section size");
      Encoded = Wasm[Offset++];
      if (Shift == ULEB32FinalShift && (Encoded & ULEB32FinalUnusedBits) != 0)
        return writeError("malformed wasm section size");
      Size |= static_cast<uint32_t>(Encoded & 0x7F) << Shift;
      Shift += 7;
    } while ((Encoded & 0x80) != 0);
    if (Offset > Wasm.size() || Size > Wasm.size() - Offset)
      return writeError("wasm section extends beyond the input");
    Offset += Size;
  }
  return {};
}

} // namespace

LinkExpect<void>
UniversalWasmWriter::write(const LinkGraph &Graph, Span<const Byte> Wasm,
                           const std::filesystem::path &Output) noexcept {
  try {
    std::vector<Byte> Bytes;
    Writer Buffer(Bytes);
    EXPECTED_TRY(write(Graph, Wasm, Buffer));
    auto File = Writer::open(Output);
    if (!File)
      return writeError("failed to open output", DiagnosticKind::IO);
    return writeImage(*File, Bytes, "universal Wasm");
  } catch (...) {
    return writeError("unexpected exception while writing");
  }
}

LinkExpect<void> UniversalWasmWriter::write(const LinkGraph &Graph,
                                            Span<const Byte> Wasm,
                                            Writer &Result) {
  EXPECTED_TRY(validateWasmSections(Wasm));
  const auto Arch = Internal::architecture(Graph.target());
  if (AOT::kHostOSType == AOT::OSType::Unsupported)
    return writeError("unsupported host operating system");
  if (!Arch)
    return writeError("unsupported object architecture");
  if (*Arch != AOT::kHostArchitecture)
    return writeError("object architecture does not match the host");
  if (!Graph.relocationsApplied())
    return writeError("relocations have not been applied");
  if (!Graph.rebases().empty())
    return unsupportedRebase(Graph, Graph.rebases().front());
  const uint64_t ImageBase = runtimeImageBase(Graph);

  std::vector<Byte> Payload;
  Writer Section(Payload);
  EXPECTED_TRY(encoded(writeHeader(Section), "encode the custom section"));
  EXPECTED_TRY(auto Tables, semanticTables(Graph, ImageBase));
  EXPECTED_TRY(
      encoded(writeAddresses(Section, Tables), "encode the custom section"));
  EXPECTED_TRY(auto Sections, outputSections(Graph, ImageBase));
  if (Sections.size() > std::numeric_limits<uint32_t>::max())
    return writeError("too many output sections");
  EXPECTED_TRY(
      encoded(writeSections(Section, Sections), "encode the custom section"));
  if (Payload.size() > UINT32_MAX)
    return writeError("custom section payload is too large");
  return encoded(writeContainer(Result, Wasm, Payload), "write output");
}

} // namespace Linker
} // namespace LLVM
} // namespace WasmEdge
